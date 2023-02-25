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
#include "incbin.h"
#include <windows.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <immintrin.h>

#if defined(__AVX512F__)
#define reg_type    __m512i
#define reg_add16   _mm512_add_epi16
#define reg_sub16   _mm512_sub_epi16
#define reg_max16   _mm512_max_epi16
#define reg_add32   _mm512_add_epi32
#define reg_madd16  _mm512_madd_epi16
#define ALLIGN      64
#elif defined(__AVX2__) || defined(__AVX__)
#define reg_type    __m256i
#define reg_add16   _mm256_add_epi16
#define reg_sub16   _mm256_sub_epi16
#define reg_max16   _mm256_max_epi16
#define reg_add32   _mm256_add_epi32
#define reg_madd16  _mm256_madd_epi16
#define ALLIGN      32
#elif defined(__SSE2__)
#define reg_type    __m128i
#define reg_add16   _mm_add_epi16
#define reg_sub16   _mm_sub_epi16
#define reg_max16   _mm_max_epi16
#define reg_add32   _mm_add_epi32
#define reg_madd16  _mm_madd_epi16
#define ALLIGN      16
#endif

INCBIN(Net, EVALFILE);

const int INPUT_NEURONS = 3072;
const int SIDE_NEURONS = 768;
const int HIDDEN_NEURONS = 2 * SIDE_NEURONS;

const int Q_IN = 2;
const int Q_HIDDEN = 512;

struct NetInput {
    std::vector <short> ind[2];
};

struct Update {
    int16_t ind;
    int8_t coef;
};

struct LightUpdate {
    int piece, sq;
    int8_t coef;
};

class Network {
public:

    Network() {
        histSz = 0;

        updateSz[WHITE] = updateSz[BLACK] = 0;

        load();
    }

    int32_t get_sum(reg_type& x) {
#if defined(__AVX512F__)
        __m256i reg_256 = _mm256_add_epi32(_mm512_castsi512_si256(x), _mm512_extracti32x8_epi32(x, 1));
        __m128i a = _mm_add_epi32(_mm256_castsi256_si128(reg_256), _mm256_extractf128_si256(reg_256, 1));
#elif defined(__AVX2__) || defined(__AVX__)
        __m128i a = _mm_add_epi32(_mm256_castsi256_si128(x), _mm256_extractf128_si256(x, 1));
#else
        __m128i a = x;
#endif
        __m128i b = _mm_add_epi32(a, _mm_srli_si128(a, 8));
        __m128i c = _mm_add_epi32(b, _mm_srli_si128(b, 4));

        return _mm_cvtsi128_si32(c);
    }

    int32_t calc(NetInput& input, bool stm) {
        int32_t sum;

        for (int n = 0; n < SIDE_NEURONS; n++) {
            sum = inputBiases[n];

            for (auto& prevN : input.ind[WHITE]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[0][WHITE][n] = sum;

            sum = inputBiases[n];

            for (auto& prevN : input.ind[BLACK]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[0][BLACK][n] = sum;

            assert(-32768 <= sum && sum <= 32767);
        }

        histSz = 1;

        return getOutput(stm);
    }

    int32_t getOutput(NetInput& input, bool stm) { /// feed forward
        int32_t sum;

        for (int n = 0; n < SIDE_NEURONS; n++) {
            sum = inputBiases[n];

            for (auto& prevN : input.ind[WHITE]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[0][WHITE][n] = sum;

            sum = inputBiases[n];

            for (auto& prevN : input.ind[BLACK]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[0][BLACK][n] = sum;

            assert(-32768 <= sum && sum <= 32767);
        }

        sum = outputBias * Q_IN;

        reg_type zero{};
        reg_type acc{}, acc2{};

        reg_type* v = (reg_type*)outputWeights;
        reg_type* w = (reg_type*)histOutput[stm][0];

        for (int j = 0; j < batches / 2; j++) {
            acc = reg_add32(acc, reg_madd16(reg_max16(w[2 * j], zero), v[2 * j]));
            acc2 = reg_add32(acc2, reg_madd16(reg_max16(w[2 * j + 1], zero), v[2 * j + 1]));
        }

        reg_type* w2 = (reg_type*)histOutput[stm ^ 1][0];

        for (int j = 0; j < batches / 2; j++) {
            acc = reg_add32(acc, reg_madd16(reg_max16(w2[2 * j], zero), v[2 * j + batches]));
            acc2 = reg_add32(acc2, reg_madd16(reg_max16(w2[2 * j + 1], zero), v[2 * j + 1 + batches]));
        }

        acc = reg_add32(acc, acc2);

        sum += get_sum(acc);

        return sum / Q_IN / Q_HIDDEN;
    }

    void removeInput(int side, int16_t ind) {
        updates[side][updateSz[side]++] = { ind, -1 };
    }

    void removeInput(int piece, int sq, int kingWhite, int kingBlack) {
        lightUpdates[histSz][lightUpdatesSz[histSz]++] = { piece, sq, -1 };
        updates[WHITE][updateSz[WHITE]++] = { netInd(piece, sq, kingWhite, WHITE), -1 };
        updates[BLACK][updateSz[BLACK]++] = { netInd(piece, sq, kingBlack, BLACK), -1 };
    }

    void addInput(int side, int16_t ind) {
        updates[side][updateSz[side]++] = { ind, 1 };
    }

    void addInput(int piece, int sq, int kingWhite, int kingBlack) {
        lightUpdates[histSz][lightUpdatesSz[histSz]++] = { piece, sq, 1 };
        updates[WHITE][updateSz[WHITE]++] = { netInd(piece, sq, kingWhite, WHITE), 1 };
        updates[BLACK][updateSz[BLACK]++] = { netInd(piece, sq, kingBlack, BLACK), 1 };
    }

    void apply(int16_t* a, int updatesCnt, Update* updates) {
        reg_type* w = (reg_type*)a;
        for (int i = 0; i < updatesCnt; i++) {
            reg_type* v = (reg_type*)inputWeights[updates[i].ind];

            if (updates[i].coef == 1) {
                for (int j = 0; j < batches; j += 4) {
                    w[j] = reg_add16(w[j], v[j]);
                    w[j + 1] = reg_add16(w[j + 1], v[j + 1]);
                    w[j + 2] = reg_add16(w[j + 2], v[j + 2]);
                    w[j + 3] = reg_add16(w[j + 3], v[j + 3]);
                }
            }
            else {
                for (int j = 0; j < batches; j += 4) {
                    w[j] = reg_sub16(w[j], v[j]);
                    w[j + 1] = reg_sub16(w[j + 1], v[j + 1]);
                    w[j + 2] = reg_sub16(w[j + 2], v[j + 2]);
                    w[j + 3] = reg_sub16(w[j + 3], v[j + 3]);
                }
            }
        }
    }

    void applyUpdates(int c) {
        memcpy(histOutput[histSz][c], histOutput[histSz - 1][c], sizeof(int16_t) * SIDE_NEURONS);
        apply(histOutput[histSz][c], updateSz[c], updates[c]);
        updateSz[c] = 0;
        lightUpdatesSz[histSz + 1] = 0;
    }

    void applyInitUpdates(int c) {
        memcpy(histOutput[histSz][c], inputBiases, sizeof(int16_t) * SIDE_NEURONS);
        apply(histOutput[histSz][c], updateSz[c], updates[c]);
        updateSz[c] = 0;
        lightUpdatesSz[histSz + 1] = 0;
    }

    void revertUpdates() {
        histSz--;
        lightUpdatesSz[histSz] = 0;
    }

    void applyUpdatesFromPrev(int c, int dif, int kingSq) {
        memcpy(histOutput[histSz][c], histOutput[histSz - dif - 1][c], sizeof(int16_t) * SIDE_NEURONS);
        int nrUpdates = 0;
        Update realUpdates[1005];
        for (int i = histSz - dif; i <= histSz; i++) {
            for (int j = 0; j < lightUpdatesSz[i]; j++) {
                realUpdates[nrUpdates++] = { netInd(lightUpdates[i][j].piece, lightUpdates[i][j].sq, kingSq, c), lightUpdates[i][j].coef };
            }
        }
        apply(histOutput[histSz][c], nrUpdates, realUpdates);
        updateSz[c] = 0;
    }

    int32_t getOutput(bool stm) {
        int32_t sum = outputBias * Q_IN;

        reg_type zero{};
        reg_type acc0{}, acc1{};
        reg_type acc2{}, acc3{};

        reg_type* w = (reg_type*)histOutput[histSz - 1][stm];

        for (int j = 0; j < batches; j += 4) {
            acc0 = reg_add32(acc0, reg_madd16(reg_max16(w[j], zero), v[j]));
            acc1 = reg_add32(acc1, reg_madd16(reg_max16(w[j + 1], zero), v[j + 1]));
            acc2 = reg_add32(acc2, reg_madd16(reg_max16(w[j + 2], zero), v[j + 2]));
            acc3 = reg_add32(acc3, reg_madd16(reg_max16(w[j + 3], zero), v[j + 3]));
        }

        reg_type* w2 = (reg_type*)histOutput[histSz - 1][stm ^ 1];

        for (int j = 0; j < batches; j += 4) {
            acc0 = reg_add32(acc0, reg_madd16(reg_max16(w2[j], zero), v[j + batches]));
            acc1 = reg_add32(acc1, reg_madd16(reg_max16(w2[j + 1], zero), v[j + 1 + batches]));
            acc2 = reg_add32(acc2, reg_madd16(reg_max16(w2[j + 2], zero), v[j + 2 + batches]));
            acc3 = reg_add32(acc3, reg_madd16(reg_max16(w2[j + 3], zero), v[j + 3 + batches]));
        }

        acc0 = reg_add32(acc0, acc1);
        acc2 = reg_add32(acc2, acc3);
        acc0 = reg_add32(acc0, acc2);

        sum += get_sum(acc0);

        return sum / Q_IN / Q_HIDDEN;
    }

    void load() {
        int* intData;
        float* floatData;
        std::vector <float> w;

        int x;
        intData = (int*)gNetData;

        x = *(intData++);
        assert(x == 3);

        floatData = (float*)intData;

        int sz;

        sz = SIDE_NEURONS;

        for (int j = 0; j < sz; j++) {
            float val = *(floatData++);
            inputBiases[j] = round(val * Q_IN);
        }

        for (int i = 0; i < SIDE_NEURONS * INPUT_NEURONS; i++) {
            float val = *(floatData++);
            inputWeights[i / SIDE_NEURONS][i % SIDE_NEURONS] = round(val * Q_IN);
        }

        sz = 1;

        outputBias = round(*(floatData++) * Q_HIDDEN);

        for (int j = 0; j < HIDDEN_NEURONS; j++) {
            float val = *(floatData++);
            outputWeights[j] = round(val * Q_HIDDEN);
        }
    }

    int lg = sizeof(reg_type) / sizeof(int16_t);
    int batches = SIDE_NEURONS / lg;

    int histSz;

    int16_t inputBiases[SIDE_NEURONS] __attribute__((aligned(ALLIGN)));
    int32_t outputBias;
    int16_t histOutput[2005][2][SIDE_NEURONS] __attribute__((aligned(ALLIGN)));
    int16_t inputWeights[INPUT_NEURONS][SIDE_NEURONS] __attribute__((aligned(ALLIGN)));
    int16_t outputWeights[HIDDEN_NEURONS] __attribute__((aligned(ALLIGN)));
    int16_t deeznuts[HIDDEN_NEURONS] __attribute__((aligned(ALLIGN)));

    reg_type* v = (reg_type*)outputWeights;

    int updateSz[2];
    Update updates[2][105];
    int lightUpdatesSz[2005];
    LightUpdate lightUpdates[2005][5];
    //int kingSq[2005];
};