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
        if (score >= TB_WIN_SCORE)
            return score - ply;
        else if (score <= -TB_WIN_SCORE)
            return score + ply;
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

    bool probe(uint64_t hash, Entry& entry);

    void save(uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV);

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
    //initTable(128LL * MB);
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

    table = new Entry[entries * BUCKET + BUCKET]();

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

inline bool HashTable::probe(uint64_t hash, Entry& entry) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;

    for (int i = 0; i < BUCKET; i++) {
        if ((bucket + i)->hash == hash) {
            (bucket + i)->refresh(generation);
            entry = *(bucket + i);
            return 1;
        }
    }

    return 0;
}

inline void HashTable::save(uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;

    if (score >= TB_WIN_SCORE)
        score += ply;
    else if (score <= -TB_WIN_SCORE)
        score -= ply;

    Entry temp;

    temp.move = move; temp.eval = short(eval); temp.score = short(score);
    temp.about = uint16_t(bound | (depth << 2) | (wasPV << 9) | (generation << 10));
    temp.hash = hash;

    if (bucket->hash == hash) {
        if (bound == EXACT || depth >= bucket->depth() - 3) {
            if (move == NULLMOVE)
                temp.move = bucket->move;
            *bucket = temp;
        }
        return;
    }

    Entry* replace = bucket;

    for (int i = 1; i < BUCKET; i++) {
        if ((bucket + i)->hash == hash) {
            if (bound == EXACT || depth >= bucket->depth() - 3) {
                if (move == NULLMOVE)
                    temp.move = (bucket + i)->move;
                *(bucket + i) = temp;
            }
            return;
        }
        else if ((bucket + i)->about < replace->about) {
            replace = (bucket + i);
        }
    }

    if (move == NULLMOVE)
        temp.move = replace->move;
    *replace = temp;
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