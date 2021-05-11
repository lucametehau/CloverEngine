#pragma once
#include "board.h"
#include "tt.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/// search params

int RazorCoef = 325;
int StaticNullCoef = 85;
int StaticNullImproveCoef = 0;

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

    void startSearch(Info *info);
    int quiesce(int alpha, int beta); /// for quiet position check (tuning)

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
    int search(int alpha, int beta, int depth, uint16_t excluded = NULLMOVE);

    void printPv();
    void updatePv(int ply, int move);

    bool checkForStop();

  public:

    uint16_t pvTable[DEPTH + 5][DEPTH + 5];
    int pvTableLen[DEPTH + 5];
    uint16_t killers[DEPTH + 5][2];
    uint16_t cmTable[2][13][64];
    int hist[2][64][64];
    int follow[2][13][64][13][64];
    int lmrCnt[2][9];
    int lmrRed[64][64];
    StackEntry Stack[DEPTH + 5];

  private:
    uint64_t tbHits;
    uint32_t t0;
    Info *info;

    volatile int flag;
    int checkCount;

    int threadCount;
    int tDepth, selDepth;

    std::unique_ptr <std::thread> principalThread;
    std::mutex readyMutex;

  public:
    uint64_t nodes;
    bool principalSearcher;
    Board board;

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
