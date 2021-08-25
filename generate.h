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
  Info info[1];
  Board board;
  int gameInd = 1, totalFens = 0;
  double startTime = getTime();

  Search *searcher = new Search();

  board.setFen(START_POS_FEN);

  info->timeset = false;
  info->depth = 8;
  info->startTime = 0;
  searcher->setTime(info);

  std::mutex M;

  searcher->TT = new tt :: HashTable();

  while(totalFens < nrFens) {
    FenData data;
    std::vector <FenData> fens;
    double result = 0;
    int ply = 0;

    board.setFen(START_POS_FEN);
    searcher->_setFen(START_POS_FEN);

    searcher->clearHistory();
    searcher->clearKillers();
    searcher->clearStack();

    searcher->TT->initTable(128 * MB);
    searcher->TT->resetAge();

    while(true) {
      uint16_t move;
      int score;

      uint16_t moves[256];

      int nrMoves = genLegal(board, moves);

      /// game over checking

      if(isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw()) {
        result = 0.5;
        break;
      }

      if(!nrMoves) {
        if(inCheck(board)) {
          result = (board.turn == WHITE ? 0.0 : 1.0);
        } else {
          result = 0.5;
        }

        break;
      }

      if(ply < 8) { /// simulating a book ?
        move = moves[rng(gen) % nrMoves];
        makeMove(board, move);
        searcher->_makeMove(move);
      } else {
        searcher->TT->age();
        searcher->clearBoard();
        board.clear();

        score = searcher->search(-INF, INF, 8);

        tt::Entry entry = {};

        move = searcher->pvTable[0][0];

        if(abs(score) <= 1500) {
          data.fen = board.fen();
          data.score = score;

          //board.print();
          //std::cout << evaluate(board) << " " << searcher->quiesce(-INF, INF, false) << "\n";

          if(!inCheck(board) && searcher->quiesce(-INF, INF, false) == evaluate(board)) /// relatively quiet position
            fens.push_back(data);
        }

        makeMove(board, move);
        searcher->_makeMove(move);

      }

      ply++;

      //board.print();
      //std::cout << toString(move) << " " << ply << "\n";
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

void filterData(std::string path) {
  freopen (path.c_str(), "r", stdin);
  std::ofstream out ("temp.txt");
  Board board;
  Search *searcher = new Search();
  char fen[105], c, stm, a[15];
  double gameRes;
  int eval;
  int total = 0, nrFens = 0;

  Info info[1];
  info->timeset = false;
  info->depth = 8;
  info->startTime = 0;
  searcher->setTime(info);


  searcher->_setFen(START_POS_FEN);

  searcher->clearHistory();
  searcher->clearKillers();
  searcher->clearStack();

  searcher->TT = new tt :: HashTable();
  searcher->TT->initTable(128 * MB);
  searcher->TT->resetAge();

  while(true) {
    if(feof(stdin)) {
      std::cout << total << " fens out of " << nrFens << "\n";
      break;
    }

    nrFens++;
    scanf("%s", fen);

    scanf("%c", &c);
    scanf("%c", &stm);

    scanf("%s", a); scanf("%s", a); scanf("%s", a); scanf("%s", a);

    scanf("%c", &c);
    scanf("%c", &c);
    scanf("%lf", &gameRes);
    scanf("%c", &c);
    scanf("%d", &eval);

    std::cout << "! 2 " << fen << " " << total << "\n";

    std::string str(fen);

    std::cout << str << " XD\n";
    std::cout << str << " XD\n";

    searcher->clearBoard();
    searcher->_setFen(str);
    std::cout << str << " XD\n";

    if(!inCheck(searcher->board) && searcher->quiesce(-INF, INF) == evaluate(searcher->board))
      out << searcher->board.fen() << " " << gameRes << " " << eval << "\n", total++;
  }
}

void generateData(int nrFens, int nrThreads) {
  std::string path[10]; /// don't think I will use more than 8 threads

  srand(time(0));

  for(int i = 0; i < nrThreads; i++) {
    path[i] = "CloverData2.";
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

