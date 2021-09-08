#pragma once
#include <fstream>
#include <vector>
#include <thread>
#include <ctime>
#include "search.h"

struct FenData {
  int score;
  std::string fen;
};

void generateFens(int id, int nrFens, std::string path) {
  std::ofstream out (path);
  std::mt19937_64 gn(getTime() * id + 74628599LL);
  std::uniform_int_distribution <uint32_t> rnd;

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

      uint16_t moves[256];

      int nrMoves = genLegal(searcher->board, moves);

      /// game over checking

      if(isRepetition(searcher->board, 0) || searcher->board.halfMoves >= 100 || searcher->board.isMaterialDraw()) {
        result = 0.5;
        break;
      }

      if(!nrMoves) {
        if(inCheck(searcher->board)) {
          result = (searcher->board.turn == WHITE ? 0.0 : 1.0);
        } else {
          result = 0.5;
        }

        break;
      }

      if(ply < 8) { /// simulating a book ?
        move = moves[rnd(gn) % nrMoves];
        searcher->_makeMove(move);
      } else {
        searcher->TT->age();
        searcher->clearBoard();

        //searcher->board.print();
        std::pair <int, uint16_t> res = searcher->startSearch(info);
        score = res.first, move = res.second;

        if(abs(score) <= 1500) {
          data.fen = searcher->board.fen();
          data.score = score;

          searcher->flag = 0;

          if(!inCheck(searcher->board) && searcher->quiesce(-INF, INF, false) == evaluate(searcher->board)) /// relatively quiet position
            fens.push_back(data);
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

    totalFens += fens.size();


    M.lock();
    std::cout << "Done game #" << gameInd << ", generated " << fens.size() << " fens, " << totalFens << " fens in total for thread #" << id << std::endl;
    std::cout << "Time elapsed: " << (getTime() - startTime) / 1000.0 << "s" << std::endl;
    M.unlock();

    gameInd++;
  }
}

void generateData(int nrFens, int nrThreads) {
  std::string path[10]; /// don't think I will use more than 8 threads

  srand(time(0));

  for(int i = 0; i < nrThreads; i++) {
    path[i] = "CloverData7.";
    path[i] += char(i + '0');
    path[i] += ".txt";
    std::cout << path[i] << "\n";
  }

  std::vector <std::thread> threads(nrThreads);
  int batch = nrFens / nrThreads, i = 0;

  std::cout << batch << "\n";

  for(auto &t : threads) {
    std::string pth = path[i];
    std::cout << "Starting thread " << i << std::endl;
    t = std::thread{ generateFens, i, batch, pth };
    i++;
  }

  for(auto &t : threads)
    t.join();
}

