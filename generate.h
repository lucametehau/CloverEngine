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
#include "search.h"

/*struct FenData {
  int score;
  std::string fen;
};

void generateFens(std::atomic <int> &sumFens, int id, int nrFens, std::string path, uint64_t seed) {
  std::ofstream out (path);
  std::mt19937_64 gn(seed);

  Info info[1];
  int gameInd = 1, totalFens = 0;
  double startTime = getTime();

  Search *searcher = new Search();

  info->timeset = false;
  info->depth = 9;
  info->startTime = getTime();
  searcher->setTime(info);

  std::mutex M;

  searcher->TT = new tt :: HashTable();
  searcher->principalSearcher = true;

  while(totalFens < nrFens) {
    FenData data;
    std::vector <FenData> fens;
    double result = 0;
    int ply = 0;
    //int resignCnt = 0, drawCnt = 0;

    searcher->_setFen(START_POS_FEN);

    searcher->clearHistory();
    searcher->clearKillers();
    searcher->clearStack();

    searcher->TT->initTable(32 * MB);
    searcher->TT->resetAge();

    while(true) {
      uint16_t move;
      int score;

      /// game over checking

      if(isRepetition(searcher->board, 0) || searcher->board.halfMoves >= 100 || searcher->board.isMaterialDraw()) {
        result = 0.5;
        break;
      }

      uint16_t moves[256];

      int nrMoves = genLegal(searcher->board, moves);

      if(!nrMoves) {
        if(inCheck(searcher->board)) {
          result = (searcher->board.turn == WHITE ? 0.0 : 1.0);
        } else {
          result = 0.5;
        }

        break;
      }

      if(ply < 8) { /// simulating a book ?
        std::uniform_int_distribution <uint32_t> rnd(0, nrMoves - 1);
        move = moves[rnd(gn)];
        searcher->_makeMove(move);
      } else {
        searcher->TT->age();
        searcher->clearBoard();

        //searcher->board.print();
        std::pair <int, uint16_t> res = searcher->startSearch(info);
        score = res.first, move = res.second;

        if(abs(score) <= 1500) {

          searcher->flag = 0;

          if(!inCheck(searcher->board) && searcher->quiesce(-INF, INF, false) == searcher->Stack[0].eval) { /// relatively quiet position
            data.fen = searcher->board.fen();
            data.score = score;
            fens.push_back(data);
          }
        } else {

          score *= (searcher->board.turn == WHITE ? 1 : -1);

          result = (score < 0 ? 0.0 : 1.0);

          break;
        }

        searcher->_makeMove(move);

      }

      ply++;
    }

    for(auto &fen : fens) {
      out << fen.fen << " [" << result << "] " << fen.score << "\n";
    }


    M.lock();

    sumFens = sumFens + (int)fens.size();
    totalFens += fens.size();
    M.unlock();

    gameInd++;
  }
}*/

void generateData(int nrFens, int nrThreads, std::string rootPath) {
  std::string path[100];

  srand(time(0));

  for(int i = 0; i < nrThreads; i++) {
    path[i] = rootPath;
    path[i] += char(i + '0');
    path[i] += ".txt";
    std::cout << path[i] << "\n";
  }

  /*std::vector <std::thread> threads(nrThreads);
  int batch = nrFens / nrThreads, i = 0;

  std::cout << batch << "\n";

  std::random_device rd;
  std::atomic <int> sumFens {0};
  double startTime = getTime();

  for(auto &t : threads) {
    std::string pth = path[i];
    std::cout << "Starting thread " << i << std::endl;
    t = std::thread{ generateFens, std::ref(sumFens), i, batch, pth, rd() };
    i++;
  }

  while(sumFens <= nrFens) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Generated " << sumFens << " ; Time elapsed: " << (getTime() - startTime) / 1000.0 << "s" << "\r";
  }

  for(auto &t : threads)
    t.join();*/
}

