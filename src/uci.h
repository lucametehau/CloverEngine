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
#include <string>
#include <fstream>
#include <sstream>
#include "move.h"
#include "search.h"
#include "Fathom/src/tbprobe.h"
#include "init.h"
#include "perft.h"
#include "generate.h"

const std::string VERSION = "6.1.23";

class UCI {
public:
    UCI(Search& _searcher) : searcher(_searcher) {}
    ~UCI() {}

public:
    void uciLoop();
    void Bench(int depth);

private:

    void Uci();
    void UciNewGame(uint64_t ttSize);
    void IsReady();
    void Go(Info* info);
    void Stop();
    void Position();
    void SetOption();
    void Eval();
    void Perft(int depth);
    void addOption(std::string name, int value);
    void setParameterI(std::istringstream& iss, int& value);

public:
    Search& searcher;
};

void UCI::setParameterI(std::istringstream& iss, int& value) {
    std::string valuestr;
    iss >> valuestr;

    int newValue;
    iss >> newValue;
    value = newValue;
}

void UCI::uciLoop() {

    uint64_t ttSize = 8;

    std::cout << "Clover " << VERSION << " by Luca Metehau" << std::endl;

#ifndef GENERATE
    TT = new HashTable();
#endif

    Info info[1];

    init(info);
    searcher.info = info;

    UciNewGame(ttSize);
    std::string input;

    info->ponder = false;

    while (getline(std::cin, input)) {

        std::istringstream iss(input);

        std::string cmd;

        iss >> std::skipws >> cmd;

        if (cmd == "isready") {
            IsReady();
        }
        else if (cmd == "position") {
            std::string type;

            while (iss >> type) {
                if (type == "startpos") {
                    searcher.set_fen(START_POS_FEN);
                }
                else if (type == "fen") {
                    std::string fen;

                    for (int i = 0; i < 6; i++) {
                        std::string component;
                        iss >> component;
                        fen += component + " ";
                    }

                    searcher.set_fen(fen, info->chess960);
                }
                else if (type == "moves") {
                    std::string moveStr;

                    while (iss >> moveStr) {
                        int move = parse_move_string(searcher.board, moveStr, info);

                        searcher.make_move(move);
                    }
                }
            }

            //searcher.board.print(); /// just to be sure
        }
        else if (cmd == "ucinewgame") {
            UciNewGame(ttSize);
        }
        else if (cmd == "go") {
            int depth = -1, movestogo = 40, movetime = -1;
            int time = -1, inc = 0;
            int64_t nodes = -1;
            bool turn = searcher.board.turn;
            info->timeset = 0;
            info->sanMode = 0;

            std::string param;

            while (iss >> param) {
                if (param == "infinite") {
                    ;
                }
                else if (param == "binc" && turn == BLACK) {
                    iss >> inc;
                }
                else if (param == "winc" && turn == WHITE) {
                    iss >> inc;
                }
                else if (param == "wtime" && turn == WHITE) {
                    iss >> time;
                }
                else if (param == "btime" && turn == BLACK) {
                    iss >> time;
                }
                else if (param == "movestogo") {
                    iss >> movestogo;
                }
                else if (param == "movetime") {
                    iss >> movetime;
                }
                else if (param == "depth") {
                    iss >> depth;
                }
                else if (param == "nodes") {
                    iss >> nodes;
                }
                else if (param == "san") {
                    info->sanMode = true;
                }
                else if (param == "ponder") {
                    info->ponder = true;
                }
            }

            info->startTime = getTime();
            info->depth = depth;

            if (movetime != -1) {
                time = movetime;
                info->timeset = 1;
                info->goodTimeLim = info->hardTimeLim = time;
                info->stopTime = info->startTime + info->goodTimeLim;
            }
            else if (time != -1) {
                const int MOVE_OVERHEAD = 100; // idk
                time -= MOVE_OVERHEAD;
                int incTime = time + movestogo * inc;
                int goodTimeLim, hardTimeLim;
                goodTimeLim = std::min<int>(incTime / std::max(movestogo / 2, 1), time * 0.35);
                hardTimeLim = std::min<int>(goodTimeLim * 5.5, time * 0.75);
                info->goodTimeLim = goodTimeLim;
                info->hardTimeLim = hardTimeLim;
                info->timeset = 1;
                info->stopTime = info->startTime + goodTimeLim;
            }

            info->nodes = nodes;
            if (depth == -1)
                info->depth = DEPTH;

            Go(info);
        }
        else if (cmd == "ponderhit") {
            searcher.info->ponder = false;
        }
        else if (cmd == "quit") {
            Stop();
            searcher.stop_workers();
            searcher.kill_principal_search();
            return;
        }
        else if (cmd == "stop") {
            Stop();
        }
        else if (cmd == "uci") {
            Uci();
        }
        else if (cmd == "checkmove") {
            std::string moveStr;
            iss >> moveStr;
            uint16_t move = parse_move_string(searcher.board, moveStr, info);
            std::cout << is_legal(searcher.board, move) << " " << is_legal_slow(searcher.board, move) << "\n";
        }
        else if (cmd == "printparams") {
#ifdef TUNE_FLAG
            print_params_for_ob();
#endif
        }
        else if (cmd == "setoption") {
            std::string name, value;

            iss >> name;

            if (name == "name")
                iss >> name;

            if (name == "Hash") {
                iss >> value;

                iss >> ttSize;
#ifndef GENERATE
                TT->initTable(ttSize * MB, searcher.threadCount + 1);
#endif
            }
            else if (name == "Threads") {
                int nrThreads;

                iss >> value;

                iss >> nrThreads;

                searcher.set_num_threads(nrThreads - 1);
                UciNewGame(ttSize);
            }
            else if (name == "SyzygyPath") {
                std::string path;

                iss >> value;

                iss >> path;

                tb_init(path.c_str());
            }
            else if (name == "MultiPV") {
                iss >> value;

                int multipv;

                iss >> multipv;

                info->multipv = multipv;
            }
            else if (name == "UCI_Chess960") {
                iss >> value;

                std::string truth;

                iss >> truth;

                info->chess960 = (truth == "true");
            }
            else {
                for (auto& param : params) {
                    if (name == param.name) {
                        setParameterI(iss, param.value);
                        break;
                    }
                }
            }
        }
        else if (cmd == "generate") {
            int nrThreads, nrFens;
            std::string path;

            iss >> nrFens >> nrThreads >> path;

            generateData(nrFens, nrThreads, path);
        }
        else if (cmd == "show") {
            searcher.board.print();
        }
        else if (cmd == "eval") {
            Eval();
        }
        else if (cmd == "perft") {
            int depth;

            iss >> depth;

            Perft(depth);
        }
        else if (cmd == "bench") {
            Bench(-1);
        }
        else if (cmd == "evalbench") {
            int eval = evaluate(searcher.board);
            uint64_t total = 0;
            const int N = (int)1e8;
            for (int i = 0; i < N; i++) {
                auto start = std::chrono::high_resolution_clock::now();
                eval = evaluate(searcher.board);
                auto end = std::chrono::high_resolution_clock::now();
                total += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            }

            std::cout << eval << " evaluation and " << total / N << "ns\n";
        }
        else if (cmd == "see") {
            std::string mv;
            int th;
            iss >> mv >> th;

            uint16_t move = parse_move_string(searcher.board, mv, info);

            std::cout << see(searcher.board, move, th) << std::endl;
        }

        if (info->quit)
            break;
    }
}

void UCI::Uci() {
    std::cout << "id name Clover " << VERSION << std::endl;
    std::cout << "id author Luca Metehau" << std::endl;
    std::cout << "option name Hash type spin default 8 min 2 max 131072" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
    std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
    std::cout << "option name MultiPV type spin default 1 min 1 max 255" << std::endl;
    std::cout << "option name UCI_Chess960 type check default false" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
    for (auto& param : params)
        std::cout << "option name " << param.name << " type spin default " << param.value << " min " << param.min << " max " << param.max << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::UciNewGame(uint64_t ttSize) {
    searcher.set_fen(START_POS_FEN);

    searcher.clear_history();
    searcher.clear_stack();

#ifndef GENERATE
    TT->resetAge();
    TT->initTable(ttSize * MB, searcher.threadCount + 1);
#endif
}

void UCI::Go(Info* info) {
#ifndef GENERATE
    TT->age();
#endif
    searcher.clear_stack();
    searcher.clear_board();
    searcher.start_principal_search(info);
}

void UCI::Stop() {
    searcher.stop_principal_search();
}

void UCI::Eval() {
    std::cout << evaluate(searcher.board) << std::endl;
}

void UCI::IsReady() {
    searcher.isReady();
}

void UCI::Perft(int depth) {
    long double t1 = getTime();
    uint64_t nodes = perft(searcher.board, depth, 1);
    long double t2 = getTime();
    long double t = (t2 - t1) / 1000.0;
    uint64_t nps = nodes / t;

    std::cout << "nodes: " << nodes << std::endl;
    std::cout << "time : " << t << std::endl;
    std::cout << "nps  : " << nps << std::endl;
}

/// positions used for benching

std::string benchPos[] = {
  "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14   ",
  "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24     ",
  "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42        ",
  "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44  ",
  "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54      ",
  "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36     ",
  "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10      ",
  "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87      ",
  "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42       ",
  "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37  ",
  "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34     ",
  "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37      ",
  "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17       ",
  "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42      ",
  "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29        ",
  "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68  ",
  "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20   ",
  "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49    ",
  "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51  ",
  "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34        ",
  "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22    ",
  "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5      ",
  "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12 ",
  "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19       ",
  "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25        ",
  "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32  ",
  "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5    ",
  "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12      ",
  "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20      ",
  "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27 ",
  "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33 ",
  "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38  ",
  "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45       ",
  "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49        ",
  "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54   ",
  "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59   ",
  "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63   ",
  "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67     ",
  "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23   ",
  "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22        ",
  "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12 ",
  "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55  ",
  "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44  ",
  "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25      ",
  "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14    ",
  "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36 ",
  "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2        ",
  "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20     ",
  "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23       ",
  "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93",
};

/// IMPORTANT NOTICE: AFTER RUNNING bench, RUN ucinewgame AGAIN (to fix)
/// why? because tt is initialized with 16MB when bench is run

void UCI::Bench(int depth) {
    Info info[1];

#ifndef GENERATE
    TT = new HashTable();
#endif

    init(info);

    uint64_t ttSize = 16;

    UciNewGame(ttSize);

    searcher.principalSearcher = true;

    printStats = false;

    /// do fixed depth searches for some positions

    uint64_t start = getTime();
    uint64_t totalNodes = 0;

    for (auto& fen : benchPos) {
        searcher.set_fen(fen);

        //std::cout << fen << "\n";

        info->timeset = 0;
        info->depth = (depth == -1 ? 12 : depth);
        info->startTime = getTime();
        info->nodes = -1;
        searcher.start_search(info);
        totalNodes += searcher.nodes;

        UciNewGame(ttSize);
    }

    //std::cout << "};\n";

    uint64_t end = getTime();
    long double t = 1.0 * (end - start) / 1000.0;

    printStats = true;

    std::cout << totalNodes << " nodes " << int(totalNodes / t) << " nps" << std::endl;
}