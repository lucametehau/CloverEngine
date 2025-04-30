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

const bool FRC_DATAGEN = true;
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

void generateFens(SearchThread &thread_data, std::atomic<uint64_t> &sumFens, std::atomic<uint64_t> &sumGames,
                  uint64_t nrFens, std::string path, uint64_t seed, uint64_t extraSeed)
{
    std::ofstream out(path);
    std::mt19937_64 gn((std::chrono::system_clock::now().time_since_epoch().count() + extraSeed) ^ 8257298672678ULL);

    Info info;
    int gameInd = 1;
    uint64_t totalFens = 0;

    info.init();
    info.set_min_nodes(MIN_NODES);
    info.set_max_nodes(MAX_NODES);

    std::mutex M;

    thread_data.TT = new HashTable();
    thread_data.TT->init(4 * MB);
    std::array<FenData, 10000> fens;
    std::uniform_int_distribution<uint32_t> rnd_dfrc(0, 960 * 960);

    while (totalFens < nrFens)
    {
        FenData data;
        double result = 0;
        int ply = 0;
        int nr_fens = 0;

        int winCnt = 0, drawCnt = 0, idx = 0;

        if (FRC_DATAGEN)
        {
            idx = rnd_dfrc(gn) % (960 * 960);
        }

        thread_data.board.set_dfrc(idx);

        thread_data.clear_history();
        thread_data.clear_stack();

        std::uniform_int_distribution<int> rnd_ply(0, 100000);

        int additionalPly = rnd_ply(gn) % 2;

        while (true)
        {
            uint16_t move;
            int score;

            // game over checking
            if (thread_data.board.is_draw(0))
            {
                result = 0.5;
                break;
            }

            MoveList moves;
            int nr_moves = thread_data.board.gen_legal_moves<MOVEGEN_ALL>(moves);

            if (!nr_moves)
            {
                if (thread_data.board.checkers())
                    result = (thread_data.board.turn == WHITE ? 0.0 : 1.0);
                else
                    result = 0.5;

                break;
            }

            if (ply < 8 + additionalPly)
            { /// simulating a book ?
                std::uniform_int_distribution<uint32_t> rnd(0, nr_moves - 1);
                move = moves[rnd(gn)];
                thread_data.make_move(move);
            }
            else
            {
                thread_data.TT->age();
                thread_data.board.clear();

                thread_data.start_search(info);
                score = thread_data.root_score[1], move = thread_data.best_move[1];

                if (!thread_data.board.checkers() && !thread_data.board.is_noisy_move(move) &&
                    abs(score) < ADJUDICATION_MAX_SCORE)
                { /// relatively quiet position
                    data.fen = thread_data.board.fen();
                    data.score = score * (thread_data.board.turn == WHITE ? 1 : -1);
                    fens[nr_fens++] = data;
                }

                winCnt = (abs(score) >= ADJUDICATION_MAX_SCORE ? winCnt + 1 : 0);
                drawCnt = (abs(score) <= ADJUDICATION_DRAW_SCORE ? drawCnt + 1 : 0);

                if (winCnt >= ADJUDICATION_WIN_CNT)
                {
                    score *= (thread_data.board.turn == WHITE ? 1 : -1);
                    result = (score < 0 ? 0.0 : 1.0);

                    break;
                }

                if (drawCnt >= ADJUDICATION_DRAW_CNT)
                {
                    result = 0.5;
                    break;
                }

                thread_data.make_move(move);
            }

            ply++;
        }

        for (int i = 0; i < nr_fens; i++)
        {
            out << fens[i].fen << " | " << fens[i].score << " | "
                << (result > 0.6 ? "1.0" : (result < 0.4 ? "0.0" : "0.5")) << "\n";
        }

        sumFens.fetch_add(nr_fens);
        sumGames.fetch_add(1);
        totalFens += nr_fens;

        /*M.lock();

        sumFens = sumFens + nr_fens;
        totalFens += nr_fens;
        M.unlock();*/

        gameInd++;
    }
}
#endif

void generateData(uint64_t nrFens, int nrThreads, std::string rootPath, uint64_t extraSeed = 0)
{
#ifdef GENERATE
    thread_pool.create_pool(nrThreads);
    printStats = false;
    std::string path[100];

    srand(time(0));

    for (int i = 0; i < nrThreads; i++)
    {
        path[i] = rootPath;

        if (i < 10)
            path[i] += char(i + '0');
        else
            path[i] += char(i / 10 + '0'), path[i] += char(i % 10 + '0');
        path[i] += ".txt";
    }

    std::vector<std::thread> threads(nrThreads);
    uint64_t batch = nrFens / nrThreads, i = 0;

    std::mt19937 gen(time(0));
    std::uniform_int_distribution<uint64_t> rng;
    std::atomic<uint64_t> sumFens{0}, sumGames{0};
    std::atomic<uint64_t> totalFens{nrFens};
    std::time_t startTime = get_current_time();

    for (auto &t : threads)
    {
        std::string pth = path[i];
        std::cout << "Starting thread " << i << std::endl;
        t = std::thread{generateFens,
                        std::ref(thread_pool.threads_data[i]),
                        std::ref(sumFens),
                        std::ref(sumGames),
                        batch,
                        pth,
                        rng(gen),
                        extraSeed};
        i++;
    }

    while (sumFens <= totalFens)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::time_t time_elapsed = get_current_time() - startTime;
        std::cout << "Games " << std::setw(9) << sumGames << "; Fens " << std::setw(11) << sumFens
                  << " ; Time elapsed: " << std::setw(9) << std::floor(time_elapsed / 1000.0) << "s ; "
                  << "fens/s " << std::setw(6) << static_cast<uint64_t>(std::floor(1LL * sumFens * 1000 / time_elapsed))
                  << "\r";
    }

    for (auto &t : threads)
        t.join();

    std::cout << "\n";
#endif
}
