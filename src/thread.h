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

int seeVal[] = { 0, 93, 308, 346, 521, 994, 20000 };

int nmpR = 4;
int nmpDepthDiv = 4;
int nmpEvalDiv = 135;

int SNMPDepth = 12;
int SNMPCoef1 = 84;
int SNMPCoef2 = 19;

int seeCoefQuiet = 71;
int seeCoefNoisy = 10;
int seeDepthCoef = 15;

int probcutDepth = 10;
int probcutMargin = 100;
int probcutR = 3;

int fpMargin = 100;
int fpCoef = 99;

int histDiv = 6928;

int chCoef = -2000;
int fhCoef = -2000;

int seePruningQuietDepth = 8;
int seePruningNoisyDepth = 8;
int lmpDepth = 8;

int nodesSearchedDiv = 10000;

int lmrMargin = 12;
int lmrDiv = 24;
int lmrCapDiv = 17;

int tmScoreDiv = 111;
int tmBestMoveStep = 50;
int tmBestMoveMax = 1250;
int tmNodesSearchedMaxPercentage = 1570;

int quiesceFutilityCoef = 203;

int aspirationWindow = 8;

int pawnAttackedCoef = 36;
int pawnPushBonus = 9520;
int kingAttackBonus = 3579;

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
                lmrRed[i][j] = 1.0 * lmrMargin / 10 + log(i) * log(j) / (1.0 * lmrDiv / 10);
                lmrRedNoisy[i][j] = lmrRed[i][j] / (1.0 * lmrCapDiv / 10);
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
    void clearHistory();
    void clearStack();
    void clearBoard();
    void setThreadCount(int nrThreads);
    void startPrincipalSearch(Info* info);
    void stopPrincipalSearch();
    void isReady();

    void _setFen(std::string fen, bool chess960 = false);
    void _setDFRC(int idx);
    void _makeMove(uint16_t move);

    std::pair <int, uint16_t> startSearch(Info* info);

    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack); /// for quiet position check (tuning)

    template <bool pvNode>
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack);

    int rootSearch(int alpha, int beta, int depth, int multipv, StackEntry* stack);

    void setTime(Info* tInfo) { info = tInfo; }

    void startWorkerThreads(Info* info);
    void flagWorkersStop();
    void stopWorkerThreads();
    void lazySMPSearcher();
    void releaseThreads();
    void killMainThread();

    void printPv();
    void updatePv(int ply, int move);

    bool checkForStop();

    uint64_t nodesSearched[2][64 * 64];
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