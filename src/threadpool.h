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

class ThreadPool {
public:
    std::vector<SearchThread> search_threads;

public:
    std::mutex threads_mutex;
    std::condition_variable cv;
    std::vector<std::thread> threads;
    std::queue<std::tuple<std::function<void()>, std::string, int>> job_queue;
    std::atomic<int> actual_job_queue_size{0};
    std::atomic<bool> destroy_pool{false}; // used to kill the pool

public:
    ThreadPool(const std::size_t thread_count = 0) { actual_job_queue_size = 0; destroy_pool = false; create_pool(thread_count); }

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
            auto &search_thread = search_threads.emplace_back();
            search_thread.thread_id = i;
            search_thread.flag_stopped = false;
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

                task = std::move(std::get<0>(job_queue.front())); // better than copy ig?
                std::cout << std::get<1>(job_queue.front()) << " is being processed on thread " << std::get<2>(job_queue.front()) << "\n";
                job_queue.pop();
            }
            task();
            actual_job_queue_size--;
        }
    }

    void queue_task(std::function<void()> task, std::string name = "", int thread_id = 0) {
        {
            std::lock_guard<std::mutex> lock(threads_mutex);
            actual_job_queue_size++;
            //std::cout << name << " " << thread_id << "\n";
            job_queue.push({task, name, thread_id});
        }
        cv.notify_one();
    }

    inline void pool_stop() {
        //std::cerr << "Stopping pool\n";
        for (auto &search_thread : search_threads) search_thread.stop_thread();
        cv.notify_all();
    }

    uint64_t get_total_nodes_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t nodes = 0;
        for (auto &search_thread : search_threads) nodes += search_thread.nodes, std::cout << search_thread.nodes << " ";
        std::cout << std::endl;
        return nodes;
    }

    uint64_t get_total_tb_hits_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t tbhits = 0;
        for (auto &search_thread : search_threads) tbhits += search_thread.tb_hits;
        return tbhits;
    }

    void wait_jobs_done() {
        while (actual_job_queue_size);
    }

    void isready() {
        // wait for all the jobs to be done
        wait_jobs_done();
        std::cout << "readyok" << std::endl;
    }

    inline void clear_stack() {
        //std::cerr << "Clearing stack pool\n";
        assert(!search_threads.empty());
        //search_threads[0].clear_stack();
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_stack(); }, "clear_stack", i);
        }
    }

    inline void clear_history() {
        //std::cerr << "Clearing history pool\n";
        assert(!search_threads.empty());
        //search_threads[0].clear_history();
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_history(); }, "clear_history", i);
        }
    }

    inline void set_fen(std::string fen, bool chess960 = false) {
        //std::cerr << "Setting fen " << fen << " pool\n";
        assert(!search_threads.empty());
        //search_threads[0].set_fen(fen, chess960);
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([fen, chess960, i, this]() { search_threads[i].set_fen(fen, chess960); }, "set_fen", i);
        }
    }

    inline void set_dfrc(int idx) {
        //std::cerr << "Setting DFRC " << idx << " pool\n";
        assert(!search_threads.empty());
        //search_threads[0].set_dfrc(idx);
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([idx, i, this]() { search_threads[i].set_dfrc(idx); }, "set_dfrc", i);
        }
    }

    inline void make_move(Move move) {
        assert(!search_threads.empty());
        //search_threads[0].make_move(move);
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([move, i, this]() { search_threads[i].make_move(move); }, "make_move", i);
        }
    }

    inline void clear_board() {
        assert(!search_threads.empty());
        //search_threads[0].clear_board();
        for (std::size_t i = 0; i < search_threads.size(); i++) {
            queue_task([i, this]() { search_threads[i].clear_board(); }, "clear_board", i);
        }
    }

    void main_thread_handler(Info info) {
        assert(!search_threads.empty());
        wait_jobs_done();
        destroy_pool = false;
        for (std::size_t i = 1; i < search_threads.size(); i++) {
            queue_task([i, info, this]() { search_threads[i].search(info); }, "start_search", i);
        }
        search_threads[0].search(info);

        pool_stop();
        wait_jobs_done();

        int best_score = 0;
        Move best_move = NULLMOVE;
        
        int bestDepth = search_threads[0].completed_depth;
        best_score = search_threads[0].root_score[1];
        best_move = search_threads[0].best_move[1];
        for (std::size_t i = 1; i < search_threads.size(); i++) {
            if (search_threads[i].root_score[1] > best_score && search_threads[i].completed_depth >= bestDepth) {
                best_score = search_threads[i].root_score[1];
                best_move = search_threads[i].best_move[1];
                bestDepth = search_threads[i].completed_depth;
            }
        }

        if (printStats) {
            std::cout << "bestmove " << move_to_string(best_move, info.chess960);
            std::cout << std::endl;
        }
    }
};

ThreadPool thread_pool;