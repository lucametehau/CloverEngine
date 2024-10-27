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
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

#ifndef TUNE_FLAG
constexpr int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#else
int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#endif

class SearchThread {
public:
    SearchThread() {}

    SearchThread(SearchThread&& other) noexcept
        : info(std::move(other.info)), best_move(std::move(other.best_move)),
        scores(std::move(other.scores)), root_score(std::move(other.root_score)),
        values(std::move(other.values)), pv_table_len(std::move(other.pv_table_len)),
        pv_table(std::move(other.pv_table)), search_stack(std::move(other.search_stack)),
        histories(std::move(other.histories)), t0(other.t0), checkCount(other.checkCount),
        best_move_cnt(other.best_move_cnt), multipv_index(other.multipv_index),
        tDepth(other.tDepth), sel_depth(other.sel_depth), rootEval(other.rootEval),
        tb_hits(other.tb_hits), nodes(other.nodes), completed_depth(other.completed_depth),
        thread_id(other.thread_id), board(std::move(other.board)), flag_stopped(other.flag_stopped.load()) 
    {}

    SearchThread& operator=(SearchThread&& other) noexcept {
        if (this != &other) {
            info = std::move(other.info);
            best_move = std::move(other.best_move);
            scores = std::move(other.scores);
            root_score = std::move(other.root_score);
            values = std::move(other.values);
            pv_table_len = std::move(other.pv_table_len);
            pv_table = std::move(other.pv_table);
            search_stack = std::move(other.search_stack);
            histories = std::move(other.histories);
            t0 = other.t0;
            checkCount = other.checkCount;
            best_move_cnt = other.best_move_cnt;
            multipv_index = other.multipv_index;
            tDepth = other.tDepth;
            sel_depth = other.sel_depth;
            rootEval = other.rootEval;
            tb_hits = other.tb_hits;
            nodes = other.nodes;
            completed_depth = other.completed_depth;
            thread_id = other.thread_id;
            board = std::move(other.board);
            flag_stopped = other.flag_stopped.load();
            // Optionally reset 'other' fields as needed
        }
        return *this;
    }

    inline void clear_stack() {
        pv_table_len.fill(0);
        nodes_seached.fill(0);
        fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(pv_table, 0);
    }

    inline void clear_history() {
        histories.clear_history();
    }

    void start_search(Info info);

    inline void setTime(Info &_info) { info = _info; }

    void stop_thread() { flag_stopped = true; nodes = 0; tb_hits = 0; }
    void search(Info info) {
        {
            std::mutex lol;
            std::lock_guard<std::mutex> lock(lol);
            std::cout << "Starting thread " << thread_id << " with " << flag_stopped << std::endl;
        }
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

    inline void make_move(Move move) {
        using namespace std::chrono_literals;
        //std::this_thread::sleep_for(1000ms); 
        board.make_move(move); }
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

    std::atomic<bool> flag_stopped;

#ifdef GENERATE
    HashTable* TT;
#endif
    
};