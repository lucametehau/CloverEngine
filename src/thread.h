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

class SearchData {
public:
    SearchData() {
        for (int i = 0; i < 64; i++) { /// depth
            for (int j = 0; j < 64; j++) { /// moves played 
                lmr_red[i][j] = LMRQuietBias / 100.0 + log(i) * log(j) / (LMRQuietDiv / 100.0);
                lmr_red_noisy[i][j] = LMRNoisyBias / 100.0 + lmr_red[i][j] / (LMRNoisyDiv / 100.0);
            }
        }
    }

    void clear_stack() {
        memset(pv_table_len, 0, sizeof(pv_table_len));
        memset(pv_table, 0, sizeof(pv_table));
        memset(nodes_seached, 0, sizeof(nodes_seached));
    }

    void clear_history() {
        memset(hist, 0, sizeof(hist));
        memset(cap_hist, 0, sizeof(cap_hist));
        memset(cont_history, 0, sizeof(cont_history));
        memset(corr_hist, 0, sizeof(corr_hist));
    }

    void start_search(Info* info);

    void setTime(Info* _info) { info = _info; }

private:
    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack); /// for quiet position check (tuning)

    template <bool rootNode, bool pvNode>
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack);

    void print_pv();
    void update_pv(int ply, int move);

    template <bool checkTime>
    bool check_for_stop();

    uint64_t nodes_seached[64 * 64];

public:
    Info* info;
    History<16384> hist[2][2][2][64 * 64];
    History<16384> cap_hist[13][64][7];
    TablePieceTo cont_history[2][13][64];
    int corr_hist[2][65536];

    int lmr_red[64][64], lmr_red_noisy[64][64];

    int pv_table_len[MAX_DEPTH + 5];
    Move pv_table[MAX_DEPTH + 5][2 * MAX_DEPTH + 5];
    Move best_move[256];
    int scores[256], root_score[256];
    MeanValue values[10];

private:
    int64_t t0;
    int checkCount;

    int best_move_cnt;
    int multipv_index;

    int tDepth, sel_depth;

    int rootEval;

public:
    uint64_t tb_hits;
    int64_t nodes;
    int completed_depth;
    int thread_id;
    Board board;

    volatile bool flag_stopped;

#ifdef GENERATE
    HashTable* TT;
#endif
    
};

std::vector<std::thread> threads;
std::vector<SearchData> threads_data;
std::mutex threads_mutex;