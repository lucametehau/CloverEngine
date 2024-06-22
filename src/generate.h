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
#include <fstream>
#include <vector>
#include <thread>
#include <ctime>
#include <atomic>
#include "search.h"


#ifdef GENERATE

const bool FRC_DATAGEN = true;

struct FenData {
    int score;
    std::string fen;
};

void generateFens(std::atomic <uint64_t>& sumFens, uint64_t nrFens, std::string path, uint64_t seed, uint64_t extraSeed) {
    std::ofstream out(path);
    std::mt19937_64 gn((std::chrono::system_clock::now().time_since_epoch().count() + extraSeed) ^ 8257298672678ULL);

    Info info[1];
    int gameInd = 1;
    uint64_t totalFens = 0;

    Search* searcher = new Search();

    info->timeset = false;
    info->depth = DEPTH;
    info->startTime = getTime();
    info->min_nodes = 5000;
    info->max_nodes = (1 << 20);
    info->nodes = -1;

    info->multipv = 1;
    searcher->setTime(info);

    std::mutex M;

    searcher->TT = new HashTable();
    searcher->principalSearcher = true;
    FenData fens[10000];
    std::uniform_int_distribution <uint32_t> rnd_dfrc(0, 960 * 960);

    while (totalFens < nrFens) {
        FenData data;
        double result = 0;
        int ply = 0;
        int nr_fens = 0;

        int winCnt = 0, drawCnt = 0, idx = 0;

        if (FRC_DATAGEN) {
            idx = rnd_dfrc(gn) % (960 * 960);
        }

        searcher->set_dfrc(idx);

        searcher->clear_history();
        searcher->clear_stack();

        searcher->TT->initTable(4 * MB);
        std::uniform_int_distribution <int> rnd_ply(0, 100000);

        int additionalPly = rnd_ply(gn) % 2;

        while (true) {
            uint16_t move;
            int score;

            /// game over checking

            if (searcher->board.is_draw(0)) {
                result = 0.5;
                break;
            }

            uint16_t moves[256];

            int nrMoves = gen_legal_moves(searcher->board, moves);

            if (!nrMoves) {
                if (searcher->board.checkers) {
                    result = (searcher->board.turn == WHITE ? 0.0 : 1.0);
                }
                else {
                    result = 0.5;
                }

                break;
            }

            if (ply < 8 + additionalPly) { /// simulating a book ?
                std::uniform_int_distribution <uint32_t> rnd(0, nrMoves - 1);
                move = moves[rnd(gn)];
                searcher->make_move(move);
            }
            else {
                searcher->TT->age();
                searcher->clear_board();

                //searcher->board.print();
                std::pair <int, uint16_t> res = searcher->start_search(info);
                score = res.first, move = res.second;

                //log << searcher->board.fen() << " " << move_to_string(move) << "\n";

                if (nrMoves == 1) { /// in this case, engine reports score 0, which might be misleading
                    searcher->make_move(move);
                    ply++;
                    continue;
                }

                searcher->flag = 0;

                if (!searcher->board.checkers && !isNoisyMove(searcher->board, move) && abs(score) < 1000) { /// relatively quiet position
                    data.fen = searcher->board.fen();
                    data.score = score * (searcher->board.turn == WHITE ? 1 : -1);
                    fens[nr_fens++] = data;
                    //std::cout << searcher->nodes << std::endl;
                }

                winCnt = (abs(score) >= 1000 ? winCnt + 1 : 0);
                drawCnt = (abs(score) <= 20 ? drawCnt + 1 : 0);

                if (winCnt >= 4) {
                    score *= (searcher->board.turn == WHITE ? 1 : -1);
                    result = (score < 0 ? 0.0 : 1.0);

                    break;
                }

                if (drawCnt >= 12) {
                    result = 0.5;

                    break;
                }

                searcher->make_move(move);
            }

            ply++;
        }

        for (int i = 0; i < nr_fens; i++) {
            out << fens[i].fen << " | " << fens[i].score << " | " << (result > 0.6 ? "1.0" : (result < 0.4 ? "0.0" : "0.5")) << "\n";
        }

        sumFens.fetch_add(nr_fens);
        totalFens += nr_fens;

        /*M.lock();

        sumFens = sumFens + nr_fens;
        totalFens += nr_fens;
        M.unlock();*/

        gameInd++;
    }
}
#endif


void generateData(uint64_t nrFens, int nrThreads, std::string rootPath, uint64_t extraSeed = 0) {
#ifdef GENERATE
    printStats = false;
    std::string path[100];

    srand(time(0));

    for (int i = 0; i < nrThreads; i++) {
        path[i] = rootPath;

        if (i < 10)
            path[i] += char(i + '0');
        else
            path[i] += char(i / 10 + '0'), path[i] += char(i % 10 + '0');
        path[i] += ".txt";
        std::cout << path[i] << "\n";
    }

    std::vector <std::thread> threads(nrThreads);
    int batch = nrFens / nrThreads, i = 0;

    std::cout << batch << "\n";

    std::mt19937 gen(time(0));
    std::uniform_int_distribution <uint64_t> rng;
    std::atomic <uint64_t> sumFens{ 0 };
    std::atomic <uint64_t> totalFens{ nrFens };
    double startTime = getTime();

    for (auto& t : threads) {
        std::string pth = path[i];
        std::cout << "Starting thread " << i << std::endl;
        t = std::thread{ generateFens, std::ref(sumFens), batch, pth, rng(gen), extraSeed };
        i++;
    }

    while (sumFens <= totalFens) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Fens " << std::setw(11) << sumFens << " ; Time elapsed: " << std::setw(9) << (getTime() - startTime) / 1000.0 << "s ; " <<
            "fens/s " << std::setw(9) << 1LL * sumFens * 1000 / (getTime() - startTime) << "\r";
    }

    for (auto& t : threads)
        t.join();

    std::cout << "\n";
#endif
}
