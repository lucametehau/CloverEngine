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
#include <vector>
#include <cstring>

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

const reg_type zero{};
const reg_type one = reg_set1(Q_IN);

alignas(ALIGN) inline int16_t inputWeights[INPUT_NEURONS * SIDE_NEURONS];
alignas(ALIGN) inline int16_t inputBiases[SIDE_NEURONS];
alignas(ALIGN) inline int16_t outputWeights[HIDDEN_NEURONS];
inline int16_t outputBias;

inline int get_king_bucket_cache_index(const Square king_sq, const bool side) {
    return KING_BUCKETS * ((king_sq & 7) >= 4) + kingIndTable[king_sq.mirror(side)];
}

enum {
    SUB = 0, ADD
};

INCBIN(Net, EVALFILE);

void load_nnue_weights() {
    int16_t* intData = (int16_t*)gNetData;

    for (int i = 0; i < SIDE_NEURONS * INPUT_NEURONS; i++) {
        inputWeights[(i / SIDE_NEURONS) * SIDE_NEURONS + (i % SIDE_NEURONS)] = *intData;
        intData++;
    }
    for (int j = 0; j < SIDE_NEURONS; j++) {
        inputBiases[j] = *intData;
        intData++;
    }
    for (int j = 0; j < HIDDEN_NEURONS; j++) {
        outputWeights[j] = *intData;
        intData++;
    }
    outputBias = *intData;
}

inline reg_type reg_clamp(reg_type reg) {
    return reg_min16(reg_max16(reg, zero), one);
}

inline int32_t get_sum(reg_type_s& x) {
#if   defined(__AVX512F__)
    __m256i reg_256 = _mm256_add_epi32(_mm512_castsi512_si256(x), _mm512_extracti32x8_epi32(x, 1));
    __m128i a = _mm_add_epi32(_mm256_castsi256_si128(reg_256), _mm256_extractf128_si256(reg_256, 1));
#elif defined(__AVX2__)
    __m128i a = _mm_add_epi32(_mm256_castsi256_si128(x), _mm256_extractf128_si256(x, 1));
#elif defined(__SSE2__) || defined(__AVX__)
    __m128i a = x;
#endif

#if   defined(__ARM_NEON)
    return x;
#else
    __m128i b = _mm_add_epi32(a, _mm_srli_si128(a, 8));
    __m128i c = _mm_add_epi32(b, _mm_srli_si128(b, 4));
    return _mm_cvtsi128_si32(c);
#endif
}

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
    Network() {
        hist_size = 0;
        for (auto c : { BLACK, WHITE }) {
            for (int i = 0; i < 2 * KING_BUCKETS; i++) {
                memcpy(cached_states[c][i].output, inputBiases, sizeof(inputBiases));
                cached_states[c][i].bb.fill(Bitboard(0ull));
            }
        }
    }

    void add_input(int ind) { add_ind[add_size++] = ind; }
    void remove_input(int ind) { sub_ind[sub_size++] = ind; }
    void clear_updates() { add_size = sub_size = 0; }
    
    int32_t calc(NetInput& input, bool stm) {
        int32_t sum;

        for (int n = 0; n < SIDE_NEURONS; n++) {
            sum = inputBiases[n];
            for (auto& prevN : input.ind[WHITE]) {
                sum += inputWeights[prevN * SIDE_NEURONS + n];
            }
            assert(-32768 <= sum && sum <= 32767);
            output_history[0][SIDE_NEURONS + n] = sum;

            sum = inputBiases[n];
            for (auto& prevN : input.ind[BLACK]) {
                sum += inputWeights[prevN * SIDE_NEURONS + n];
            }
            assert(-32768 <= sum && sum <= 32767);
            output_history[0][n] = sum;
        }

        hist_size = 1;
        hist[0].calc[0] = hist[0].calc[1] = 1;

        return get_output(stm);
}

    void apply_updates(int16_t* output, int16_t* input) {
        reg_type regs[UNROLL_LENGTH];

        for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
            const int offset = b * BUCKET_UNROLL;
            const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);
            for (int idx = 0; idx < add_size; idx++) {
                reg_type* reg = reinterpret_cast<reg_type*>(&inputWeights[add_ind[idx] * SIDE_NEURONS + offset]);
                for (int i = 0; i < UNROLL_LENGTH; i++)
                    regs[i] = reg_add16(regs[i], reg[i]);
            }

            for (int idx = 0; idx < sub_size; idx++) {
                reg_type* reg = reinterpret_cast<reg_type*>(&inputWeights[sub_ind[idx] * SIDE_NEURONS + offset]);
                for (int i = 0; i < UNROLL_LENGTH; i++)
                    regs[i] = reg_sub16(regs[i], reg[i]);
            }

            reg_type* reg_out = (reg_type*)&output[offset];
            for (int i = 0; i < UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void apply_sub_add(int16_t* output, int16_t* input, int ind1, int ind2) {
        reg_type regs[UNROLL_LENGTH];
        const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&inputWeights[ind1 * SIDE_NEURONS]);
        const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&inputWeights[ind2 * SIDE_NEURONS]);

        for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
            const int offset = b * BUCKET_UNROLL;
            const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);

            reg_type* reg_out = (reg_type*)&output[offset];
            for (int i = 0; i < UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void apply_sub_add_sub(int16_t* output, int16_t* input, int ind1, int ind2, int ind3) {
        reg_type regs[UNROLL_LENGTH];
        const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&inputWeights[ind1 * SIDE_NEURONS]);
        const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&inputWeights[ind2 * SIDE_NEURONS]);
        const int16_t* inputWeights3 = reinterpret_cast<const int16_t*>(&inputWeights[ind3 * SIDE_NEURONS]);

        for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
            const int offset = b * BUCKET_UNROLL;
            const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);
            const reg_type* reg3 = reinterpret_cast<const reg_type*>(&inputWeights3[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg3[i]);

            reg_type* reg_out = (reg_type*)&output[offset];
            for (int i = 0; i < UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void apply_sub_add_sub_add(int16_t* output, int16_t* input, int ind1, int ind2, int ind3, int ind4) {
        reg_type regs[UNROLL_LENGTH];
        const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&inputWeights[ind1 * SIDE_NEURONS]);
        const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&inputWeights[ind2 * SIDE_NEURONS]);
        const int16_t* inputWeights3 = reinterpret_cast<const int16_t*>(&inputWeights[ind3 * SIDE_NEURONS]);
        const int16_t* inputWeights4 = reinterpret_cast<const int16_t*>(&inputWeights[ind4 * SIDE_NEURONS]);

        for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
            const int offset = b * BUCKET_UNROLL;
            const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);
            const reg_type* reg3 = reinterpret_cast<const reg_type*>(&inputWeights3[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg3[i]);
            const reg_type* reg4 = reinterpret_cast<const reg_type*>(&inputWeights4[offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg4[i]);

            reg_type* reg_out = (reg_type*)&output[offset];
            for (int i = 0; i < UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void process_move(Move move, Piece piece, Piece captured, Square king, bool side, int16_t* a, int16_t* b) {
        Square from = move.get_from(), to = move.get_to();
        const bool turn = piece.color();
        switch (move.get_type()) {
        case NO_TYPE: {
            if (captured == NO_PIECE)
                apply_sub_add(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side));
            else
                apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(captured, to, king, side));
        }
        break;
        case MoveTypes::CASTLE: {
            Square rFrom = to, rTo;
            Piece rPiece = Piece(PieceTypes::ROOK, turn);
            if (to > from) { // king side castle
                to = Squares::G1.mirror(turn);
                rTo = Squares::F1.mirror(turn);
            }
            else { // queen side castle
                to = Squares::C1.mirror(turn);
                rTo = Squares::D1.mirror(turn);
            }
            apply_sub_add_sub_add(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(rPiece, rFrom, king, side), net_index(rPiece, rTo, king, side));
        }
        break;
        case MoveTypes::ENPASSANT: {
            const Square pos = shift_square<SOUTH>(turn, to);
            const Piece pieceCap = Piece(PieceTypes::PAWN, 1 ^ turn);
            apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(pieceCap, pos, king, side));
        }
        break;
        default: {
            const int promPiece = Piece(move.get_prom() + PieceTypes::KNIGHT, turn);
            if (captured == NO_PIECE)
                apply_sub_add(a, b, net_index(piece, from, king, side), net_index(promPiece, to, king, side));
            else
                apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(promPiece, to, king, side), net_index(captured, to, king, side));
        }
        break;
        }
    }

    void process_historic_update(const int index, const Square king_sq, const bool side) {
        hist[index].calc[side] = 1;
        process_move(hist[index].move, hist[index].piece, hist[index].cap, king_sq, side,
            &output_history[index][side * SIDE_NEURONS], &output_history[index - 1][side * SIDE_NEURONS]);
    }

    void add_move_to_history(Move move, Piece piece, Piece captured) {
        hist[hist_size] = { move, piece, captured, piece.type() == PieceTypes::KING && recalc(move.get_from(), move.get_special_to() , piece.color()), { 0, 0 } };
        hist_size++;
    }

    void revert_move() { hist_size--; }

    int get_computed_parent(const bool c) {
        int i = hist_size - 1;
        while (!hist[i].calc[c]) {
            if (hist[i].piece.color() == c && hist[i].recalc)
                return -1;
            i--;
        }
        return i;
    }

    void bring_up_to_date(Board &board) {
        for (auto side : { BLACK, WHITE }) {
            if (!hist[hist_size - 1].calc[side]) {
                int last_computed_pos = get_computed_parent(side) + 1;
                const int king_sq = board.get_king(side);
                if (last_computed_pos) { // no full refresh required
                    while (last_computed_pos < hist_size) {
                        process_historic_update(last_computed_pos, king_sq, side);
                        last_computed_pos++;
                    }
                }
                else {
                    KingBucketState* state = &cached_states[side][get_king_bucket_cache_index(king_sq, side)];
                    clear_updates();
                    for (Piece i = Pieces::BlackPawn; i <= Pieces::WhiteKing; i++) {
                        Bitboard prev = state->bb[i];
                        Bitboard curr = board.bb[i];

                        Bitboard b = curr & ~prev; // additions
                        while (b) add_input(net_index(i, b.get_square_pop(), king_sq, side));

                        b = prev & ~curr; // removals
                        while (b) remove_input(net_index(i, b.get_square_pop(), king_sq, side));

                        state->bb[i] = curr;
                    }
                    apply_updates(state->output, state->output);
                    memcpy(&output_history[hist_size - 1][side * SIDE_NEURONS], state->output, SIDE_NEURONS * sizeof(int16_t));
                    hist[hist_size - 1].calc[side] = 1;
                }
            }
        }
    }

    int32_t get_output(bool stm) {
        reg_type_s acc{};
        const reg_type* w = reinterpret_cast<const reg_type*>(&output_history[hist_size - 1][stm * SIDE_NEURONS]);
        const reg_type* w2 = reinterpret_cast<const reg_type*>(&output_history[hist_size - 1][(stm ^ 1) * SIDE_NEURONS]);
        const reg_type* v = reinterpret_cast<const reg_type*>(outputWeights);
        const reg_type* v2 = reinterpret_cast<const reg_type*>(&outputWeights[SIDE_NEURONS]);
        reg_type clamped;

        for (int j = 0; j < NUM_REGS; j++) {
            clamped = reg_clamp(w[j]);
            acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v[j]), clamped));
            clamped = reg_clamp(w2[j]);
            acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v2[j]), clamped));
        }

        return (outputBias + get_sum(acc) / Q_IN) * 225 / Q_IN_HIDDEN;
    }

    int hist_size;
    int add_size, sub_size;

    alignas(ALIGN) int16_t output_history[STACK_SIZE][HIDDEN_NEURONS];
    MultiArray<KingBucketState, 2, 2 * KING_BUCKETS> cached_states;

    std::array<int16_t, 32> add_ind, sub_ind;
    std::array<NetHist, STACK_SIZE> hist;
};