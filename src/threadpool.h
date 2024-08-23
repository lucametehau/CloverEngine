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

bool printStats = true; // true by default

// not a thread pool, but i couldnt come up with a better name
class ThreadPool {
public:
    std::vector<SearchData> threads_data;

private:
    std::mutex threads_mutex;
    std::vector<std::thread> threads;

public:
    ThreadPool() { threads.clear(); threads_data.clear(); }

    inline void join_threads() {
        for (auto &thread : threads) {
            if(thread.joinable())
                thread.join();
        }
    }

    inline void delete_pool() {
        join_threads();
        threads.clear();
        threads_data.clear();
    }

    inline void create_pool(const int thread_count) {
        delete_pool(); // delete previous existing pool
        for (int i = 0; i < thread_count; i++) {
            threads_data.emplace_back();
            threads_data[i].thread_id = i;
            threads_data[i].flag_stopped = false;
        }
    }

    inline void pool_stop() {
        for (auto &thread_data : threads_data)
            thread_data.flag_stopped = true;
    }

    inline void clear_stack_pool() {
        for (auto &thread_data : threads_data)
            thread_data.clear_stack();
    }

    inline void clear_history_pool() {
        for (auto &thread_data : threads_data)
            thread_data.clear_history();
    }

    inline void set_fen_pool(std::string fen, bool chess960 = false) {
        for (auto &thread_data : threads_data) {
            thread_data.board.chess960 = chess960;
            thread_data.board.set_fen(fen);
        }
    }

    inline void set_dfrc_pool(int idx) {
        for (auto &thread_data : threads_data) {
            thread_data.board.chess960 = (idx > 0);
            thread_data.board.set_dfrc(idx);
        }
    }

    inline void make_move_pool(Move move) {
        for (auto &thread_data : threads_data)
            thread_data.board.make_move(move);
    }

    inline void clear_board_pool() {
        for (auto &thread_data : threads_data)
            thread_data.board.clear();
    }

    uint64_t get_total_nodes_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t nodes = 0;
        for (auto &thread_data : threads_data)
            nodes += thread_data.nodes;
        return nodes;
    }

    uint64_t get_total_tb_hits_pool() {
        std::lock_guard <std::mutex> lock(threads_mutex);
        uint64_t nodes = 0;
        for (auto &thread_data : threads_data)
            nodes += thread_data.tb_hits;
        return nodes;
    }

    void main_thread_handler(Info *info) {
        assert(!threads_data.empty());
        for (std::size_t i = 1; i < threads_data.size(); i++)
            threads.push_back(std::thread(&SearchData::start_search, &threads_data[i], info));
        threads_data[0].start_search(info);

        pool_stop();
        join_threads();
        threads.clear();

        int best_score = 0;
        Move best_move = NULLMOVE;
        
        int bestDepth = threads_data[0].completed_depth;
        best_score = threads_data[0].root_score[1];
        best_move = threads_data[0].best_move[1];
        for (std::size_t i = 1; i < threads_data.size(); i++) {
            if (threads_data[i].root_score[1] > best_score && threads_data[i].completed_depth >= bestDepth) {
                best_score = threads_data[i].root_score[1];
                best_move = threads_data[i].best_move[1];
                bestDepth = threads_data[i].completed_depth;
            }
        }

        if (printStats) {
            std::cout << "bestmove " << move_to_string(best_move, info->chess960);
            std::cout << std::endl;
        }
    }
};

ThreadPool thread_pool;