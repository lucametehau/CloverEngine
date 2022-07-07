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
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <random>
#include <immintrin.h>

INCBIN(Net, EVALFILE);

const int INPUT_NEURONS = 1536;
const int SIDE_NEURONS = 512;
const int HIDDEN_NEURONS = 2 * SIDE_NEURONS;

const int Q = 32;

struct NetInput {
    std::vector <short> ind[2];
};

struct Update {
    int16_t ind;
    int8_t coef;
};

class Gradient {
public:
    //float grad;
    float m1, m2; /// momentums

    Gradient(float _m1, float _m2) {
        m1 = _m1;
        m2 = _m2;
        //grad = 0;
    }

    Gradient() {
        m1 = m2 = 0;
        //grad = 0;
    }
};

class Network {
public:

    Network() {
        histSz = 0;

        updateSz[WHITE] = updateSz[BLACK] = 0;

        load();
    }

    // the below functions are taken from here and are used to compute the dot product
    // https://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-sse-vector-sum-or-other-reduction

    float hsum_ps_sse3(__m128 v) {
        __m128 shuf = _mm_movehdup_ps(v);
        __m128 sums = _mm_add_ps(v, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    }

    float hsum256_ps_avx(__m256 v) {
        __m128 vlow = _mm256_castps256_ps128(v);
        __m128 vhigh = _mm256_extractf128_ps(v, 1);
        vlow = _mm_add_ps(vlow, vhigh);
        return hsum_ps_sse3(vlow);
    }

    float calc(NetInput& input, bool stm) {
        float sum;

        for (int n = 0; n < SIDE_NEURONS; n++) {
            sum = inputBiases[n];

            for (auto& prevN : input.ind[WHITE]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[WHITE][0][n] = sum;

            sum = inputBiases[n];

            for (auto& prevN : input.ind[BLACK]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[BLACK][0][n] = sum;
        }

        histSz = 1;

        return getOutput(stm);
    }

    float getOutput(NetInput& input, bool stm) { /// feed forward
        float sum;

        for (int n = 0; n < SIDE_NEURONS; n++) {
            sum = inputBiases[n];

            for (auto& prevN : input.ind[WHITE]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[WHITE][0][n] = sum;

            sum = inputBiases[n];

            for (auto& prevN : input.ind[BLACK]) {
                sum += inputWeights[prevN][n];
            }

            histOutput[BLACK][0][n] = sum;

            if (stm == WHITE)
                deeznuts[n] = histOutput[WHITE][0][n], deeznuts[n + SIDE_NEURONS] = histOutput[BLACK][0][n];
            else
                deeznuts[n] = histOutput[BLACK][0][n], deeznuts[n + SIDE_NEURONS] = histOutput[WHITE][0][n];
        }

        sum = outputBias;

        __m256 zero = _mm256_setzero_ps();
        __m256 acc = _mm256_setzero_ps();

        __m256* v = (__m256*)outputWeights;
        __m256* w = (__m256*)histOutput[stm][histSz - 1];

        for (int j = 0; j < batches; j++) {
            acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_max_ps(w[j], zero), v[j]));
        }

        __m256* w2 = (__m256*)histOutput[stm ^ 1][histSz - 1];

        for (int j = 0; j < batches; j++) {
            acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_max_ps(w2[j], zero), v[j + batches]));
        }

        sum += hsum256_ps_avx(acc);

        return sum;
    }

    void removeInput(int side, int16_t ind) {
        updates[side][updateSz[side]++] = { ind, -1 };
    }

    void removeInput(int piece, int sq, int kingWhite, int kingBlack) {
        updates[WHITE][updateSz[WHITE]++] = { netInd(piece, sq, kingWhite, WHITE), -1 };
        updates[BLACK][updateSz[BLACK]++] = { netInd(piece, sq, kingBlack, BLACK), -1 };
    }

    void addInput(int side, int16_t ind) {
        updates[side][updateSz[side]++] = { ind, 1 };
    }

    void addInput(int piece, int sq, int kingWhite, int kingBlack) {
        updates[WHITE][updateSz[WHITE]++] = { netInd(piece, sq, kingWhite, WHITE), 1 };
        updates[BLACK][updateSz[BLACK]++] = { netInd(piece, sq, kingBlack, BLACK), 1 };
    }

    void applyUpdates(int c) {
        memcpy(histOutput[c][histSz], histOutput[c][histSz - 1], sizeof(histOutput[c][histSz - 1]));

        __m256* w = (__m256*)histOutput[c][histSz];

        for (int i = 0; i < updateSz[c]; i++) {
            __m256* v = (__m256*)inputWeights[updates[c][i].ind];
            __m256 ct = _mm256_set1_ps(updates[c][i].coef);

            for (int j = 0; j < batches; j++)
                w[j] = _mm256_add_ps(_mm256_mul_ps(ct, v[j]), w[j]);
        }

        updateSz[c] = 0;
    }

    void applyInitUpdates(int c) {
        memcpy(histOutput[c][histSz], inputBiases, sizeof(histOutput[c][histSz]));
        __m256* w = (__m256*)histOutput[c][histSz];

        for (int i = 0; i < updateSz[c]; i++) {
            __m256* v = (__m256*)inputWeights[updates[c][i].ind];
            __m256 ct = _mm256_set1_ps(updates[c][i].coef);

            for (int j = 0; j < batches; j++)
                w[j] = _mm256_add_ps(_mm256_mul_ps(ct, v[j]), w[j]);
        }

        updateSz[c] = 0;
    }

    void revertUpdates() {
        histSz--;
    }

    float getOutput(bool stm) {
        float sum = outputBias;

        __m256 zero = _mm256_setzero_ps();
        __m256 acc = _mm256_setzero_ps();

        __m256* w = (__m256*)histOutput[stm][histSz - 1];

        for (int j = 0; j < batches; j++) {
            acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_max_ps(w[j], zero), v[j]));
        }

        __m256* w2 = (__m256*)histOutput[stm ^ 1][histSz - 1];

        for (int j = 0; j < batches; j++) {
            acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_max_ps(w2[j], zero), v[j + batches]));
        }

        sum += hsum256_ps_avx(acc);

        return sum;
    }

    void load() {
        int* intData;
        float* floatData;
        Gradient* gradData;
        Gradient kekw;
        std::vector <float> w;

        int x;
        intData = (int*)gNetData;

        x = *(intData++);
        assert(x == 3);

        floatData = (float*)intData;

        int sz;

        sz = SIDE_NEURONS;

        for (int j = 0; j < sz; j++)
            inputBiases[j] = *(floatData++);

        gradData = (Gradient*)floatData;

        for (int j = 0; j < sz; j++)
            kekw = *(gradData++);

        floatData = (float*)gradData;

        for (int i = 0; i < SIDE_NEURONS * INPUT_NEURONS; i++) {
            inputWeights[i / SIDE_NEURONS][i % SIDE_NEURONS] = *(floatData++);
        }

        gradData = (Gradient*)floatData;

        for (int i = 0; i < SIDE_NEURONS * INPUT_NEURONS; i++) {
            kekw = *(gradData++);
        }

        sz = 1;
        floatData = (float*)gradData;

        outputBias = *(floatData++);

        gradData = (Gradient*)floatData;

        kekw = *(gradData++);

        floatData = (float*)gradData;

        for (int j = 0; j < HIDDEN_NEURONS; j++) {
            outputWeights[j] = *(floatData++);
        }
    }

    int lg = sizeof(__m256) / sizeof(float);
    int batches = SIDE_NEURONS / lg;

    int histSz, updateSz[2];

    float inputBiases[SIDE_NEURONS] __attribute__((aligned(32)));
    float outputBias;
    float histOutput[2][2005][SIDE_NEURONS] __attribute__((aligned(32)));
    float inputWeights[INPUT_NEURONS][SIDE_NEURONS] __attribute__((aligned(32)));
    float outputWeights[HIDDEN_NEURONS] __attribute__((aligned(32)));
    float deeznuts[HIDDEN_NEURONS] __attribute__((aligned(32)));

    __m256* v = (__m256*)outputWeights;

    Update updates[2][105];
};