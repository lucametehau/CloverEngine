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

const int MB = (1 << 20);
const int BUCKET = 4;

namespace tt {
    struct Entry {
        uint64_t hash;
        union {
            struct {
                uint16_t about;
                int16_t score;
                int16_t eval;
                uint16_t move;
            } info;
        };
        void refresh(int gen) {
            info.about = (info.about & 1023u) | (gen << 10u);
        }

        int value(int ply) {
            if (info.score >= TB_WIN_SCORE)
                return info.score - ply;
            else if (info.score <= -TB_WIN_SCORE)
                return info.score + ply;
            return info.score;
        }

        int bound() {
            return info.about & 3u;
        }

        int depth() {
            return (info.about >> 2u) & 255u;
        }

        int generation() {
            return info.about >> 10u;
        }
    };

    class HashTable {
    public:
        Entry* table;
        uint64_t entries;
        int generation = 1;

        HashTable();

        void initTable(uint64_t size);

        ~HashTable();

        HashTable(const HashTable&) = delete;

        void prefetch(uint64_t hash);

        bool probe(uint64_t hash, Entry& entry);

        void save(uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval);

        void resetAge();

        void age();

        int tableFull();
    };
}

tt::HashTable* TT; /// shared hash table

inline uint64_t pow2(uint64_t size) {
    if (size & (size - 1)) {
        for (unsigned i = 1; i < 64; i++)
            size |= size >> i;
        size++;
        size >>= 1;
    }
    return size;
}

inline tt::HashTable::HashTable() {
    entries = 0;
    initTable(128LL * MB);
}

inline tt::HashTable :: ~HashTable() {
    delete[] table;
}

inline void tt::HashTable::initTable(uint64_t size) {
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
    memset(table, 0, entries * BUCKET + BUCKET);
}

inline void tt::HashTable::prefetch(uint64_t hash) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;
    __builtin_prefetch(bucket);
}

inline bool tt::HashTable::probe(uint64_t hash, Entry& entry) {
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

inline void tt::HashTable::save(uint64_t hash, int score, int depth, int ply, int bound, uint16_t move, int eval) {
    uint64_t ind = (hash & entries) * BUCKET;
    Entry* bucket = table + ind;

    if (score >= TB_WIN_SCORE)
        score += ply;
    else if (score <= -TB_WIN_SCORE)
        score -= ply;

    Entry temp = {};

    temp.info.move = move; temp.info.eval = short(eval); temp.info.score = short(score);
    temp.info.about = uint16_t(bound | (depth << 2u) | (generation << 10u));
    temp.hash = hash;

    if (bucket->hash == hash) {
        if (bound == EXACT || depth >= bucket->depth() - 3)
            *bucket = temp;
        return;
    }

    tt::Entry* replace = bucket;

    for (int i = 1; i < BUCKET; i++) {
        if ((bucket + i)->hash == hash) {
            if (bound == EXACT || depth >= bucket->depth() - 3) {
                *(bucket + i) = temp;
            }
            return;
        }
        else if ((bucket + i)->info.about < replace->info.about) {
            replace = (bucket + i);
        }
    }

    *replace = temp;
}

inline void tt::HashTable::resetAge() {
    generation = 1;

    for (uint64_t i = 0; i < (uint64_t)entries * BUCKET; i++)
        table[i].refresh(0);
}

inline void tt::HashTable::age() {
    generation++;

    if (generation == 63) {
        generation = 1;

        for (uint64_t i = 0; i < (uint64_t)entries * BUCKET; i++)
            table[i].refresh(0);
    }
}

inline int tt::HashTable::tableFull() {
    int tempSize = 4000, divis = 4, cnt = 0;

    for (int i = 0; i < tempSize; i++) {
        if (table[i].generation() == generation)
            cnt++;
    }

    return cnt / divis;
}