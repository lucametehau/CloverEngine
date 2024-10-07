#pragma once

#include "defs.h"
#include <math.h>
#include <iostream>

constexpr int MOVE_FRACTION_PLIES = 2;
const float PI = acos(-1);

class MoveFraction {
private:
    int64_t nodes, total_nodes;

public:
    MoveFraction() : nodes(0), total_nodes(0) {}

    void update(int64_t delta_nodes, int64_t delta_total_nodes) { nodes += delta_nodes; total_nodes += delta_total_nodes; }

    inline float get_fraction() const { return total_nodes == 0 ? 0.0 : 1.0 * nodes / total_nodes; }
};

class MoveFractionEntry {
public:
    uint64_t key;
    MoveList moves;
    std::array<MoveFraction, 256> fractions;
    int nr_moves;

public:
    MoveFractionEntry() : key(0), nr_moves(0) {}

    MoveFractionEntry(uint64_t _key, MoveList &_moves, int _nr_moves) {
        key = _key, moves = _moves, nr_moves = _nr_moves;
        for (int i = 0; i < nr_moves; i++) fractions[i] = MoveFraction();
    }

    void init(uint64_t _key, MoveList &_moves, int _nr_moves) {
        key = _key, moves = _moves, nr_moves = _nr_moves;
        for (int i = 0; i < nr_moves; i++) fractions[i] = MoveFraction();
    }

    inline float get_fraction(Move move) {
        if (key == 0) return 0.0;
        for (int i = 0; i < nr_moves; i++) {
            if (moves[i] == move) return fractions[i].get_fraction();
        }
        return 0.0;
    }

    void update_fraction(Move move, uint64_t nodes, uint64_t total_nodes) {
        //std::cout << move_to_string(move, false) << " " << nodes << " " << total_nodes << "\n";
        for (int i = 0; i < nr_moves; i++) {
            if (moves[i] == move) {
                fractions[i].update(nodes, total_nodes);
                //std::cout << "Fraction found! " << fractions[i].get_fraction() << "\n";
                break;
            }
        }
    }

    inline int get_movepicker_score(Move move) {
        const float fraction = get_fraction(move);
        //const float weird_scale = 1.0 / (1.0 + exp(-total_nodes));
        assert(0.0 <= fraction && fraction <= 1.0);
        //std::cout << "Getting a score of " << 16384 * sin(fraction * PI / 2) << "\n";
        return 16384 * sin(fraction * PI / 2);
    }

    void print() {
        for (int i = 0; i < nr_moves; i++)
            std::cout << move_to_string(moves[i], false) << " " << 100.0 * fractions[i].get_fraction() << "\n";
    }
};

class MoveFractionTable {
private:
    static constexpr int TABLE_SIZE = (1 << 7);
    static constexpr int TABLE_MASK = TABLE_SIZE - 1;
    std::array<MoveFractionEntry, TABLE_SIZE> entries;

public:
    MoveFractionTable() {
        entries.fill(MoveFractionEntry());
    }

    MoveFractionEntry* get_entry(uint64_t key) {
        return &entries[key & TABLE_MASK];
    }

    void update_entry(MoveFractionEntry* entry, uint64_t key, MoveList& moves, int nr_moves) {
        if (entry->key != key) {
            entry->init(key, moves, nr_moves);
            //std::cout << entry->key << " " << key << " initing\n";
        }
    }
};