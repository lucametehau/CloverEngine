#pragma once
#include "defs.h"
#include "move.h"
#include <vector>

class RootMove
{
  public:
    bool lowerbound, upperbound;
    bool searched;
    MoveList pv;
    int pv_len;
    int16_t score, search_score;
    int sel_depth;
    uint64_t nodes_searched;

  public:
    constexpr RootMove(Move move) : pv_len(1)
    {
        pv[0] = move;
        score = -INF;
        search_score = 0;
        nodes_searched = 0;
        lowerbound = upperbound = false;
        searched = false;
    }

    constexpr bool operator<(const RootMove &other) const
    {
        return score > other.score; // descending
    }
};

class RootMoves
{
  public:
    std::vector<RootMove> root_moves;

  public:
    RootMoves() = default;

    RootMoves(MoveList &moves, int nr_moves)
    {
        for (int i = 0; i < nr_moves; i++)
            root_moves.push_back(RootMove(moves[i]));
    }

    RootMove &get_root_move(Move move)
    {
        return *std::find_if(root_moves.begin(), root_moves.end(),
                             [move](const RootMove &rm) { return rm.pv[0] == move; });
    }

    constexpr bool move_was_searched(Move move) const
    {
        return std::any_of(root_moves.begin(), root_moves.end(),
                           [move](const RootMove &rm) { return rm.pv[0] == move && rm.searched; });
    }

    constexpr RootMove operator[](int index) const
    {
        return root_moves[index];
    }

    RootMove &operator[](int index)
    {
        return root_moves[index];
    }

    void sort()
    {
        std::sort(root_moves.begin(), root_moves.end());
    }

    constexpr bool empty() const
    {
        return root_moves.empty();
    }
};