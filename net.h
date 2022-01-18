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
  //double grad;
  double m1, m2; /// momentums

  Gradient(double _m1, double _m2) {
    m1 = _m1;
    m2 = _m2;
    //grad = 0;
  }

  Gradient() {
    m1 = m2 = 0;
    //grad = 0;
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

    updateSz = 0;
    updates.resize(5);

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

  void removeInput(int16_t ind) {
    /*for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] -= layers[1].weights[ind][n];
    }*/
    updates[updateSz++] = {ind, -1};
  }

  void addInput(int16_t ind) {
    /*for(int n = 0; n < layers[1].info.size; n++) {
      layers[1].output[n] += layers[1].weights[ind][n];
    }*/
    updates[updateSz++] = {ind, 1};
  }

  void applyUpdates() {
    histOutput[histSz] = histOutput[histSz - 1];
    histSz++;

    for(int n = 0; n < layers[1].info.size; n++)
      sum += std::max(0.0f, layers[1].output[n]) * layers.back().weights[n][0];

  void revertUpdates() {
    histSz--;
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

        //std::cout << "\n";

        gradData = (Gradient*)doubleData;
        for(int k = 0; k < sz; k++) {
          v[k] = *(gradData++);
        }
      }
    }
  }

  int histSz, updateSz;
  std::vector <Layer> layers;
  std::vector <std::vector <double>> histOutput;
  std::vector <Update> updates;
};

