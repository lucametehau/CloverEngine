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

#ifndef TUNE_FLAG
constexpr int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#else
int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#endif

class alignas(ALIGN) SearchThread {
public:
    SearchThread() {}

    SearchThread(int id) {
        thread_id = id;
        flag_stopped = false;
    }

    inline void clear_stack() {
        pv_table_len.fill(0);
        nodes_seached.fill(0);
        fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(pv_table, 0);
    }

    inline void clear_history() {
        histories.clear_history();
    }

    void start_search(Info &info);

    inline void setTime(Info &_info) { info = _info; }

    void stop_thread() { flag_stopped = true; nodes = 0; tb_hits = 0; }
    void unstop_thread() { flag_stopped = false; }
    void search(Info &info) {
        unstop_thread();
        start_search(info);
    }

    inline void set_fen(std::string fen, bool chess960 = false) {
        board.chess960 = chess960;
        board.set_fen(fen);
    }

    inline void set_dfrc(int idx) {
        board.chess960 = (idx > 0);
        board.set_dfrc(idx);
    }

    inline void make_move(Move move) { board.make_move(move); }
    inline void clear_board() { board.clear(); }

private:
    inline bool main_thread() { return thread_id == 0; }

    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack);

    template <bool rootNode, bool pvNode, bool cutNode>
    int search(int alpha, int beta, int depth, StackEntry* stack);

    void print_pv();
    void update_pv(int ply, int move);

    void print_iteration_info(bool san_mode, int multipv, int score, int alpha, int beta, uint64_t t, int depth, int sel_depth, uint64_t total_nodes, uint64_t total_tb_hits);

    template <bool checkTime>
    bool check_for_stop();

    std::array<uint64_t, 64 * 64> nodes_seached;

public:
    Info info;

    MoveList best_move;
    std::array<int, MAX_MOVES> scores, root_score;
    std::array<MeanValue, 100> values;

private:
    std::array<int, MAX_DEPTH + 5> pv_table_len;
    MultiArray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5> pv_table;
    std::array<StackEntry, MAX_DEPTH + 15> search_stack;
    
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