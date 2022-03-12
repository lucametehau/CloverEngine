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

const int INPUT_NEURONS = 768;
const int HIDDEN_NEURONS = 512;

struct NetInput {
    std::vector <short> ind;
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

        updateSz = 0;
        updates.resize(5);

        load();
    }

    float calc(NetInput& input) { /// feed forward
        float sum;

        for (int n = 0; n < HIDDEN_NEURONS; n++) {
            sum = inputBiases[n];

            for (auto& prevN : input.ind) {
                sum += inputWeights[prevN][n];
            }

            histOutput[0][n] = sum;
        }

        histSz = 1;

        return getOutput();
    }

    void removeInput(int16_t ind) {
        updates[updateSz++] = { ind, -1 };
    }

    void addInput(int16_t ind) {
        updates[updateSz++] = { ind, 1 };
    }

    void applyUpdates() {
        memcpy(histOutput[histSz], histOutput[histSz - 1], sizeof(histOutput[histSz - 1]));
        histSz++;

        for (int i = 0; i < updateSz; i++) {
            __m256* v = (__m256*)inputWeights[updates[i].ind];
            __m256* w = (__m256*)histOutput[histSz - 1];
            __m256 ct = _mm256_set1_ps(updates[i].coef);

            for (int j = 0; j < batches; j++)
                w[j] = _mm256_add_ps(_mm256_mul_ps(ct, v[j]), w[j]);
        }

        updateSz = 0;
    }

    void revertUpdates() {
        histSz--;
    }

    float getOutput() {
        float sum = outputBias;

        __m256* v = (__m256*)outputWeights;
        __m256* w = (__m256*)histOutput[histSz - 1];
        __m256 zero = _mm256_setzero_ps();

        for (int j = 0; j < batches; j++) {
            __m256 temp = _mm256_mul_ps(_mm256_max_ps(w[j], zero), v[j]);
            float tempRes[8] __attribute__((aligned(16)));
            _mm256_store_ps(tempRes, temp);

            sum += tempRes[0] + tempRes[1] + tempRes[2] + tempRes[3] + tempRes[4] + tempRes[5] + tempRes[6] + tempRes[7];
        }


        return sum;
    }

    void load() {
        int* intData;
        float* floatData;
        Gradient* gradData;
        std::vector <Gradient> v;
        std::vector <float> w;

        int x;
        intData = (int*)gNetData;

        x = *(intData++);
        assert(x == 3);

        floatData = (float*)intData;

        int sz;

        sz = HIDDEN_NEURONS;

        for (int j = 0; j < sz; j++)
            inputBiases[j] = *(floatData++);

        gradData = (Gradient*)floatData;

        v.resize(sz);
        for (int j = 0; j < sz; j++)
            v[j] = *(gradData++);

        for (int i = 0; i < INPUT_NEURONS; i++) {
            floatData = (float*)gradData;
            for (int j = 0; j < sz; j++) {
                inputWeights[i][j] = *(floatData++);
            }

            gradData = (Gradient*)floatData;

            for (int j = 0; j < sz; j++) {
                v[j] = *(gradData++);
            }
        }

        sz = 1;
        floatData = (float*)gradData;

        outputBias = *(floatData++);

        gradData = (Gradient*)floatData;

        v[0] = *(gradData++);
         
        floatData = (float*)gradData;

        for (int j = 0; j < HIDDEN_NEURONS; j++) {
            outputWeights[j] = *(floatData++);
        }
    }

    int lg = sizeof(__m256) / sizeof(float);
    int batches = HIDDEN_NEURONS / lg;

    int histSz, updateSz;

    float inputBiases[HIDDEN_NEURONS], outputBias __attribute__((aligned(32)));
    float histOutput[1005][HIDDEN_NEURONS] __attribute__((aligned(32)));
    float inputWeights[INPUT_NEURONS][HIDDEN_NEURONS] __attribute__((aligned(32)));
    float outputWeights[HIDDEN_NEURONS] __attribute__((aligned(32)));

    std::vector <Update> updates;
};