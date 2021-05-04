#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "move.h"
#include "search.h"
#include "tune.h"
#include "tbcore.h"
#include "init.h"
#include "perft.h"

#define INPUTBUFFER 6000

/// 2.1 seems to be 85 elo stronger than 2.0
/// TO DO - (fix multithreading scaling) -> 8 threads ... 4.5 (for now)
///       - tune on lichess-quiet.txt
///       - add kp-hash
///       - improve evaluation -> improve king safety, add more terms (idk)
///       - improve time management -> currently, it's something similar to vice :)

/// is this working?
/// i guess so?

const std::string VERSION = "2.1.3"; /// 2.0 was "FM"

char line[INPUTBUFFER];

class UCI {
  public:
    UCI(Search &_searcher) : searcher(_searcher) {}
    ~UCI() {}

  public:
    void Uci_Loop();

  private:
    void Uci();
    void UciNewGame(uint64_t ttSize);
    void IsReady();
    void Go(Info *info);
    void Stop();
    void Position();
    void SetOption();
    void Eval();
    void Tune(int nrThreads, std::string path);
    void Perft(int depth);
    void Bench(std::string path);

  private:
    Search &searcher;
};

void UCI :: Uci_Loop() {

    uint64_t ttSize = 128;

    std::cout << "Clover " << VERSION << " by Luca Metehau" << std::endl;

    TT = new tt :: HashTable();

    Info info[1];

    init(info);

    //pvMove.clear();
    //table.clear();

    //searcher.setThreadCount(1); /// 2 threads for debugging data races
    UciNewGame(ttSize);

	  while (1) {
      std::string input;
      getline(std::cin, input);

      std::istringstream iss(input);

      {

          std::string cmd;

          iss >> std::skipws >> cmd;

          if (cmd == "isready") {

            IsReady();

          } else if (cmd == "position") {

            std::string type;

            while(iss >> type) {

              if(type == "startpos") {

                searcher._setFen(START_POS_FEN);

              } else if(type == "fen") {

                std::string fen;

                for(int i = 0; i < 6; i++) {
                  std::string component;
                  iss >> component;
                  fen += component + " ";
                }

                searcher._setFen(fen);

              } else if(type == "moves") {

                std::string movestr;

                while(iss >> movestr) {

                  int move = ParseMove(searcher.board, movestr);

                  searcher._makeMove(move);
                }

              }
            }

            searcher.board.print(); /// just to be sure

          } else if (cmd == "ucinewgame") {

            UciNewGame(ttSize);

          } else if (cmd == "go") {

                int depth = -1, movestogo = 30, movetime = -1;
                int time = -1, inc = 0;
                bool turn = searcher.board.turn;
                info->timeset = 0;

                std::string param;

                ///

                while(iss >> param) {

                    if(param == "infinite") {
                        ;
                    } else if(param == "binc" && turn == BLACK) {

                      iss >> inc;

                    } else if(param == "winc" && turn == WHITE) {

                      iss >> inc;

                    } else if(param == "wtime" && turn == WHITE) {

                      iss >> time;

                    } else if(param == "btime" && turn == BLACK) {

                      iss >> time;

                    } else if(param == "movestogo") {

                      iss >> movestogo;

                    } else if(param == "movetime") {

                      iss >> movetime;

                    } else if(param == "depth") {

                      iss >> depth;

                    }

                }

                if(movetime != -1) {
                  time = movetime;
                  movestogo = 60;
                }

                info->startTime = getTime();
                info->depth = depth;

                int goodTimeLim, hardTimeLim;

                if(time != -1) {

                    goodTimeLim = time / (movestogo + 1) + inc;
                    hardTimeLim = std::min(goodTimeLim * 8, time / std::min(4, movestogo));

                    hardTimeLim = std::max(10, std::min(hardTimeLim, time));
                    goodTimeLim = std::max(1, std::min(hardTimeLim, goodTimeLim));
                    info->timeset = 1;
                    info->stopTime = info->startTime + goodTimeLim;
                }

                if(depth == -1)
                  info->depth = DEPTH;

                //printf("time:%lld start:%lf stop:%lf depth:%d timeset:%d\n",time,info->startTime,info->stopTime,info->depth,info->timeset);

                Go(info);

          } else if (cmd == "quit") {

            return;

          } else if(cmd == "stop") {

            Stop();

          } else if (cmd == "uci") {

            Uci();

          } else if(cmd == "setoption") {

            std::string name, value;

            iss >> name;

            if(name == "name")
              iss >> name;

            if(name == "Hash") {

              iss >> value;

              iss >> ttSize;

              TT->initTable(ttSize * MB);

            } else if(name == "Threads") {
              int nrThreads;

              iss >> value;

              iss >> nrThreads;

              searcher.setThreadCount(nrThreads - 1);
              UciNewGame(ttSize);

            } else if(name == "SyzygyPath") {
              std::string path;

              iss >> value;

              iss >> path;

              tb_init(path.c_str());

              //cout << "Set TB path to " << path << "\n";
            }

            /// search params

            if(name == "RazorCoef") {
              iss >> value;

              int val;

              iss >> val;
              RazorCoef = val;
            } else if(name == "StaticNullCoef") {
              iss >> value;

              int val;

              iss >> val;
              StaticNullCoef = val;
            }

          } else if(cmd == "tune") {

            int nrThreads;
            std::string path;

            iss >> nrThreads;

            iss >> path;

            Tune(nrThreads, path);

          } else if(cmd == "eval") {

            Eval();

          } else if(cmd == "perft") {

            int depth;

            iss >> depth;

            Perft(depth);

          } else if(cmd == "bench") {

            std::string path;

            iss >> path;

            Bench(path);
          }
          if(info->quit)
            break;
    }


  }
}

void UCI :: Uci() {
  std::cout << "id name Clover " << VERSION << std::endl;
  std::cout << "id author Luca Metehau" << std::endl;
  std::cout << "option name Hash type spin default 128 min 2 max 131072" << std::endl;
  std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
  std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
  std::cout << "uciok" << std::endl;
}

void UCI :: UciNewGame(uint64_t ttSize) {
  searcher._setFen(START_POS_FEN);

  searcher.clearHistory();
  searcher.clearKillers();
  searcher.clearStack();

  TT->initTable(ttSize * MB);
  TT->resetAge();
}

void UCI :: Go(Info *info) {
  TT->age();
  searcher.clearBoard();
  searcher.startPrincipalSearch(info);
}

void UCI :: Stop() {
  searcher.stopPrincipalSearch();
}

void UCI :: Eval() {
  std::cout << evaluate(searcher.board) << std::endl;
}

void UCI :: Tune(int nrThreads, std::string path) {
  TUNE = true;
  tune(nrThreads, path);
}

void UCI :: IsReady() {
  searcher.isReady();
}

void UCI :: Perft(int depth) {
  long double t1 = getTime();
  uint64_t nodes = perft(searcher.board, depth);
  long double t2 = getTime();
  long double t = (t2 - t1) / 1000.0;
  uint64_t nps = nodes / t;

  std::cout << "nodes: " << nodes << std::endl;
  std::cout << "time : " << t << std::endl;
  std::cout << "nps  : " << nps << std::endl;
}

void UCI :: Bench(std::string path) {
  std::ifstream in (path);
  std::string fen;
  Info info[1];

  init(info);

  uint64_t ttSize = 16;

  UciNewGame(ttSize);

  printStats = false;

  searcher.principalSearcher = 1;

  /// do fixed depth searches for some positions

  uint64_t start = getTime();
  uint64_t totalNodes = 0;

  while(getline(in, fen)) {
    //std::cout << fen << "\n";
    searcher._setFen(fen);
    //searcher.board.print();
    info->timeset = 0;
    info->depth = 11;
    info->startTime = getTime();
    searcher.startSearch(info);
    totalNodes += searcher.nodes;

    UciNewGame(ttSize);
  }

  uint64_t end = getTime();
  long double t = 1.0 * (end - start) / 1000.0;

  printStats = true;

  std::cout << "nodes: " << totalNodes << "\n";
  std::cout << " time: " << t << "\n";
  std::cout << "  nps: " << int(totalNodes / t) << "\n";
}

