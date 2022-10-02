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
int nmpDepthDiv = 6;
int nmpEvalDiv = 130;

int RazorCoef = 381;

int SNMPCoef1 = 97;
int SNMPCoef2 = 25;

int seeCoefQuiet = 71;
int seeCoefNoisy = 10;
int seeDepthCoef = 15;

int probcutDepth = 10;
int probcutMargin = 100;
int probcutR = 3;

int fpMargin = 100;
int fpCoef = 103;

int histDiv = 4766;

int chCoef = -2000;
int fhCoef = -2000;

int fpHistDiv = 512;

int nodesSearchedDiv = 10000;

int lmrMargin = 10;
int lmrDiv = 25;
int lmrCapDiv = 15;

int lmpStart1 = 3, lmpMult1 = 1, lmpDiv1 = 2;
int lmpStart2 = 3, lmpMult2 = 1, lmpDiv2 = 1;

int tmScoreDiv = 113;
int tmBestMoveStep = 188;
int tmBestMoveMax = 2173;
int tmNodesSearchedMaxPercentage = 1441;

int quiesceFutilityCoef = 200;

const int TERMINATED_BY_USER = 1;
const int TERMINATED_BY_TIME = 2;
const int TERMINATED_SEARCH = 3; /// 1 | 2

class Search {

    //friend class Movepick;
    //friend class History;

public:
    Search();
    ~Search();
    Search(const Search&) = delete;
    Search& operator = (const Search&) = delete;

public:
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

    void _setFen(std::string fen);
    void _makeMove(uint16_t move);

    std::pair <int, uint16_t> startSearch(Info* info);
    int quiesce(int alpha, int beta, bool useTT = true); /// for quiet position check (tuning)
    int search(int alpha, int beta, int depth, bool cutNode, uint16_t excluded = NULLMOVE);
    int rootSearch(int alpha, int beta, int depth, int multipv);

    void setTime(Info* tInfo) { info = tInfo; }

    void startWorkerThreads(Info* info);
    void flagWorkersStop();
    void stopWorkerThreads();
    void lazySMPSearcher();
    void releaseThreads();

    void printPv();
    void updatePv(int ply, int move);

    bool checkForStop();

    uint64_t nodesSearched[64][64];
    uint16_t pvTable[DEPTH + 5][DEPTH + 5];
    int pvTableLen[DEPTH + 5];
    uint16_t killers[DEPTH + 5][2];
    uint16_t cmTable[2][13][64];
    int hist[2][64][64];
    int follow[2][13][64][13][64];
    int capHist[13][64][7];
    int lmrCnt[2][9];
    int lmrRed[64][64];
    StackEntry Stack[DEPTH + 5];
    int bestMoves[256], scores[256];

    int kekw[500];

    int16_t contempt;

    volatile int flag;

    uint64_t tbHits;
    uint64_t t0;
    Info* info;
    int checkCount;

    uint64_t nmpFail, nmpTries;
    int64_t cnt, cnt2;
    int bestMoveCnt;

    bool lastNullMove;

    int threadCount;
    int tDepth, selDepth;

    std::unique_ptr <std::thread> principalThread;
    std::mutex readyMutex;

    uint64_t nodes, qsNodes;
    bool principalSearcher;
    Board board;

    tt::HashTable* threadTT;

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