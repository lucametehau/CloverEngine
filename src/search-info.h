#pragma once
#include "defs.h"
#include <chrono>
#include <cstdint>

const auto t_init = std::chrono::steady_clock::now();

inline std::time_t get_current_time()
{
    auto t = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t - t_init).count();
}

class Info
{
  private:
    std::time_t start_time;
    std::time_t recommended_soft_limit, soft_limit, hard_limit;
    int depth, multipv;
    int64_t nodes_lim, min_nodes, max_nodes;

    bool timeset, chess960;

  public:
    Info() : depth(MAX_DEPTH), multipv(1), nodes_lim(-1), min_nodes(-1), max_nodes(-1), chess960(false)
    {
    }

    void init()
    {
        timeset = false;
        nodes_lim = min_nodes = max_nodes = -1;
        depth = MAX_DEPTH;
        start_time = get_current_time();
    }

    constexpr int get_depth_limit() const
    {
        return depth;
    }
    constexpr int get_multipv() const
    {
        return multipv;
    }
    constexpr bool is_chess960() const
    {
        return chess960;
    }

    void set_soft_limit(std::time_t time)
    {
        soft_limit = time;
    }
    void set_recommended_soft_limit(float coef)
    {
        recommended_soft_limit = soft_limit * coef;
    }
    void set_hard_limit(std::time_t time)
    {
        hard_limit = time;
    }

    void set_nodes(int64_t nodes)
    {
        nodes_lim = nodes;
    }
    void set_min_nodes(int64_t nodes)
    {
        min_nodes = nodes;
    }
    void set_max_nodes(int64_t nodes)
    {
        max_nodes = nodes;
    }

    void set_depth(int _depth)
    {
        depth = _depth;
    }
    void set_multipv(int _multipv)
    {
        multipv = _multipv;
    }
    void set_chess960(bool _chess960)
    {
        chess960 = _chess960;
    }

    void set_time(std::time_t time, std::time_t inc)
    {
        std::time_t time_with_inc = time + 40 * inc;
        timeset = true;
        soft_limit = std::min<int>(time_with_inc * TMCoef1, time * TMCoef2);
        hard_limit = std::min<int>(soft_limit * TMCoef3, time * TMCoef4);
    }

    void set_movetime(std::time_t time)
    {
        timeset = true;
        soft_limit = -1;
        hard_limit = time;
    }

    const std::time_t get_time_elapsed() const
    {
        return get_current_time() - start_time;
    }

    const bool soft_limit_passed() const
    {
        return timeset && soft_limit != -1 && get_time_elapsed() >= recommended_soft_limit;
    }
    const bool hard_limit_passed() const
    {
        return timeset && get_time_elapsed() >= hard_limit;
    }
    constexpr bool min_nodes_passed(int64_t nodes) const
    {
        return min_nodes != -1 && nodes >= min_nodes;
    }
    constexpr bool max_nodes_passed(int64_t nodes) const
    {
        return max_nodes != -1 && nodes >= max_nodes;
    }
    constexpr bool nodes_limit_passed(int64_t nodes) const
    {
        return nodes_lim != -1 && nodes >= nodes_lim;
    }
};