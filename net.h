#pragma once
#include "incbin.h"
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <random>

INCBIN(Net, EVALFILE);

const int NO_ACTIV   = 0;
const int SIGMOID    = 1;
const int RELU       = 2;

struct LayerInfo {
  int size;
  int activationType;
};

struct NetInput {
  std::vector <short> ind;
};

class Gradient {
public:
  float m1, m2; /// momentums

  Gradient(float _m1, float _m2) {
    m1 = _m1;
    m2 = _m2;
  }

  Gradient() {
    m1 = m2 = 0;
  }
};

class Layer {
public:
  Layer(LayerInfo _info, int prevNumNeurons) {
    info = _info;

    int numNeurons = _info.size;

    bias.resize(numNeurons);

    output.resize(numNeurons);

    if(prevNumNeurons) {
      weights.resize(prevNumNeurons);
      for(int i = 0; i < prevNumNeurons; i++) {
        weights[i].resize(numNeurons);
      }
    }
  }

  LayerInfo info;
  std::vector <float> bias;
  std::vector <float> output;
  std::vector <std::vector <float>> weights;
};

class Network {
public:

  Network() {
    std::vector <LayerInfo> topology;

    topology.push_back({768, NO_ACTIV});
    topology.push_back({512, RELU});
    topology.push_back({1, SIGMOID});

    for(int i = 0; i < (int)topology.size(); i++) {
      layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
    }

    load();
  }

  Network(std::vector <LayerInfo> &topology) {
    for(int i = 0; i < (int)topology.size(); i++) {
      layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
    }
  }

  float calc(NetInput &input) { /// feed forward
    float sum;

    for(int n = 0; n < layers[1].info.size; n++) {
      sum = layers[1].bias[n];

      for(auto &prevN : input.ind) {
        sum += layers[1].weights[prevN][n];
      }

      layers[1].output[n] = sum;
    }

    return getOutput();
  }

  void removeInput(int ind) {
    for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] -= layers[1].weights[ind][n];
    }
  }

  void addInput(int ind) {
    for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] += layers[1].weights[ind][n];
    }
  }

  float getOutput() {
    float sum = layers.back().bias[0];

    for(int n = 0; n < layers[1].info.size; n++)
      sum += std::max(0.0f, layers[1].output[n]) * layers.back().weights[n][0];

    return sum;
  }

  void checkRemoval() {
    NetInput input;
    input.ind.push_back(1);
    addInput(1);
    float ans = calc(input);
    input.ind.push_back(2);
    addInput(2);
    calc(input);
    removeInput(2);
    input.ind.pop_back();
    float ans2 = getOutput(), ans3 = calc(input);
    std::cout << ans2 << " " << ans << " " << ans3 << "\n";

    assert(abs(ans2 - ans) < 1e-9);
  }

  void load() {
    int *intData;
    float *floatData;
    Gradient *gradData;
    std::vector <Gradient> v;

    int x;
    intData = (int*)gNetData;

    x = *(intData++);
    assert(x == (int)layers.size());

    gradData = (Gradient*)intData;

    for(int i = 0; i < (int)layers.size(); i++) {
      int sz = layers[i].info.size;
      v.resize(sz);

      floatData = (float*)gradData;
      for(int j = 0; j < sz; j++) {
        layers[i].bias[j] = *(floatData++);
      }

      gradData = (Gradient*)floatData;
      for(int j = 0; j < sz; j++) {
        v[j] = *(gradData++);
      }

      for(int j = 0; i && j < layers[i - 1].info.size; j++) {
        floatData = (float*)gradData;
        for(int k = 0; k < sz; k++) {
          layers[i].weights[j][k] = *(floatData++);
        }

        gradData = (Gradient*)floatData;
        for(int k = 0; k < sz; k++) {
          v[k] = *(gradData++);
        }
      }
    }
  }

  std::vector <Layer> layers;
};

