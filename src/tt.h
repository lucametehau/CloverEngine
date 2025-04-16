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

constexpr int MB = (1 << 20);
constexpr int BUCKET = 3;

enum TTBounds : int {
    NONE = 0, UPPER, LOWER, EXACT
};

struct Entry {
    uint16_t hash;
    uint16_t about;
    int16_t score;
    int16_t eval;
    Move move;

    Entry() : hash(0), about(0), score(0), eval(0), move(NULLMOVE) {}

    int value(int ply) const {
        if (score != VALUE_NONE) {
            if (score >= TB_WIN_SCORE) return score - ply;
            else if (score <= -TB_WIN_SCORE) return score + ply;
        }
        return score;
    }

    int bound() const { return about & 3; }
    int depth() const { return (about >> 2) & 127; }
    bool was_pv() const { return (about >> 9) & 1; }
    int generation() const { return (about >> 10); }
    void refresh(int gen) { about = (about & 1023u) | (gen << 10u); }
};

struct Bucket {
    std::array<Entry, BUCKET> entries;
    char padding[2];

    Bucket() { entries.fill(Entry()); }
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

    void initTableSlice(uint64_t start, uint64_t size);

    void initTable(uint64_t size, int nr_threads = 1);

    ~HashTable();

    HashTable(const HashTable&) = delete;

    void prefetch(const uint64_t hash);

    Entry* probe(const uint64_t hash, bool &ttHit);

    void save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, Move move, int eval, bool was_pv);

    void reset_age();

    void age();

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
    std::fill(table + start, table + start + size, Bucket());
}

void HashTable::initTable(uint64_t size, int nr_threads) {
    std::cout << "info string initializing TT with " << size << " bytes and " << nr_threads << " threads\n";
    if (size < sizeof(Bucket)) {
        if (buckets != 0) {
            delete[] table;
            table = nullptr;
            buckets = 0;
        }
        return;
    }

    const uint64_t new_buckets = size / sizeof(Bucket);
    
    if (buckets != new_buckets) {
        delete[] table;
        table = new Bucket[new_buckets];
        buckets = new_buckets;
    }

    nr_threads = std::clamp(nr_threads, 1, static_cast<int>(buckets));
    const uint64_t slice_size = (buckets + nr_threads - 1) / nr_threads;

    std::vector<std::thread> threads;
    threads.reserve(nr_threads);

    for (int i = 0; i < nr_threads; ++i) {
        threads.emplace_back(&HashTable::initTableSlice, this, slice_size * i, std::min(slice_size * (i + 1), buckets) - slice_size * i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

uint64_t mul_hi(const uint64_t a, const uint64_t b) {
    using uint128_t = unsigned __int128;
    return (static_cast<uint128_t>(a) * static_cast<uint128_t>(b)) >> 64;
}

void HashTable::prefetch(const uint64_t hash) {
    const uint64_t ind = mul_hi(hash, buckets);
    Bucket* bucket = table + ind;
    __builtin_prefetch(bucket);
}

Entry* HashTable::probe(const uint64_t hash, bool &ttHit) {
    const uint64_t ind = mul_hi(hash, buckets);
    Entry* bucket = table[ind].entries.data();
    const uint16_t hash16 = static_cast<uint16_t>(hash);

    for (int i = 0; i < BUCKET; i++) {
        if (bucket[i].hash == hash16) {
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

void HashTable::save(Entry* entry, uint64_t hash, int score, int depth, int ply, int bound, Move move, int eval, bool was_pv) {
    if (score != VALUE_NONE) {
        if (score >= TB_WIN_SCORE)
            score += ply;
        else if (score <= -TB_WIN_SCORE)
            score -= ply;
    }

    const uint16_t hash16 = static_cast<uint16_t>(hash);

    if (move || hash16 != entry->hash) entry->move = move;

    if (bound == TTBounds::EXACT || hash16 != entry->hash || depth + 3 + 2 * was_pv >= entry->depth()) {
        entry->hash = hash16;
        entry->score = score;
        entry->eval = eval;
        entry->about = uint16_t(bound | (depth << 2) | (was_pv << 9) | (generation << 10));
    }
}

void HashTable::reset_age() {
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