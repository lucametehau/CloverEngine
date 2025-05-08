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
#include "search.h"
#include <atomic>
#include <cmath>
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>

#ifdef GENERATE

constexpr bool FRC_DATAGEN = true;
constexpr int MIN_NODES = 5000;
constexpr int MAX_NODES = (1 << 20);

constexpr int ADJUDICATION_MAX_SCORE = 8000;
constexpr int ADJUDICATION_DRAW_SCORE = 10;
constexpr int ADJUDICATION_WIN_CNT = 4;
constexpr int ADJUDICATION_DRAW_CNT = 10;

struct FenData
{
    int score;
    std::string fen;
};

void generateFens(SearchThread &thread_data, std::atomic<uint64_t> &total_fens_count, std::atomic<uint64_t> &num_games,
                  uint64_t fens_limit, std::string path, uint64_t seed, uint64_t extraSeed)
{
    std::ofstream out(path);
    std::mt19937_64 gn(
        0); //(std::chrono::system_clock::now().time_since_epoch().count() + extraSeed) ^ 8257298672678ULL);
    std::uniform_int_distribution<uint32_t> rnd_dfrc(0, 960 * 960);
    std::array<FenData, 10000> fens;
    // std::unique_ptr<std::deque<HistoricalState>> states;

    Info info;
    int gameInd = 1;
    uint64_t fens_count = 0;

    info.init();
    info.set_chess960(FRC_DATAGEN);
    info.set_min_nodes(MIN_NODES);
    info.set_max_nodes(MAX_NODES);

    std::mutex M;

    thread_data.TT = new HashTable();
    thread_data.TT->init(4 * MB);
    thread_data.m_info = info;

    thread_data.m_board.chess960 = FRC_DATAGEN;

    while (fens_count < fens_limit)
    {
        std::unique_ptr<std::deque<HistoricalState>> states;
        states = std::make_unique<std::deque<HistoricalState>>(1);
        FenData data;
        double result = 0;
        int ply = 0;
        int nr_fens = 0;

        int winCnt = 0, drawCnt = 0, idx = 0;

        if (FRC_DATAGEN)
        {
            idx = rnd_dfrc(gn) % (960 * 960);
            thread_data.m_board.set_dfrc(idx, states->back());
        }
        else
            thread_data.m_board.set_fen(START_POS_FEN, states->back());

        thread_data.clear_history();
        thread_data.clear_stack();

        std::uniform_int_distribution<int> rnd_ply(0, 100000);

        int additionalPly = rnd_ply(gn) % 2;

        while (true)
        {
            Move move;
            int score;

            // game over checking
            if (thread_data.m_board.is_draw(0))
            {
                result = 0.5;
                break;
            }

            MoveList moves;
            int nr_moves = thread_data.m_board.gen_legal_moves<MOVEGEN_ALL>(moves);

            // std::cout << "-------------------\n";
            // thread_data.m_board.print();
            // std::cout << thread_data.m_board.chess960 << "\n";

            if (!nr_moves)
            {
                if (thread_data.m_board.checkers())
                    result = thread_data.m_board.turn == WHITE ? 0.0 : 1.0;
                else
                    result = 0.5;

                break;
            }

            if (ply < 8 + additionalPly)
            {
                std::uniform_int_distribution<uint32_t> rnd(0, nr_moves - 1);
                move = moves[rnd(gn)];
                states->emplace_back();
                thread_data.make_move(move, states->back());
            }
            else
            {
                thread_data.TT->age();
                thread_data.m_board.clear();
                thread_data.m_state = ThreadStates::SEARCH;
                thread_data.start_search();

                score = thread_data.m_root_scores[1], move = thread_data.m_best_moves[1];

                if (!thread_data.m_board.checkers() && !thread_data.m_board.is_noisy_move(move) &&
                    abs(score) < ADJUDICATION_MAX_SCORE)
                {
                    data.fen = thread_data.m_board.fen();
                    data.score = score * (thread_data.m_board.turn == WHITE ? 1 : -1);
                    fens[nr_fens++] = data;
                }

                winCnt = (abs(score) >= ADJUDICATION_MAX_SCORE ? winCnt + 1 : 0);
                drawCnt = (abs(score) <= ADJUDICATION_DRAW_SCORE ? drawCnt + 1 : 0);

                if (winCnt >= ADJUDICATION_WIN_CNT)
                {
                    score *= (thread_data.m_board.turn == WHITE ? 1 : -1);
                    result = score < 0 ? 0.0 : 1.0;

                    break;
                }

                if (drawCnt >= ADJUDICATION_DRAW_CNT)
                {
                    result = 0.5;
                    break;
                }

                states->emplace_back();
                thread_data.make_move(move, states->back());
            }

            ply++;
        }

        for (int i = 0; i < nr_fens; i++)
        {
            out << fens[i].fen << " | " << fens[i].score << " | "
                << (result > 0.6 ? "1.0" : (result < 0.4 ? "0.0" : "0.5")) << "\n";
        }

        total_fens_count.fetch_add(nr_fens);
        num_games.fetch_add(1);
        fens_count += nr_fens;

        /*M.lock();

        sumFens = sumFens + nr_fens;
        totalFens += nr_fens;
        M.unlock();*/

        gameInd++;
    }
}
#endif

void generateData(uint64_t num_fens, int num_threads, std::string rootPath, uint64_t extraSeed = 0)
{
#ifdef GENERATE
    printStats = false;
    std::array<std::string, 100> path;

    srand(time(0));

    for (int i = 0; i < num_threads; i++)
        path[i] = rootPath + std::to_string(i) + ".txt";

    std::vector<std::thread> threads(num_threads);
    thread_pool.create_pool(num_threads);
    for (auto &t : thread_pool.m_threads)
        t->m_thread_id = 0;
    uint64_t batch = num_fens / num_threads;
    std::size_t i = 0;

    std::mt19937 gen(time(0));
    std::uniform_int_distribution<uint64_t> rng;
    std::atomic<uint64_t> total_fens_count{0}, num_games{0};
    std::atomic<uint64_t> num_fens_atomic{num_fens};
    std::time_t startTime = get_current_time();

    for (auto &t : threads)
    {
        std::string pth = path[i];
        std::cout << "Starting thread " << i << std::endl;
        // generateFens(*thread_pool.m_threads[i], total_fens_count, num_games, batch, pth, rng(gen), extraSeed);
        t = std::thread{generateFens,
                        std::ref(*thread_pool.m_threads[i]),
                        std::ref(total_fens_count),
                        std::ref(num_games),
                        batch,
                        pth,
                        rng(gen),
                        extraSeed};
        i++;
    }

    while (total_fens_count <= num_fens_atomic)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::time_t time_elapsed = get_current_time() - startTime;
        std::cout << "Games " << std::setw(9) << num_games << "; Fens " << std::setw(11) << total_fens_count
                  << " ; Time elapsed: " << std::setw(9) << std::floor(time_elapsed / 1000.0) << "s ; "
                  << "fens/s " << std::setw(9)
                  << static_cast<uint64_t>(std::floor(1LL * total_fens_count * 1000 / time_elapsed)) << "\r";
    }

    for (auto &t : threads)
        t.join();

    std::cout << "\n";
#endif
}
