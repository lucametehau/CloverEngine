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
#pragma once
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <random>

INCBIN(Net, EVALFILE);

const int NO_ACTIV = 0;
const int SIGMOID = 1;
const int RELU = 2;
const int Q_MULT_1 = 32;
const int Q_MULT_2 = 512;

struct LayerInfo {
    int size;
    int activationType;
};

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
const double Momentum = 0.3;

const double SIGMOID_SCALE = 0.0075;

const int NO_ACTIV   = 0;
const int SIGMOID    = 1;
const int RELU       = 2;

namespace tools {
  std::mt19937_64 gen(time(0));
  std::uniform_real_distribution <double> rng(0, 1);
  std::uniform_int_distribution <int> bin(0, 1);

  std::vector <double> createRandomArray(int length) {
    std::vector <double> v;
    double k = sqrtf(2.0 / length);

    for(int i = 0; i < length; i++)
      v.push_back(rng(gen) * k);

    return v;
  }
}

struct LayerInfo {
  int size;
  int activationType;
};

struct NetInput {
  std::vector <short> ind;
};

struct FenOutput {
  double result;
  double eval;
};

class Layer {
public:
    Layer(LayerInfo _info, int prevNumNeurons) {
        info = _info;

        int numNeurons = _info.size;

        bias.resize(numNeurons);

        output.resize(numNeurons);

        if (prevNumNeurons) {
            weights.resize(prevNumNeurons);
            for (int i = 0; i < prevNumNeurons; i++) {
                weights[i].resize(numNeurons);
            }
        }
    }

    LayerInfo info;
    std::vector <float> bias;
    std::vector <float> output;
    std::vector <std::vector <float>> weights;
  Layer(LayerInfo _info, int prevNumNeurons) {
    info = _info;

    int numNeurons = _info.size;

    bias = tools::createRandomArray(numNeurons);

    output.resize(numNeurons);

    weights.resize(numNeurons);

    if(prevNumNeurons) {
      for(int i = 0; i < numNeurons; i++) {
        weights[i] = tools::createRandomArray(prevNumNeurons);
      }
    }
  }

  LayerInfo info;
  std::vector <double> bias;
  std::vector <double> output;
  std::vector <std::vector <double>> weights;
};

class Network {
public:

    Network() {
        std::vector <LayerInfo> topology;

        topology.push_back({ 768, NO_ACTIV });
        topology.push_back({ 256, RELU });
        topology.push_back({ 1, SIGMOID });

        for (int i = 0; i < (int)topology.size(); i++) {
            layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
        }

        histSz = 0;
        histOutput.resize(1000);

        updateSz = 0;
        updates.resize(5);

        load();
    }

    Network(std::vector <LayerInfo>& topology) {
        for (int i = 0; i < (int)topology.size(); i++) {
            layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
        }
    }

    float calc(NetInput& input) { /// feed forward
        float sum;

        for (int n = 0; n < layers[1].info.size; n++) {
            sum = layers[1].bias[n];

            for (auto& prevN : input.ind) {
                sum += layers[1].weights[prevN][n];
            }

            layers[1].output[n] = sum;
        }

        histSz = 0;

        histOutput[histSz++] = layers[1].output;

        return getOutput();
    }

    void removeInput(int16_t ind) {
        /*for(int n = 0; n < layers[1].info.size; n++) {
          layers[1].output[n] -= layers[1].weights[ind][n];
        }*/
        updates[updateSz++] = { ind, -1 };
    }

    void addInput(int16_t ind) {
        /*for(int n = 0; n < layers[1].info.size; n++) {
          layers[1].output[n] += layers[1].weights[ind][n];
        }*/
        updates[updateSz++] = { ind, 1 };
    }

    void applyUpdates() {
        histOutput[histSz] = histOutput[histSz - 1];
        histSz++;

        for (int i = 0; i < updateSz; i++) {
            const int c = updates[i].coef;
            for (int n = 0; n < layers[1].info.size; n++)
                histOutput[histSz - 1][n] += c * layers[1].weights[updates[i].ind][n];
        }

        updateSz = 0;
    }

    void revertUpdates() {
        histSz--;
    }

    float getOutput() {
        float sum = layers.back().bias[0];

        for (int n = 0; n < layers[1].info.size; n++)
            sum += std::max<float>(histOutput[histSz - 1][n], 0.0) * layers.back().weights[n][0];

        //std::cout << sum << "\n";

        return sum;
    }

    void load() {
        int* intData;
        float* floatData;
        Gradient* gradData;
        std::vector <Gradient> v;

        int x;
        intData = (int*)gNetData;

        x = *(intData++);
        assert(x == (int)layers.size());

        gradData = (Gradient*)intData;

        for (int i = 0; i < (int)layers.size(); i++) {
            int sz = layers[i].info.size;
            v.resize(sz);

            floatData = (float*)gradData;
            for (int j = 0; j < sz; j++) {
                //std::cout << *floatData << " ";
                layers[i].bias[j] = *(floatData++);
            }

            gradData = (Gradient*)floatData;
            for (int j = 0; j < sz; j++) {
                v[j] = *(gradData++);
            }

            for (int j = 0; i && j < layers[i - 1].info.size; j++) {
                floatData = (float*)gradData;
                for (int k = 0; k < sz; k++) {
                    layers[i].weights[j][k] = *(floatData++);
                }

                //std::cout << "\n";

                gradData = (Gradient*)floatData;
                for (int k = 0; k < sz; k++) {
                    v[k] = *(gradData++);
                }
            }
        }
    }

    int histSz, updateSz;
    std::vector <Layer> layers;
    std::vector <std::vector <float>> histOutput;
    std::vector <Update> updates;
};
  Network() {
    std::vector <LayerInfo> topology;

    topology.push_back({768, NO_ACTIV});
    topology.push_back({256, RELU});
    topology.push_back({1, SIGMOID});

    for(int i = 0; i < (int)topology.size(); i++) {
      layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
    }

    load("Clover_20mil_d8_e64.nn");
  }

  Network(std::vector <LayerInfo> &topology) {
    for(int i = 0; i < (int)topology.size(); i++) {
      layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
    }
  }

  double calc(NetInput &input) { /// feed forward
    double sum;

    for(int n = 0; n < layers[1].info.size; n++) {
      sum = layers[1].bias[n];

      for(auto &prevN : input.ind) {
        sum += layers[1].weights[n][prevN];
      }

      layers[1].output[n] = sum;
    }

    return getOutput();
  }

  void removeInput(int ind) {
    for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] -= layers[1].weights[n][ind];
    }
  }

  void addInput(int ind) {
    for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] += layers[1].weights[n][ind];
    }
  }

  double getOutput() {
    double sum = layers.back().bias[0];

    for(int n = 0; n < layers[1].info.size; n++)
      sum += std::max(0.0, layers[1].output[n]) * layers.back().weights[0][n];

    return sum;
  }

  void checkRemoval() {
    NetInput input;
    input.ind.push_back(1);
    addInput(1);
    double ans = calc(input);
    input.ind.push_back(2);
    addInput(2);
    calc(input);
    removeInput(2);
    input.ind.pop_back();
    double ans2 = getOutput(), ans3 = calc(input);
    std::cout << ans2 << " " << ans << " " << ans3 << "\n";

    assert(abs(ans2 - ans) < 1e-9);
  }

  std::vector <double> read(int lg, std::ifstream &in) {
    std::vector <double> v;
    double x;
    for(int i = 0; i < lg; i++)
      in >> x, v.push_back(x);
    return v;
  }

  void load(std::string path) {
    std::ifstream in (path);
    int cnt = layers.size(), cnt2 = 0;

    in >> cnt2;

    if(cnt2 != cnt) {
      std::cout << "Can't load network!\n";
      std::cout << "Expected " << cnt << ", got " << cnt2 << "\n";
      assert(0);
    }

    std::vector <double> temp;

    for(int i = 0; i < (int)layers.size(); i++) {
      int sz = layers[i].info.size;
      layers[i].bias = read(sz, in);
      layers[i].output = read(sz, in);
      temp = read(sz, in);
      temp = read(sz, in);

      for(int j = 0; j < sz && i; j++) {
        layers[i].weights[j] = read(layers[i - 1].info.size, in);
      }
    }
  }

  std::vector <Layer> layers;
};
