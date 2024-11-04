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

#ifdef _MSC_VER
#ifndef __clang__
#error MSVC not supported
#endif
#undef _MSC_VER // clang on windows
#endif

#include "incbin.h"
#include "defs.h"
#include "board.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

#if defined(__AVX512F__)
#define reg_type    __m512i
#define reg_type_s  __m512i
#define reg_set1    _mm512_set1_epi16
#define reg_add16   _mm512_add_epi16
#define reg_sub16   _mm512_sub_epi16
#define reg_max16   _mm512_max_epi16
#define reg_min16   _mm512_min_epi16
#define reg_add32   _mm512_add_epi32
#define reg_mullo   _mm512_mullo_epi16
#define reg_madd16  _mm512_madd_epi16
#define reg_load    _mm512_load_si512
#define reg_save    _mm512_store_si512
#define ALIGN       64
#elif defined(__AVX2__) 
#define reg_type    __m256i
#define reg_type_s  __m256i
#define reg_set1    _mm256_set1_epi16
#define reg_add16   _mm256_add_epi16
#define reg_sub16   _mm256_sub_epi16
#define reg_min16   _mm256_min_epi16
#define reg_max16   _mm256_max_epi16
#define reg_add32   _mm256_add_epi32
#define reg_mullo   _mm256_mullo_epi16
#define reg_madd16  _mm256_madd_epi16
#define reg_load    _mm256_load_si256
#define reg_save    _mm256_store_si256
#define ALIGN       32
#elif defined(__SSE2__) || defined(__AVX__)
#define reg_type    __m128i
#define reg_type_s  __m128i
#define reg_set1    _mm_set1_epi16
#define reg_add16   _mm_add_epi16
#define reg_sub16   _mm_sub_epi16
#define reg_max16   _mm_max_epi16
#define reg_min16   _mm_min_epi16
#define reg_add32   _mm_add_epi32
#define reg_mullo   _mm_mullo_epi16
#define reg_madd16  _mm_madd_epi16
#define reg_load    _mm_load_si128
#define reg_save    _mm_store_si128
#define ALIGN       16
#elif defined(__ARM_NEON)
#define reg_type          int16_t
#define reg_type_s        int32_t
#define reg_set1          int16_t
#define reg_add16(a, b)   ((a) + (b))
#define reg_sub16(a, b)   ((a) - (b))
#define reg_max16(a, b)   ((a) > (b) ? (a) : (b))  
#define reg_min16(a, b)   ((a) < (b) ? (a) : (b))   
#define reg_add32(a, b)   ((a) + (b))
#define reg_mullo(a, b)   ((a) * (b))
#define reg_madd16(a, b)  ((a) * (b))
#define reg_load(a)       (*(a))
#define reg_save(a, b)    (*(a)) = (b)
#define ALIGN       16
#endif

constexpr int KING_BUCKETS = 7;
constexpr int INPUT_NEURONS = 768 * KING_BUCKETS;
constexpr int SIDE_NEURONS = 1280;
constexpr int HIDDEN_NEURONS = 2 * SIDE_NEURONS;
constexpr int REG_LENGTH = sizeof(reg_type) / sizeof(int16_t);
constexpr int NUM_REGS = SIDE_NEURONS / REG_LENGTH;
constexpr int BUCKET_UNROLL = 128;
constexpr int UNROLL_LENGTH = BUCKET_UNROLL / REG_LENGTH;

constexpr int Q_IN = 255;
constexpr int Q_HIDDEN = 64;
constexpr int Q_IN_HIDDEN = Q_IN * Q_HIDDEN;

extern void load_nnue_weights();

inline int get_king_bucket_cache_index(const Square king_sq, const bool side) {
    return KING_BUCKETS * ((king_sq & 7) >= 4) + kingIndTable[king_sq.mirror(side)];
}

enum {
    SUB = 0, ADD
};

struct NetworkWeights {
    alignas(ALIGN) int16_t inputWeights[INPUT_NEURONS * SIDE_NEURONS];
    alignas(ALIGN) int16_t inputBiases[SIDE_NEURONS];
    alignas(ALIGN) int16_t outputWeights[HIDDEN_NEURONS];
    int16_t outputBias;
};

struct NetHist {
    Move move;
    Piece piece, cap;
    bool recalc;
    bool calc[2];
};

struct KingBucketState {
    alignas(ALIGN) int16_t output[SIDE_NEURONS];
    std::array<Bitboard, 12> bb;
};

class Network {
public:
    Network();

    void add_input(int ind);
    void remove_input(int ind);
    void clear_updates();
    
    int32_t calc(NetInput& input, bool stm);

    void apply_updates(int16_t* output, int16_t* input);
    void apply_sub_add(int16_t* output, int16_t* input, int ind1, int ind2);
    void apply_sub_add_sub(int16_t* output, int16_t* input, int ind1, int ind2, int ind3);
    void apply_sub_add_sub_add(int16_t* output, int16_t* input, int ind1, int ind2, int ind3, int ind4);

    void process_move(Move move, Piece piece, Piece captured, Square king, bool side, int16_t* a, int16_t* b);
    void process_historic_update(const int index, const Square king_sq, const bool side);
    void add_move_to_history(Move move, Piece piece, Piece captured);
    void revert_move();

    int get_computed_parent(const bool c);
    void bring_up_to_date(Board &board);

    int32_t get_output(bool stm);

    int hist_size;
    int add_size, sub_size;

    alignas(ALIGN) int16_t output_history[STACK_SIZE][HIDDEN_NEURONS];
    MultiArray<KingBucketState, 2, 2 * KING_BUCKETS> cached_states;

    std::array<int16_t, 32> add_ind, sub_ind;
    std::array<NetHist, STACK_SIZE> hist;
};