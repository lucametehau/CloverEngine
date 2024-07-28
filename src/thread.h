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

int seeVal[] = { 0, SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000 };

const int TERMINATED_BY_USER = 1;
const int TERMINATED_BY_LIMITS = 2;
const int TERMINATED_SEARCH = TERMINATED_BY_USER | TERMINATED_BY_LIMITS;

struct SearchData {
    SearchData() {
        for (int i = 0; i < 64; i++) { /// depth
            for (int j = 0; j < 64; j++) { /// moves played 
                lmrRed[i][j] = LMRQuietBias / 100.0 + log(i) * log(j) / (LMRQuietDiv / 100.0);
                lmrRedNoisy[i][j] = LMRNoisyBias / 100.0 + lmrRed[i][j] / (LMRNoisyDiv / 100.0);
            }
        }
    }

    //~SearchData() {}

    //SearchData(const SearchData&) = delete;
    //SearchData& operator = (const SearchData&) = delete;

    Move easy_move();
    void start_search(Info* info);

    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack); /// for quiet position check (tuning)

    template <bool rootNode, bool pvNode>
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack);

    void setTime(Info* tInfo) { info = tInfo; }

    void printPv();
    void update_pv(int ply, int move);

    template <bool checkTime>
    bool check_for_stop();

    uint64_t nodes_seached[64 * 64];

    int16_t hist[2][2][2][64 * 64];
    int16_t cap_hist[13][64][7];
    TablePieceTo cont_history[2][13][64];
    int corr_hist[2][65536];

    int lmrRed[64][64], lmrRedNoisy[64][64];

    int pvTableLen[DEPTH + 5];
    Move pvTable[DEPTH + 5][2 * DEPTH + 5];
    Move bestMoves[256];
    int scores[256], rootScores[256];
    MeanValue values[10];

    uint64_t tbHits;
    int64_t t0;
    Info* info;
    int checkCount;

    int best_move_cnt;
    int multipv_index;

    int tDepth, completedDepth, selDepth;

    int rootEval, rootDepth;

    int64_t nodes;
    int thread_id;
    Board board;

#ifdef GENERATE
    HashTable* TT;
#endif
    
};

std::vector<std::thread> threads;
std::vector<SearchData> threads_data;
std::thread main_thread;
std::mutex threads_mutex;
volatile int global_flag;