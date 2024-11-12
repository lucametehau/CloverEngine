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
#include "evaluate.h"
#include "search-info.h"
#include "net.h"
#include "fen.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>

static bool printStats = true; // true by default

typedef int ThreadState;

enum ThreadStates : ThreadState {
    IDLE = 0, SEARCH = 1, STOP = 2, EXIT = 4
};

class ThreadPool;

class SearchThread {
public:
    Info m_info;

    MoveList m_best_moves;
    std::array<int, MAX_MOVES> m_scores, m_root_scores;
    std::array<MeanValue, 100> m_values;

private:
    std::array<int, MAX_DEPTH + 5> m_pv_table_len;
    MultiArray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5> m_pv_table;
    std::array<uint64_t, 64 * 64> m_nodes_seached;
    std::array<StackEntry, MAX_DEPTH + 15> m_search_stack;
    StackEntry* m_stack;
    
    Histories m_histories;

    int m_time_check_count;
    int m_best_move_cnt;
    int m_multipv;
    int m_id_depth, m_sel_depth;
    int m_root_eval;

public:
    uint64_t m_tb_hits;
    int64_t m_nodes;
    int m_completed_depth;
    Board m_board;
    Network NN;

public:
    ThreadPool* m_thread_pool;
    int m_thread_id;
    std::mutex m_mutex;
    std::thread m_thread;
    std::condition_variable m_cv;
    std::atomic<ThreadState> m_state{ThreadStates::IDLE};

#ifdef GENERATE
    HashTable* TT;
#endif
    
public:
    SearchThread() {}

    SearchThread(ThreadPool* thread_pool, int thread_id) : m_thread_pool(thread_pool), m_thread_id(thread_id) {
        m_state = ThreadStates::IDLE;
        m_thread = std::thread(&SearchThread::main_loop, this);
    }

    ~SearchThread() = default;

public:
    bool main_thread() { return m_thread_id == 0; }
    bool must_stop() { return m_state & (ThreadStates::STOP | ThreadStates::EXIT); }

    void wait_for_finish() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] { return !(m_state & ThreadStates::SEARCH); });
    }

    void start_search();

    void main_loop() {
        while (!(m_state & ThreadStates::EXIT)) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&] { return m_state & ThreadStates::SEARCH; });
                
            if (m_state & ThreadStates::EXIT) return;

            start_search();
            m_state &= ~ThreadStates::SEARCH;
            m_cv.notify_all();
        }
    }

    void exit() {
        m_state = ThreadStates::EXIT | ThreadStates::SEARCH | ThreadStates::STOP;
        m_cv.notify_all();
        if (m_thread.joinable()) m_thread.join();
    }

public:
    void clear_stack() {
        m_pv_table_len.fill(0);
        m_nodes_seached.fill(0);
        fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(m_pv_table, NULLMOVE);
    }
    void clear_history() { m_histories.clear_history(); }

    void make_move(Move move, HistoricalState& next_state) { 
        const Piece piece = m_board.piece_at(move.get_from());
        m_board.make_move(move, next_state);
        NN.add_move_to_history(move, piece, m_board.captured());
    }

    void undo_move(Move move) {
        m_board.undo_move(move);
        NN.revert_move();
    }

private:
    template <bool pvNode>
    int quiesce(int alpha, int beta, StackEntry* stack);

    template <bool rootNode, bool pvNode, bool cutNode>
    int search(int alpha, int beta, int depth, StackEntry* stack);

    void iterative_deepening();

    void print_pv();
    void update_pv(int ply, Move move);

    void print_iteration_info(bool san_mode, int multipv, int score, int alpha, int beta, uint64_t t, int depth, int sel_depth, uint64_t total_nodes, uint64_t total_tb_hits);

    template <bool checkTime>
    bool check_for_stop();

    int draw_score() { return 1 - (m_nodes & 2); }
};

class ThreadPool {
public:
    std::vector<std::unique_ptr<SearchThread>> m_threads;
    Info m_info;
    Board m_board;

    ThreadPool() { create_pool(0); }
    ~ThreadPool() { exit(); }

    void create_pool(std::size_t thread_count) {
        exit();
        m_threads.clear();
        for (std::size_t i = 0; i < thread_count; i++) m_threads.push_back(std::make_unique<SearchThread>(this, i));
    }

    std::size_t get_num_threads() { return m_threads.size(); }

    void is_ready() {
        wait_for_finish();
        std::cout << "readyok" << std::endl;
    }

    void stop() {
        for (auto &thread : m_threads) thread->m_state |= ThreadStates::STOP;
    }
    void exit() {
        for (auto &thread : m_threads) thread->exit();
    }
    void wait_for_finish(bool main_as_well = true) {
        for (auto &thread : m_threads) {
            if (main_as_well || thread != m_threads.front()) thread->wait_for_finish();
        }
    }
    void clear_history() {
        for (auto &thread : m_threads) thread->clear_history();
    }
    void clear_board() {
        m_board.clear();
    }
    
    Board &get_board() { return m_board; }

    uint64_t get_nodes() {
        uint64_t nodes = 0;
        for (auto &thread : m_threads) nodes += thread->m_nodes;
        return nodes;
    }

    uint64_t get_tbhits() {
        uint64_t tbhits = 0;
        for (auto &thread : m_threads) tbhits += thread->m_tb_hits;
        return tbhits;
    }

    void search(Info info) {
        m_info = info;
        stop();
        wait_for_finish();
        for (auto &thread : m_threads) {
            thread->m_state &= ~ThreadStates::STOP;
            std::lock_guard<std::mutex> lock(thread->m_mutex);
            thread->m_state |= ThreadStates::SEARCH;
        }
        for (auto &thread : m_threads) {
            thread->m_cv.notify_all();
        }
    }

    void pick_and_print_best_thread() {
        int best_score = 0;
        Move best_move = NULLMOVE;
        
        int bestDepth = m_threads.front()->m_completed_depth;
        best_score = m_threads.front()->m_root_scores[1];
        best_move = m_threads.front()->m_best_moves[1];
        for (std::size_t i = 1; i < m_threads.size(); i++) {
            if (m_threads[i]->m_root_scores[1] > best_score && m_threads[i]->m_completed_depth >= bestDepth) {
                best_score = m_threads[i]->m_root_scores[1];
                best_move = m_threads[i]->m_best_moves[1];
                bestDepth = m_threads[i]->m_completed_depth;
            }
        }

        if (printStats) {
            std::cout << "bestmove " << best_move.to_string(m_info.is_chess960());
            std::cout << std::endl;
        }
    }
};

static ThreadPool thread_pool;