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
#include "board.h"
#include "tt.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/// search params

int seeVal[] = { 0, SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000 };

const int TERMINATED_BY_USER = 1;
const int TERMINATED_BY_LIMITS = 2;
const int TERMINATED_SEARCH = TERMINATED_BY_USER | TERMINATED_BY_LIMITS;

struct Search {
    Search() : threads(nullptr), params(nullptr) {
        threadCount = flag = checkCount = 0;
        principalSearcher = terminateSMP = SMPThreadExit = false;
        lazyFlag = false;

        for (int i = 0; i < 64; i++) { /// depth
            for (int j = 0; j < 64; j++) { /// moves played 
                lmrRed[i][j] = LMRQuietBias / 100.0 + log(i) * log(j) / (LMRQuietDiv / 100.0);
                lmrRedNoisy[i][j] = LMRNoisyBias / 100.0 + lmrRed[i][j] / (LMRNoisyDiv / 100.0);
            }
        }
    }

    ~Search() {
        //std::cout << "its not even called\n";
        releaseThreads();
    }

    Search(const Search&) = delete;
    Search& operator = (const Search&) = delete;

    void initSearch();
    void clearForSearch();
    void clearKillers();
    void clear_history();
    void clear_stack();
    void clear_board();
    void set_num_threads(int nrThreads);
    void start_principal_search(Info* info);
    void stop_principal_search();
    void isReady();

    void set_fen(std::string fen, bool chess960 = false);
    void set_dfrc(int idx);
    void make_move(uint16_t move);

    std::pair <int, uint16_t> start_search(Info* info);

    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack); /// for quiet position check (tuning)

    template <bool rootNode, bool pvNode>
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack);

    void setTime(Info* tInfo) { info = tInfo; }

    void start_workers(Info* info);
    void flag_workers_stop();
    void stop_workers();
    void lazySMPSearcher();
    void releaseThreads();
    void kill_principal_search();

    void printPv();
    void update_pv(int ply, int move);

    template <bool checkTime>
    bool checkForStop();

    uint64_t nodesSearched[64 * 64];
    uint16_t pvTable[DEPTH + 5][2 * DEPTH + 5];
    int pvTableLen[DEPTH + 5];
    uint16_t cmTable[13][64];
    int16_t hist[2][2][2][64 * 64];
    TablePieceTo continuationHistory[2][13][64];
    int16_t capHist[13][64][7];
    int lmrRed[64][64], lmrRedNoisy[64][64];
    int bestMoves[256], ponderMoves[256], scores[256], rootScores[256];
    MeanValue values[10];

    volatile int flag;

    uint64_t tbHits;
    int64_t t0;
    Info* info;
    int checkCount;

    int64_t cnt, cnt2;
    int bestMoveCnt;
    int multipv_index;

    bool lastNullMove;

    int threadCount;
    int tDepth, completedDepth, selDepth;

    int rootEval, rootDepth;

    std::unique_ptr <std::thread> principalThread;
    std::mutex readyMutex;

    uint64_t nodes, qsNodes;
    bool principalSearcher;
    Board board;

#ifdef GENERATE
    HashTable* TT;
#endif

    std::unique_ptr <std::thread[]> threads;
    std::unique_ptr <Search[]> params;
    std::condition_variable lazyCV;
    volatile bool lazyFlag;
    volatile bool SMPThreadExit;

    bool isLazySMP() {
        return lazyFlag;
    }

    void resetLazySMP() {
        lazyFlag = 0;
    }

    bool terminateSMP;

};