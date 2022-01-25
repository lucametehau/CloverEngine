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

/// search params (tuning all with ctt, but I didn't get any nice results)

int RazorCoef = 325;
int SNMPCoef1 = 85;
int SNMPCoef2 = 20;
int seeCoefQuiet = 80;
int seeCoefNoisy = 18;
int fpCoef = 90; /// 51
int histDiv = 5000;

const int TERMINATED_BY_USER = 1;
const int TERMINATED_BY_TIME = 2;
const int TERMINATED_SEARCH  = 3; /// 1 | 2

class Search {

  friend class Movepick;
  friend class History;

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
    void startPrincipalSearch(Info *info);
    void stopPrincipalSearch();
    void isReady();
    int getThreadCount();

    void _setFen(std::string fen);
    void _makeMove(uint16_t move);

    std::pair <int, uint16_t> startSearch(Info *info);
    int quiesce(int alpha, int beta, bool useTT = true); /// for quiet position check (tuning)
    int search(int alpha, int beta, int depth, bool cutNode, uint16_t excluded = NULLMOVE);

    void setTime(Info *tInfo) {
      info = tInfo;
    }

  private:
    void startWorkerThreads(Info *info);
    void flagWorkersStop();
    void stopWorkerThreads();
    void lazySMPSearcher();
    void releaseThreads();
    void waitUntilDone();

    void printPv();
    void updatePv(int ply, int move);

    bool checkForStop();

  public:

    uint64_t nodesSearched[64][64];
    uint16_t pvTable[DEPTH + 5][DEPTH + 5];
    int pvTableLen[DEPTH + 5];
    uint16_t killers[DEPTH + 5][2];
    uint16_t cmTable[2][13][64];
    int hist[2][64][64];
    int follow[2][13][64][13][64];
    int capHist[13][6][64];
    int lmrCnt[2][9];
    int lmrRed[64][64];
    StackEntry Stack[DEPTH + 5];

    Info *info;
    volatile int flag;

  private:
    uint64_t tbHits;
    uint64_t t0;

    int checkCount;

    uint64_t nmpFail, nmpTries;
    int bestMoveCnt;

    int threadCount;
    int tDepth, selDepth;

    std::unique_ptr <std::thread> principalThread;
    std::mutex readyMutex;

  public:
    uint64_t nodes;
    bool principalSearcher;
    Board board;

    tt :: HashTable *threadTT;

  private:
    std::unique_ptr <std::thread[]> threads;
    std::unique_ptr <Search[]> params;
    std::condition_variable lazyCV;
    volatile int lazyDepth;
    volatile bool SMPThreadExit;

    bool isLazySMP() {
      return lazyDepth > 0;
    }

    void resetLazySMP() {
      lazyDepth = 0;
    }

    const int cmpDepth[2] = {3, 2};
    const int cmpHistoryLimit[2] = {    0, -1000};

    const int fmpDepth[2] = {3, 2};
    const int fmpHistoryLimit[2] = {-2000, -4000};

    const int fpHistoryLimit[2] =  {12000,  6000};

    bool terminateSMP;

};
