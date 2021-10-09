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

const int NO_ACTIV = 0;
const int SIGMOID  = 1;
const int RELU     = 2;

struct LayerInfo {
  int size;
  int activationType;
};

struct NetInput {
  std::vector <short> ind;
};

class Gradient {
public:
  double m1, m2; /// momentums

  Gradient(double _m1, double _m2) {
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
  std::vector <double> bias;
  std::vector <double> output;
  std::vector <std::vector <double>> weights;
};

class Network {
public:

  Network() {
    std::vector <LayerInfo> topology;

    topology.push_back({768, NO_ACTIV});
    topology.push_back({256, RELU});
    topology.push_back({1, SIGMOID});

    for(int i = 0; i < (int)topology.size(); i++) {
      layers.push_back(Layer(topology[i], (i > 0 ? topology[i - 1].size : 0)));
    }

    histSz = 0;
    histOutput.resize(1000);

    load();
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
        sum += layers[1].weights[prevN][n];
      }

      layers[1].output[n] = sum;
    }

    histSz = 0;

    histOutput[histSz++] = layers[1].output;

    return getOutput();
  }

  void removeInput(int ind) {
    /*for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] -= layers[1].weights[ind][n];
    }*/
    updates.push_back(std::make_pair(ind, -1));
  }

  void addInput(int ind) {
    /*for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] += layers[1].weights[ind][n];
    }*/
    updates.push_back(std::make_pair(ind, 1));
  }

  void applyUpdates() {
    histOutput[histSz] = histOutput[histSz - 1];
    histSz++;

    for(auto &u : updates) {
      for(int n = 0; n < layers[1].info.size; n++)
        histOutput[histSz - 1][n] += u.second * layers[1].weights[u.first][n];
    }

    updates.clear();
  }

  void revertUpdates() {
    histSz--;
  }

  double getOutput() {
    double sum = layers.back().bias[0];

    for(int n = 0; n < layers[1].info.size; n++)
      sum += std::max(0.0, histOutput[histSz - 1][n]) * layers.back().weights[n][0];

    return sum;
  }

  void load() {
    int *intData;
    double *doubleData;
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

      doubleData = (double*)gradData;
      for(int j = 0; j < sz; j++) {
        //std::cout << *doubleData << " ";
        layers[i].bias[j] = *(doubleData++);
        //std::cout << layers[i].bias[j] << " " << layers[i].bias[j] / K_MULT << "\n";
      }

      gradData = (Gradient*)doubleData;
      for(int j = 0; j < sz; j++) {
        v[j] = *(gradData++);
      }

      for(int j = 0; i && j < layers[i - 1].info.size; j++) {
        doubleData = (double*)gradData;
        for(int k = 0; k < sz; k++) {
          layers[i].weights[j][k] = *(doubleData++);
        }

        gradData = (Gradient*)doubleData;
        for(int k = 0; k < sz; k++) {
          v[k] = *(gradData++);
        }
      }
    }
  }

  int histSz;
  std::vector <Layer> layers;
  std::vector <std::vector <double>> histOutput;
  std::vector <std::pair <int, int8_t>> updates;
};

