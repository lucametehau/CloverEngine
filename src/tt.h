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
#include "defs.h"
#include <thread>

const int MB = (1 << 20);
const int BUCKET = 4;

struct Entry {
    uint64_t hash;
    uint16_t about;
    int16_t score;
    int16_t eval;
    uint16_t move;
    void refresh(int gen) {
        about = (about & 1023u) | (gen << 10u);
    }

    int value(int ply) const {
        if (score != VALUE_NONE) {
            if (score >= TB_WIN_SCORE)
                return score - ply;
            else if (score <= -TB_WIN_SCORE)
                return score + ply;
        }
        return score;
    }

    int bound() const {
        return about & 3;
    }

    int depth() const {
        return (about >> 2) & 127;
    }

    bool wasPV() const {
        return (about >> 9) & 1;
    }

    int generation() const {
        return (about >> 10);
    }

};

class HashTable {
public:
    Entry* table;
    uint64_t entries;
    int generation = 1;

    HashTable();

    void initTableSlice(uint64_t start, uint64_t size);

    void initTable(uint64_t size, int nr_threads = 1);

    ~HashTable();

    HashTable(const HashTable&) = delete;

    void prefetch(uint64_t hash);

    Entry* probe(uint64_t hash, bool &ttHit);

    void save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV);

    void resetAge();

    void age();

    int tableFull();
};

#ifndef GENERATE
HashTable* TT; /// shared hash table
#endif

inline uint64_t pow2(uint64_t size) {
    if (size & (size - 1)) {
        for (unsigned i = 1; i < 64; i++)
            size |= size >> i;
        size++;
        size >>= 1;
    }
    return size;
}

inline HashTable::HashTable() {
    entries = 0;
}

inline HashTable::~HashTable() {
    delete[] table;
}

void HashTable::initTableSlice(uint64_t start, uint64_t size) {
    memset(&table[start], 0, size * sizeof(Entry));
}

inline void HashTable::initTable(uint64_t size, int nr_threads) {
    size /= (sizeof(Entry) * BUCKET);
    size = pow2(size);

    if (entries)
        delete[] table;

    if (size < sizeof(Entry)) {
        entries = 0;
        return;
    }
    else {
        entries = size - 1;
    }

    table = (Entry*)malloc((entries * BUCKET + BUCKET) * sizeof(Entry));

    std::vector <std::thread> threads(nr_threads);
    uint64_t start = 0;
    uint64_t slice_size = (entries * BUCKET + BUCKET) / nr_threads + 1;

    for (auto& t : threads) {
        uint64_t good_size = std::min(entries * BUCKET + BUCKET - start, slice_size);
        t = std::thread{ &HashTable::initTableSlice, this, start, good_size };
        start += slice_size;
    }
    for (auto& t : threads)
        t.join();
}

inline void HashTable::prefetch(uint64_t hash) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;
    __builtin_prefetch(bucket);
}

inline Entry* HashTable::probe(uint64_t hash, bool &ttHit) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;

    for (int i = 0; i < BUCKET; i++) {
        if (bucket[i].hash == hash) {
            ttHit = 1;
            bucket[i].refresh(generation);
            return bucket + i;
        }
    }

    ttHit = 0;
    int idx = 0;
    for (int i = 1; i < BUCKET; i++) {
        if (bucket[i].depth() + bucket[i].generation() < bucket[idx].depth() + bucket[idx].generation()) {
            idx = i;
        }
    }

    return bucket + idx;
}

inline void HashTable::save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV) {
    if (score != VALUE_NONE) {
        if (score >= TB_WIN_SCORE)
            score += ply;
        else if (score <= -TB_WIN_SCORE)
            score -= ply;
    }

    if (move || hash != entry->hash) entry->move = move;

    if (bound == EXACT || hash != entry->hash || depth + 3 >= entry->depth()) {
        entry->hash = hash;
        entry->score = score;
        entry->eval = eval;
        entry->about = uint16_t(bound | (depth << 2) | (wasPV << 9) | (generation << 10));
    }
}

inline void HashTable::resetAge() {
    generation = 1;

    for (uint64_t i = 0; i < (uint64_t)entries * BUCKET; i++)
        table[i].refresh(0);
}

inline void HashTable::age() {
    generation++;

    if (generation == 63) {
        generation = 1;

        for (uint64_t i = 0; i < (uint64_t)entries * BUCKET; i++)
            table[i].refresh(0);
    }
}

inline int HashTable::tableFull() {
    int tempSize = 4000, divis = 4, cnt = 0;

    for (int i = 0; i < tempSize; i++) {
        if (table[i].generation() == generation)
            cnt++;
    }

    return cnt / divis;
}