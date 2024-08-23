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
const int BUCKET = 3;

struct Entry {
    uint16_t hash;
    uint16_t about;
    int16_t score;
    int16_t eval;
    uint16_t move;
    inline void refresh(int gen) { about = (about & 1023u) | (gen << 10u); }

    inline int value(int ply) const {
        if (score != VALUE_NONE) {
            if (score >= TB_WIN_SCORE) return score - ply;
            else if (score <= -TB_WIN_SCORE) return score + ply;
        }
        return score;
    }

    inline int bound() const { return about & 3; }

    inline int depth() const { return (about >> 2) & 127; }

    inline bool wasPV() const { return (about >> 9) & 1; }

    inline int generation() const { return (about >> 10); }
};

struct Bucket {
    std::array<Entry, BUCKET> entries;
    char padding[2];
};

static_assert(sizeof(Bucket) == 32);

class HashTable {
public:
    Bucket* table;
    uint64_t buckets;

private:
    int generation = 1;

public:
    HashTable();

    inline void initTableSlice(uint64_t start, uint64_t size);

    void initTable(uint64_t size, int nr_threads = 1);

    ~HashTable();

    HashTable(const HashTable&) = delete;

    inline void prefetch(uint64_t hash);

    Entry* probe(uint64_t hash, bool &ttHit);

    void save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV);

    inline void resetAge();

    inline void age();

    int hashfull();
};

#ifndef GENERATE
HashTable* TT; /// shared hash table
#endif

HashTable::HashTable() {
    buckets = 0;
}

HashTable::~HashTable() {
    delete[] table;
}

void HashTable::initTableSlice(uint64_t start, uint64_t size) {
    memset(&table[start], 0, size * sizeof(Bucket));
}

void HashTable::initTable(uint64_t size, int nr_threads) {
    size /= sizeof(Bucket);

    if (buckets)
        delete[] table;

    if (size < sizeof(Bucket)) {
        buckets = 0;
        return;
    }
    else {
        buckets = size;
    }

    table = (Bucket*)malloc(buckets * sizeof(Bucket));

    std::vector <std::thread> threads(nr_threads);
    uint64_t start = 0;
    uint64_t slice_size = buckets / nr_threads + 1;

    for (auto& t : threads) {
        uint64_t good_size = std::min(buckets - start, slice_size);
        t = std::thread{ &HashTable::initTableSlice, this, start, good_size };
        start += slice_size;
    }
    for (auto& t : threads) {
        if (t.joinable())
           t.join();
    }
}

void HashTable::prefetch(uint64_t hash) {
    uint64_t ind = mul_hi(hash, buckets);
    Bucket* bucket = table + ind;
    __builtin_prefetch(bucket);
}

Entry* HashTable::probe(uint64_t hash, bool &ttHit) {
    uint64_t ind = mul_hi(hash, buckets);
    Entry* bucket = table[ind].entries.data();

    for (int i = 0; i < BUCKET; i++) {
        if (bucket[i].hash == (uint16_t)hash) {
            ttHit = 1;
            bucket[i].refresh(generation);
            return bucket + i;
        }
    }

    ttHit = 0;
    int idx = 0;
    for (int i = 1; i < BUCKET; i++) {
        if (bucket[i].depth() + bucket[i].generation() < bucket[idx].depth() + bucket[idx].generation()) idx = i;
    }

    return bucket + idx;
}

void HashTable::save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval, bool wasPV) {
    if (score != VALUE_NONE) {
        if (score >= TB_WIN_SCORE)
            score += ply;
        else if (score <= -TB_WIN_SCORE)
            score -= ply;
    }

    if (move || hash != entry->hash) entry->move = move;

    if (bound == EXACT || (uint16_t)hash != entry->hash || depth + 3 >= entry->depth()) {
        entry->hash = (uint16_t)hash;
        entry->score = score;
        entry->eval = eval;
        entry->about = uint16_t(bound | (depth << 2) | (wasPV << 9) | (generation << 10));
    }
}

void HashTable::resetAge() {
    generation = 1;

    for (uint64_t i = 0; i < buckets; i++)
        for (int j = 0; j < BUCKET; j++)
            table[i].entries[j].refresh(0);
}

void HashTable::age() {
    generation++;

    if (generation == 63) {
        generation = 1;

        for (uint64_t i = 0; i < buckets; i++)
            for (int j = 0; j < BUCKET; j++)
                table[i].entries[j].refresh(0);
    }
}

int HashTable::hashfull() {
    int tempSize = 1000, cnt = 0;

    for (int i = 0; i < tempSize; i++) {
        for (int j = 0; j < BUCKET; j++) {
            if (table[i].entries[j].generation() == generation)
                cnt++;
        }
    }

    return cnt / BUCKET;
}