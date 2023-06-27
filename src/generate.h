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
struct FenData {
    int score;
    std::string fen;
};

void generateFens(std::atomic <uint64_t>& sumFens, uint64_t nrFens, std::string path, uint64_t seed) {
    std::ofstream out(path);
    std::mt19937_64 gn((std::chrono::system_clock::now().time_since_epoch().count() + 246515411586195ULL) ^ 43298590881908ULL);

    Info info[1];
    int gameInd = 1;
    uint64_t totalFens = 0;

    Search* searcher = new Search();

    info->timeset = false;
    info->depth = DEPTH;
    info->startTime = getTime();
    info->nodes = 5000;

    info->multipv = 1;
    searcher->setTime(info);

    std::mutex M;

    searcher->TT = new HashTable();
    searcher->principalSearcher = true;
    FenData fens[10000];

    while (totalFens < nrFens) {
        FenData data;
        double result = 0;
        int ply = 0;
        int nr_fens = 0;

        int winCnt = 0, drawCnt = 0;

        searcher->_setFen(START_POS_FEN);

        searcher->clearHistory();
        searcher->clearStack();

        searcher->TT->initTable(4 * MB);
        searcher->TT->resetAge();

        while (true) {
            uint16_t move;
            int score;

            /// game over checking

            if (isRepetition(searcher->board, 0) || searcher->board.halfMoves >= 100 || searcher->board.isMaterialDraw()) {
                result = 0.5;
                break;
            }

            uint16_t moves[256];

            int nrMoves = genLegal(searcher->board, moves);

            if (!nrMoves) {
                if (searcher->board.checkers) {
                    result = (searcher->board.turn == WHITE ? 0.0 : 1.0);
                }
                else {
                    result = 0.5;
                }

                break;
            }

            if (ply < 8) { /// simulating a book ?
                std::uniform_int_distribution <uint32_t> rnd(0, nrMoves - 1);
                move = moves[rnd(gn)];
                searcher->_makeMove(move);
            }
            else {
                searcher->TT->age();
                searcher->clearBoard();

                //searcher->board.print();
                std::pair <int, uint16_t> res = searcher->startSearch(info);
                score = res.first, move = res.second;

                //log << searcher->board.fen() << " " << toString(move) << "\n";

                if (nrMoves == 1) { /// in this case, engine reports score 0, which might be misleading
                    searcher->_makeMove(move);
                    ply++;
                    continue;
                }

                searcher->flag = 0;

                if (!searcher->board.checkers && !isNoisyMove(searcher->board, move)) { /// relatively quiet position
                    data.fen = searcher->board.fen();
                    data.score = score;
                    fens[nr_fens++] = data;
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

                searcher->_makeMove(move);
            }

            ply++;
        }

        for (int i = 0; i < nr_fens; i++) {
            out << fens[i].fen << " [" << result << "] " << fens[i].score << "\n";
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


void generateData(uint64_t nrFens, int nrThreads, std::string rootPath) {
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
        t = std::thread{ generateFens, std::ref(sumFens), batch, pth, rng(gen) };
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
