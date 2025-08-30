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
#include "evaluate.h"
#include "fen.h"
#include "history.h"
#include "net.h"
#include "root-moves.h"
#include "search-info.h"
#include "tt.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

static bool printStats = true; // true by default

typedef int ThreadState;

enum ThreadStates : ThreadState
{
    IDLE = 0,
    SEARCH = 1,
    STOP = 2,
    EXIT = 4
};

class ThreadPool;

class alignas(64) SearchThread
{
  public:
    Info info;

    RootMoves root_moves;
    std::array<MeanValue, 100> values;

  private:
    std::array<int, MAX_DEPTH + 5> pv_table_len;
    MultiArray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5> pv_table;
    MultiArray<Move, 2, KP_MOVE_SIZE> kp_move;
    std::array<StackEntry, MAX_DEPTH + 15> search_stack;
    StackEntry *stack;

    Histories histories;

    int time_check_count;
    int best_move_cnt;
    int multipv;
    int id_depth, sel_depth;
    int root_eval;

  public:
    std::atomic<int64_t> tb_hits;
    std::atomic<int64_t> nodes;
    int completed_depth;
    Board board;
    Network NN;

  public:
    ThreadPool *thread_pool;
    int thread_id;
    std::mutex mutex;
    std::thread thread;
    std::condition_variable cv;
    std::atomic<ThreadState> state{ThreadStates::IDLE};

#ifdef GENERATE
    HashTable *TT;
#endif

  public:
    SearchThread()
    {
    }

    SearchThread(ThreadPool *thread_pool, int thread_id) : thread_pool(thread_pool), thread_id(thread_id)
    {
        state = ThreadStates::IDLE;
        thread = std::thread(&SearchThread::main_loop, this);
    }

    ~SearchThread() = default;

  public:
    constexpr bool main_thread() const
    {
        return thread_id == 0;
    }
    bool must_stop()
    {
        return state & (ThreadStates::STOP | ThreadStates::EXIT);
    }

    int64_t get_nodes()
    {
        return nodes.load(std::memory_order_relaxed);
    }
    int64_t get_tb_hits()
    {
        return tb_hits.load(std::memory_order_relaxed);
    }

    void wait_for_finish()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return !(state & ThreadStates::SEARCH); });
    }

    void start_search();

    void main_loop()
    {
        while (!(state & ThreadStates::EXIT))
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&] { return state & ThreadStates::SEARCH; });

            if (state & ThreadStates::EXIT)
                return;

            start_search();
            state &= ~ThreadStates::SEARCH;
            cv.notify_all();
        }
    }

    void exit()
    {
        state = ThreadStates::EXIT | ThreadStates::SEARCH | ThreadStates::STOP;
        cv.notify_all();
        if (thread.joinable())
            thread.join();
    }

  public:
    void clear_stack()
    {
        pv_table_len.fill(0);
        fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(pv_table, NULLMOVE);
    }
    void clear_history()
    {
        histories.clear_history();
        fill_multiarray<Move, 2, KP_MOVE_SIZE>(kp_move, NULLMOVE);
    }

    void make_move(Move move, HistoricalState &next_state)
    {
        const Piece piece = board.piece_at(move.get_from());
        board.make_move(move, next_state);
        NN.add_move_to_history(move, piece, board.captured());
    }

    void undo_move(Move move)
    {
        board.undo_move(move);
        NN.revert_move();
    }

  private:
    template <bool pvNode> int quiesce(int alpha, int beta, StackEntry *stack);

    template <bool rootNode, bool pvNode, bool cutNode> int search(int alpha, int beta, int depth, StackEntry *stack);

    void iterative_deepening();

    void update_pv(const int ply, const Move move)
    {
        pv_table[ply][0] = move;
        for (int i = 0; i < pv_table_len[ply + 1]; i++)
            pv_table[ply][i + 1] = pv_table[ply + 1][i];
        pv_table_len[ply] = 1 + pv_table_len[ply + 1];
    }

    void print_iteration_info(uint64_t t, int depth, uint64_t total_nodes, uint64_t total_tb_hits);

    template <bool checkTime> bool check_for_stop()
    {
        if (!main_thread())
            return 0;

        if (must_stop())
            return 1;

        if (info.nodes_limit_passed(nodes) || info.max_nodes_passed(nodes))
        {
            state |= ThreadStates::STOP;
            return 1;
        }

        time_check_count++;
        if (time_check_count == (1 << 10))
        {
            if constexpr (checkTime)
            {
                if (info.hard_limit_passed())
                    state |= ThreadStates::STOP;
            }
            time_check_count = 0;
        }

        return must_stop();
    }

    int draw_score()
    {
        return 1 - (get_nodes() & 2);
    }
};

class ThreadPool
{
  public:
    std::vector<std::unique_ptr<SearchThread>> threads;
    Info info;
    Board board;

    ThreadPool()
    {
        create_pool(0);
    }
    ~ThreadPool()
    {
        exit();
    }

    void create_pool(std::size_t thread_count)
    {
#ifndef GENERATE
        exit();
#endif
        threads.clear();
        for (std::size_t i = 0; i < thread_count; i++)
            threads.push_back(std::make_unique<SearchThread>(this, i));
    }

    std::size_t get_num_threads()
    {
        return threads.size();
    }

    void is_ready()
    {
        wait_for_finish();
        std::cout << "readyok" << std::endl;
    }

    void stop()
    {
        for (auto &thread : threads)
            thread->state |= ThreadStates::STOP;
    }
    void exit()
    {
        for (auto &thread : threads)
            thread->exit();
    }
    void wait_for_finish(bool main_as_well = true)
    {
        for (auto &thread : threads)
        {
            if (main_as_well || thread != threads.front())
                thread->wait_for_finish();
        }
    }
    void clear_info()
    {
        for (auto &thread : threads)
            thread->nodes = thread->tb_hits = 0;
    }
    void clear_history()
    {
        for (auto &thread : threads)
            thread->clear_history();
    }
    void clear_board()
    {
        board.clear();
    }

    Board &get_board()
    {
        return board;
    }

    uint64_t get_nodes()
    {
        uint64_t nodes = 0;
        for (auto &thread : threads)
            nodes += thread->get_nodes();
        return nodes;
    }

    uint64_t get_tbhits()
    {
        uint64_t tbhits = 0;
        for (auto &thread : threads)
            tbhits += thread->get_tb_hits();
        return tbhits;
    }

    void search(Info _info)
    {
        info = _info;
        stop();
        wait_for_finish();
        for (auto &thread : threads)
        {
            thread->state &= ~ThreadStates::STOP;
            std::lock_guard<std::mutex> lock(thread->mutex);
            thread->state |= ThreadStates::SEARCH;
        }
        for (auto &thread : threads)
        {
            thread->cv.notify_all();
        }
    }

    void pick_and_print_best_thread()
    {
        int best_score = 0;
        Move best_move = NULLMOVE;

        int bestDepth = threads.front()->completed_depth;
        best_score = threads.front()->root_moves[0].score;
        best_move = threads.front()->root_moves[0].pv[0];
        for (std::size_t i = 1; i < threads.size(); i++)
        {
            if (threads[i]->root_moves[0].score > best_score && threads[i]->completed_depth >= bestDepth)
            {
                best_score = threads[i]->root_moves[0].score;
                best_move = threads[i]->root_moves[0].pv[0];
                bestDepth = threads[i]->completed_depth;
            }
        }

        if (printStats)
        {
            std::cout << "bestmove " << best_move.to_string(threads[0]->info.is_chess960()) << std::endl;
        }
    }
};

static ThreadPool thread_pool;