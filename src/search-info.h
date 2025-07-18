#pragma once
#include "defs.h"
#include <chrono>
#include <cstdint>

inline const auto t_init = std::chrono::steady_clock::now();

inline std::time_t get_current_time()
{
    auto t = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t - t_init).count();
}

class Info
{
  private:
    std::time_t m_start_time;
    std::time_t m_recommended_soft_limit, m_soft_limit, m_hard_limit;
    int m_depth, m_multipv;
    int64_t m_nodes, m_min_nodes, m_max_nodes;

    bool m_timeset, m_san_mode, m_chess960;

  public:
    Info() : m_depth(MAX_DEPTH), m_multipv(1), m_nodes(-1), m_min_nodes(-1), m_max_nodes(-1), m_chess960(false)
    {
    }

    void init()
    {
        m_timeset = m_san_mode = false;
        m_nodes = m_min_nodes = m_max_nodes = -1;
        m_depth = MAX_DEPTH;
        m_start_time = get_current_time();
    }

    int get_depth_limit() const
    {
        return m_depth;
    }
    int get_multipv() const
    {
        return m_multipv;
    }
    bool is_san_mode() const
    {
        return m_san_mode;
    }
    bool is_chess960() const
    {
        return m_chess960;
    }

    void set_soft_limit(std::time_t time)
    {
        m_soft_limit = time;
    }
    void set_recommended_soft_limit(float coef)
    {
        m_recommended_soft_limit = m_soft_limit * coef;
    }
    void set_hard_limit(std::time_t time)
    {
        m_hard_limit = time;
    }

    void set_nodes(int64_t nodes)
    {
        m_nodes = nodes;
    }
    void set_min_nodes(int64_t nodes)
    {
        m_min_nodes = nodes;
    }
    void set_max_nodes(int64_t nodes)
    {
        m_max_nodes = nodes;
    }

    void set_depth(int depth)
    {
        m_depth = depth;
    }
    void set_multipv(int multipv)
    {
        m_multipv = multipv;
    }
    void set_chess960(bool chess960)
    {
        m_chess960 = chess960;
    }
    void set_san_mode()
    {
        m_san_mode = true;
    }

    void set_time(std::time_t time, std::time_t inc)
    {
        std::time_t time_with_inc = time + 40 * inc;
        m_timeset = true;
        m_soft_limit = std::min<int>(time_with_inc * TMCoef1, time * TMCoef2);
        m_hard_limit = std::min<int>(m_soft_limit * TMCoef3, time * TMCoef4);
    }

    void set_movetime(std::time_t time)
    {
        m_timeset = true;
        m_soft_limit = -1;
        m_hard_limit = time;
    }

    std::time_t get_time_elapsed() const
    {
        return get_current_time() - m_start_time;
    }

    bool soft_limit_passed() const
    {
        return m_timeset && m_soft_limit != -1 && get_time_elapsed() >= m_recommended_soft_limit;
    }
    bool hard_limit_passed() const
    {
        return m_timeset && get_time_elapsed() >= m_hard_limit;
    }
    bool min_nodes_passed(int64_t nodes) const
    {
        return m_min_nodes != -1 && nodes >= m_min_nodes;
    }
    bool max_nodes_passed(int64_t nodes) const
    {
        return m_max_nodes != -1 && nodes >= m_max_nodes;
    }
    bool nodes_limit_passed(int64_t nodes) const
    {
        return m_nodes != -1 && nodes >= m_nodes;
    }
};