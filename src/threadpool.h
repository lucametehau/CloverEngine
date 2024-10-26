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
#include "thread.h"

#include <cassert>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

bool printStats = true; // true by default

class SearchThread {
public:
    SearchData thread_data;
    std::atomic<bool> stop{false};

    SearchThread(int id) { 
        thread_data.thread_id = id;
        thread_data.flag_stopped = false;
    }

    SearchThread(const SearchThread&) = delete;
    SearchThread& operator = (const SearchThread&) = delete;

    SearchThread(SearchThread&& other) noexcept : thread_data(std::move(other.thread_data)), stop(other.stop.load()) {}
    SearchThread& operator = (SearchThread&& other) {
        if (this != &other) {
            thread_data = std::move(other.thread_data);
            stop = other.stop.load();
        }
        return *this;
    }

    void stop_thread() { stop = true; thread_data.flag_stopped = true; thread_data.nodes = 0; thread_data.tb_hits = 0; }
    void unstop_thread() { stop = false; thread_data.flag_stopped = false; }
    void search(Info &info) {
        unstop_thread();
        thread_data.start_search(info);
    }

    inline void clear_stack() { thread_data.clear_stack(); }
    inline void clear_history() { thread_data.clear_history(); }

    inline void set_fen(std::string fen, bool chess960 = false) {
        thread_data.board.chess960 = chess960;
        thread_data.board.set_fen(fen);
    }

    inline void set_dfrc(int idx) {
        thread_data.board.chess960 = (idx > 0);
        thread_data.board.set_dfrc(idx);
    }

    inline void make_move(Move move) { thread_data.board.make_move(move); }
    inline void clear_board() { thread_data.board.clear(); }

    inline uint64_t get_nodes() const { return thread_data.nodes; }
    inline uint64_t get_tbhits() const { return thread_data.tb_hits; } 
};

class ThreadPool {
public:
    std::vector<SearchThread> search_threads;

private:
    std::mutex threads_mutex;
    std::condition_variable cv;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> job_queue;
    std::atomic<bool> destroy_pool{false}; // used to kill the pool

public:
    ThreadPool(const std::size_t thread_count = 1) { create_pool(thread_count); }

    ~ThreadPool() { delete_pool(); }

    void delete_pool() {
        //std::cerr << "Deleting pool\n";
        {
            std::lock_guard<std::mutex> lock(threads_mutex);
            destroy_pool = true;
        }
        cv.notify_all();
        for (auto &thread : threads) {
            if (thread.joinable()) thread.join();
        }
        threads.clear();
        search_threads.clear();
    }

    void create_pool(const std::size_t thread_count) {
        delete_pool();
        //std::cerr << "Creating pool with " << thread_count << " threads\n";
        destroy_pool = false;

        for (std::size_t i = 0; i < thread_count; i++) {
            search_threads.emplace_back(i);
            threads.emplace_back(&ThreadPool::thread_loop, this);
        }

        //std::cerr << "We now have " << threads.size() << " threads\n";
    }

    void thread_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(threads_mutex);
                cv.wait(lock, [&] { return !job_queue.empty() || destroy_pool; });

                if (job_queue.empty() && destroy_pool) return;

                task = std::move(job_queue.front()); // better than copy ig?
                job_queue.pop();
            }
            task();
        }
    }

    void queue_task(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(threads_mutex);
            job_queue.push(task);
        }
        cv.notify_one();
    }

    inline void pool_stop() {
        //std::cerr << "Stopping pool\n";
        for (auto &search_thread : search_threads) search_thread.stop_thread();
        cv.notify_all();
    }

    Board& get_board() { assert(!search_threads.empty()); return search_threads[0].thread_data.board; }

    uint64_t get_total_nodes_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t nodes = 0;
        for (auto &search_thread : search_threads) nodes += search_thread.get_nodes();
        return nodes;
    }

    uint64_t get_total_tb_hits_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t tbhits = 0;
        for (auto &search_thread : search_threads) tbhits += search_thread.get_tbhits();
        return tbhits;
    }

    inline void clear_stack() {
        //std::cerr << "Clearing stack pool\n";
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_stack(); });
        }
    }

    inline void clear_history() {
        //std::cerr << "Clearing history pool\n";
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_history(); });
        }
    }

    inline void set_fen(std::string fen, bool chess960 = false) {
        //std::cerr << "Setting fen " << fen << " pool\n";
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([fen, chess960, i, this]() { search_threads[i].set_fen(fen, chess960); });
        }
    }

    inline void set_dfrc(int idx) {
        //std::cerr << "Setting DFRC " << idx << " pool\n";
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([idx, i, this]() { search_threads[i].set_dfrc(idx); });
        }
    }

    inline void make_move(Move move) {
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([move, i, this]() { search_threads[i].make_move(move); });
        }
    }

    inline void clear_board() {
        assert(!search_threads.empty());
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_board(); });
        }
    }

    void main_thread_handler(Info &info) {
        assert(!search_threads.empty());
        destroy_pool = false;
        for (std::size_t i = 1; i < search_threads.size(); i++) {
            queue_task([i, &info, this]() { search_threads[i].search(info); });
        }
        search_threads[0].search(info);

        pool_stop();

        int best_score = 0;
        Move best_move = NULLMOVE;
        
        int bestDepth = search_threads[0].thread_data.completed_depth;
        best_score = search_threads[0].thread_data.root_score[1];
        best_move = search_threads[0].thread_data.best_move[1];
        for (std::size_t i = 1; i < search_threads.size(); i++) {
            if (search_threads[i].thread_data.root_score[1] > best_score && search_threads[i].thread_data.completed_depth >= bestDepth) {
                best_score = search_threads[i].thread_data.root_score[1];
                best_move = search_threads[i].thread_data.best_move[1];
                bestDepth = search_threads[i].thread_data.completed_depth;
            }
        }

        if (printStats) {
            std::cout << "bestmove " << move_to_string(best_move, info.chess960);
            std::cout << std::endl;
        }
    }
};

ThreadPool thread_pool;