/*
  Clover is a UCI chess playing engine authored by Luca Metehau.
  <https://github.com/lucametehau/CloverEngine>

  Clover is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clover is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "../search.h"
#include "binpack.h"
#include <atomic>
#include <cmath>
#include <csignal>
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>


#ifdef GENERATE

constexpr bool FRC_DATAGEN = false;
constexpr int MIN_NODES = 20000;
constexpr int MAX_NODES = (1 << 20);
constexpr int ADJ_OPENING_THRESHOLD = 400;

constexpr bool DO_ADJ = true;
constexpr int ADJ_WIN_PLY_THRESHOLD = 4;
constexpr int ADJ_WIN_SCORE_THRESHOLD = 2000;
constexpr int ADJ_DRAW_PLY_THRESHOLD = 10;
constexpr int ADJ_DRAW_SCORE_THRESHOLD = 10;

std::atomic<bool> stop_requested{false};

void handle_sigint(int) {
    stop_requested = true;
}

void generate_fens(SearchThread &white_thread_data, SearchThread &black_thread_data, std::atomic<uint64_t> &total_fens_count, std::atomic<uint64_t> &num_games, uint64_t fens_limit, std::string path)
{
    std::ofstream out(path, std::ios::binary);
    std::ofstream seed_out("seed_" + path);
    std::mt19937_64 gn(std::chrono::system_clock::now().time_since_epoch().count() ^ white_thread_data.thread_id);
    std::uniform_int_distribution<uint32_t> rnd_dfrc(0, 960 * 960 - 1);
    BinpackFormat binpack;

    Info info;
    uint64_t fens_count = 0;

    info.init();
    info.set_chess960(FRC_DATAGEN);
    info.set_min_nodes(MIN_NODES);
    info.set_max_nodes(MAX_NODES);

    std::vector<SearchThread*> threads{&black_thread_data, &white_thread_data};

    for (auto &thread_data : threads) {
        thread_data->thread_id = 0;
        thread_data->TT = new HashTable();
        thread_data->TT->init(4 * MB);
        thread_data->info = info;
    }


    while (fens_count < fens_limit && !stop_requested)
    {
        std::unique_ptr<std::deque<HistoricalState>> states[2];
        for (auto color : {WHITE, BLACK})
            states[color] = std::make_unique<std::deque<HistoricalState>>(1);
        int ply = 0;

        if constexpr (FRC_DATAGEN) {
            int idx = rnd_dfrc(gn);
            for (auto color : {WHITE, BLACK})
                threads[color]->board.set_dfrc(idx, states[color]->back());
            seed_out << idx << ":";
        }
        else {   
            for (auto color : {WHITE, BLACK})
                threads[color]->board.set_fen(START_POS_FEN, states[color]->back());
        }

        for (auto color : {WHITE, BLACK}) {
            threads[color]->clear_history();
            threads[color]->clear_stack();
        }

        std::uniform_int_distribution<int> rnd_ply(0, 100000);

        int extra_ply = rnd_ply(gn) % 2;
        int book_ply_count = 8 + extra_ply;
        int win_count = 0, draw_count = 0;
        bool turn = WHITE;

        while (true)
        {
            Move move;
            int score;

            // game over checking
            if (threads[turn]->board.is_draw(0))
            {
                binpack.set_result(1);
                break;
            }

            MoveList moves;
            int nr_moves = threads[turn]->board.gen_legal_moves<MOVEGEN_ALL>(moves);

            if (!nr_moves)
            {
                if (threads[turn]->board.checkers())
                    binpack.set_result(threads[turn]->board.turn == WHITE ? 0 : 2);
                else
                    binpack.set_result(1);
                break;
            }

            if (ply < book_ply_count)
            {
                std::uniform_int_distribution<uint32_t> rnd(0, nr_moves - 1);
                move = moves[rnd(gn)];
                for (auto color : {WHITE, BLACK}) {
                    states[color]->emplace_back();
                    threads[color]->board.make_move(move, states[color]->back());
                }
                seed_out << move.to_string(FRC_DATAGEN) << " ";

                if (ply == book_ply_count - 1) {
                    binpack.init(threads[turn]->board);
                    seed_out << "\n";
                }
            }
            else
            {
                threads[turn]->TT->age();
                threads[turn]->board.clear();
                threads[turn]->state = ThreadStates::SEARCH;
                threads[turn]->start_search();

                score = threads[turn]->root_scores[1] * (turn == WHITE ? 1 : -1);
                move = threads[turn]->best_moves[1];


                // too big score out of book
                if (ply == book_ply_count && abs(score) >= ADJ_OPENING_THRESHOLD) {
                    break;
                }

                binpack.add_move(move, score);

                if constexpr (DO_ADJ) {
                    win_count = abs(score) >= ADJ_WIN_SCORE_THRESHOLD ? win_count + 1 : 0;
                    draw_count = abs(score) <= ADJ_DRAW_SCORE_THRESHOLD ? draw_count + 1 : 0;

                    if (win_count == ADJ_WIN_PLY_THRESHOLD) {
                        binpack.set_result(score < 0 ? 0 : 2);
                        break;
                    }
                    if (draw_count == ADJ_DRAW_PLY_THRESHOLD) {
                        binpack.set_result(1);
                        break;
                    }
                }

                for (auto color : {WHITE, BLACK}) {
                    states[color]->emplace_back();
                    threads[color]->board.make_move(move, states[color]->back());
                }
            }

            ply++;
            turn ^= 1;
        }

        if (binpack.size() == 0)
            continue;

        binpack.write(out);
        total_fens_count.fetch_add(binpack.size());
        num_games.fetch_add(1);
        fens_count += binpack.size();
        
        if (stop_requested) break;
    }
}

void play_datagen_game(uint64_t dfrc_index, std::vector<std::string> &moves)
{
    Info info;

    thread_pool.create_pool(1);
    thread_pool.threads[0]->thread_id = 0;

    auto &thread_data = *thread_pool.threads[0];

    info.init();
    info.set_chess960(FRC_DATAGEN);
    info.set_min_nodes(MIN_NODES);
    info.set_max_nodes(MAX_NODES);

    thread_data.TT = new HashTable();
    thread_data.TT->init(4 * MB);
    thread_data.info = info;

    std::unique_ptr<std::deque<HistoricalState>> states;
    states = std::make_unique<std::deque<HistoricalState>>(1);
    double result = 0;
    int ply = 0;
    int nr_fens = 0;
    int nr_moves;

    thread_data.clear_history();
    thread_data.clear_stack();

    thread_data.board.set_dfrc(dfrc_index, states->back());


    std::cout << dfrc_index << "\n";

    for (auto &move : moves) {
        states->emplace_back();
        thread_data.board.make_move(parse_move_string(thread_data.board, move, info), states->back());
        thread_data.board.print();
        std::cout << move << std::endl;
    }

    thread_data.board.set_fen("bqr1k1rn/ppbp1ppp/2p3n1/4p3/P7/2PP3P/1P2PPP1/BRNQKBRN w GBgc - 0 1", states->back());

    // MoveList pos_moves;
    // int nr_moves = thread_data.board.gen_legal_moves<MOVEGEN_ALL>(pos_moves);

    // for (int i = 0; i < nr_moves; i++)
    // {
    //     std::cout << pos_moves[i].to_string(true) << "\n";
    // }

    thread_data.board.print();

    while (true)
    {
        Move move;
        int score;

        // game over checking
        if (thread_data.board.is_draw(0))
        {
            result = 0.5;
            break;
        }

        MoveList moves;
        nr_moves = thread_data.board.gen_legal_moves<MOVEGEN_ALL>(moves);

        if (!nr_moves)
        {
            if (thread_data.board.checkers())
                result = thread_data.board.turn == WHITE ? 0.0 : 1.0;
            else
                result = 0.5;
            break;
        }

        thread_data.TT->age();
        thread_data.board.clear();
        thread_data.state = ThreadStates::SEARCH;
        thread_data.start_search();

        score = thread_data.root_scores[1] * (thread_data.board.turn == WHITE ? 1 : -1);
        move = thread_data.best_moves[1];

        thread_data.board.print();
        std::cout << move.to_string(true) << " " << score << std::endl;

        states->emplace_back();
        thread_data.board.make_move(move, states->back());
        nr_fens++;
        ply++;
    }
    
    std::cout << "Result: " << result << " | Fens: " << nr_fens << std::endl;
}

void generateData(uint64_t num_fens, int num_threads, std::string rootPath, uint64_t extraSeed = 0)
{
    std::signal(SIGINT, handle_sigint);
    printStats = false;

    srand(time(0));


    std::vector<std::thread> threads(num_threads);
    thread_pool.create_pool(2 * num_threads);
    uint64_t batch = num_fens / num_threads;
    std::size_t i = 0;

    std::mt19937 gen(time(0));
    std::uniform_int_distribution<uint64_t> rng;
    std::vector<std::string> path(num_threads);
    std::atomic<uint64_t> total_fens_count{0}, num_games{0};
    std::atomic<uint64_t> num_fens_atomic{num_fens};
    std::time_t startTime = get_current_time();

    for (int i = 0; i < num_threads; i++)
        path[i] = rootPath + "-" + std::to_string(rng(gen)) + ".bin";

    for (auto &t : threads)
    {
        std::string pth = path[i / 2];
        std::cout << "Starting threads " << i << ", " << i + 1 << std::endl;
        t = std::thread{generate_fens, std::ref(*thread_pool.threads[i]), std::ref(*thread_pool.threads[i + 1]), std::ref(total_fens_count), std::ref(num_games), batch, pth};
        i += 2;
    }

    while (total_fens_count <= num_fens_atomic)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::time_t time_elapsed = (get_current_time() - startTime) / 1000;
        uint64_t speed = static_cast<uint64_t>(total_fens_count / (time_elapsed + !time_elapsed));
        uint64_t time_left = std::max<uint64_t>(0ull, num_fens - total_fens_count) / (speed + !speed);
        std::cout << "Games: " << std::setw(10) << num_games << " | Fens: " << std::setw(11) << total_fens_count << " | ";
        std::cout << "Time Elapsed: " << std::setw(4) << time_elapsed / 3600 << "h " 
                  << (time_elapsed % 3600) / 600 << ((time_elapsed % 3600) / 60) % 10 << "min "
                  << (time_elapsed % 60) / 10 << (time_elapsed % 60) % 10 << "s | ";
        std::cout << "Fens/s: " << std::setw(9) << speed << " | ";
        std::cout << "ETA: " << std::setw(4) << time_left / 3600 << "h " 
                  << (time_left % 3600) / 600 << ((time_left % 3600) / 60) % 10 << "min "
                  << (time_left % 60) / 10 << (time_left % 60) % 10 << "s\r";
        if (stop_requested) {
            std::cout << "\nStopping generation...\n";
            std::this_thread::sleep_for(std::chrono::seconds(10));
            break;
        }
    }

    for (auto &t : threads)
        t.join();

    std::cout << "\n";
#endif
}
