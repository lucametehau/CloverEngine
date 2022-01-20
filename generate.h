#pragma once
#include <fstream>
#include <vector>
#include <thread>
#include <ctime>
#include <atomic>
#include "search.h"

struct FenData {
    int score;
    std::string fen;
};

void generateFens(std::atomic <int>& sumFens, int nrFens, std::string path, uint64_t seed) {
    std::ofstream out(path);
    std::mt19937_64 gn(seed + 14756154657654512LL);

    Info info[1];
    int gameInd = 1, totalFens = 0;

    Search* searcher = new Search();

    info->timeset = false;
    info->depth = 9;
    info->startTime = getTime();
    searcher->setTime(info);

    std::mutex M;

    searcher->TT = new tt::HashTable();
    searcher->principalSearcher = true;

    while (totalFens < nrFens) {
        FenData data;
        std::vector <FenData> fens;
        double result = 0;
        int ply = 0;

        int winCnt = 0, drawCnt = 0;

        searcher->_setFen(START_POS_FEN);

        searcher->clearHistory();
        searcher->clearKillers();
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
                if (inCheck(searcher->board)) {
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

                if (nrMoves == 1) { /// in this case, engine reports score 0, which might be misleading
                    searcher->_makeMove(move);
                    ply++;
                    continue;
                }

                searcher->flag = 0;

                if (!inCheck(searcher->board) && searcher->quiesce(-INF, INF, false) == searcher->Stack[0].eval) { /// relatively quiet position
                    data.fen = searcher->board.fen();
                    data.score = score;
                    fens.push_back(data);
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

        for (auto& fen : fens) {
            out << fen.fen << " [" << result << "] " << fen.score << "\n";
        }


        M.lock();

        sumFens = sumFens + (int)fens.size();
        totalFens += fens.size();
        M.unlock();

        gameInd++;
    }

    /*for(int i = 0; i < 1000; i++)
      std::cout << (cntGames[i] ? 1.0 * cntNodes[i] / cntGames[i] : 0) << " average nodes for ply " << i << "\n";*/
}

void generateData(int nrFens, int nrThreads, std::string rootPath) {
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

    std::random_device rd;
    std::atomic <int> sumFens{ 0 };
    double startTime = getTime();

    for (auto& t : threads) {
        std::string pth = path[i];
        std::cout << "Starting thread " << i << std::endl;
        t = std::thread{ generateFens, std::ref(sumFens), batch, pth, rd() };
        i++;
    }

    while (sumFens <= nrFens) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Fens " << sumFens << " ; Time elapsed: " << (getTime() - startTime) / 1000.0 << "s\r";
    }

    for (auto& t : threads)
        t.join();
}

