#pragma once

#include "defs.h"
#include "move.h"

#include <algorithm>
#include <numeric>
#include <vector>

constexpr int MOVE_FRACTION_PLY = 3;

class MoveFractionEntry
{
  private:
    Key key;
    int nr_moves;
    MoveList moves;
    std::array<uint64_t, MAX_MOVES> move_nodes;
    uint64_t total_nodes;

  public:
    MoveFractionEntry() : key(0), nr_moves(0), total_nodes(0)
    {
    }

    void add_move(const Move move, const uint64_t nodes)
    {
        for (int i = 0; i < nr_moves; i++)
        {
            if (moves[i] == move)
            {
                move_nodes[i] += nodes;
                total_nodes += nodes;
                return;
            }
        }
        if (nr_moves < MAX_MOVES)
        {
            moves[nr_moves] = move;
            move_nodes[nr_moves] = nodes;
            total_nodes += nodes;
            nr_moves++;
        }
        // TODO: what if buffer gets full?
    }

    int score_move(const Move move) const
    {
        for (int i = 0; i < nr_moves; i++)
        {
            if (moves[i] == move)
                return 600.0 * move_nodes[i] / total_nodes * std::log(total_nodes);
        }
        return 0;
    }

    void print_moves() const
    {
        std::vector<int> idxs(nr_moves);
        std::iota(idxs.begin(), idxs.end(), 0);
        std::sort(idxs.begin(), idxs.end(), [&](int a, int b) { return move_nodes[a] > move_nodes[b]; });
        for (int i : idxs)
        {
            std::cout << moves[i].to_string() << ": " << move_nodes[i] * 100.0 / total_nodes << "% of the "
                      << total_nodes << " searched nodes; score of " << score_move(moves[i]) << "\n";
        }
        std::cout << std::endl;
    }
};

class MoveFractionTable
{
  private:
    static constexpr int TABLE_SIZE = 1 << 12;
    std::array<MoveFractionEntry, TABLE_SIZE> entries;

  public:
    MoveFractionTable()
    {
        entries.fill(MoveFractionEntry());
    }

    MoveFractionEntry *get_entry(const Key key)
    {
        return &entries[mul_hi(key, TABLE_SIZE)];
    }
};