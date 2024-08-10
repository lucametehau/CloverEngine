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
#include "history.h"

#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#ifndef TUNE_FLAG
constexpr int seeVal[] = { 0, SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000 };
#else
int seeVal[] = { 0, SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000 };
#endif

class SearchData {
public:
    SearchData() {}

    inline void clear_stack() {
        pv_table_len.fill(0);
        nodes_seached.fill(0);
        fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(pv_table, 0);
    }

    inline void clear_history() {
        histories.clear_history();
    }

    void start_search(Info* info);

    inline void setTime(Info* _info) { info = _info; }

private:
    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack); /// for quiet position check (tuning)

    template <bool rootNode, bool pvNode>
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack);

    void print_pv();
    void update_pv(int ply, int move);

    template <bool checkTime>
    bool check_for_stop();

    std::array<uint64_t, 64 * 64> nodes_seached;

public:
    Info* info;

    std::array<Move, 256> best_move;
    std::array<int, 256> scores, root_score;
    std::array<MeanValue, 10> values;

private:
    std::array<int, MAX_DEPTH + 5> pv_table_len;
    MultiArray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5> pv_table;
    
    Histories histories;

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