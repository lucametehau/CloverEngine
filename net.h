#pragma once
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <random>

const double SIGMOID_SCALE = 0.0075;

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

class Param {
public:
  double value, grad;
  double m1, m2; /// momentums

  Param(double _value, double _m1, double _m2) {
    value = _value;
    m1 = _m1;
    m2 = _m2;
    grad = 0;
  }

  Param() {
    value = grad = 0;
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

    load("Clover_100mil_d9_e7_test.nn");
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

  double getOutput() {
    double sum = layers.back().bias[0];

    for(int n = 0; n < layers[1].info.size; n++)
      sum += std::max(0.0, layers[1].output[n]) * layers.back().weights[n][0];

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

  void save(std::string path) {
    FILE *f = fopen(path.c_str(), "wb");
    int cnt = layers.size(), x;

    x = fwrite(&cnt, sizeof(int), 1, f);
    assert(x == 1);

    for(int i = 0; i < (int)layers.size(); i++) {
      int sz = layers[i].info.size;

      x = fwrite(&layers[i].bias[0], sizeof(Param), sz, f);

      assert(x == sz);

      for(int j = 0; i && j < layers[i - 1].info.size; j++) {
        x = fwrite(&layers[i].weights[j][0], sizeof(Param), sz, f);

        assert(x == sz);
      }
    }

    fclose(f);
  }

  void load(std::string path) {
    FILE *f = fopen(path.c_str(), "rb");
    int cnt = layers.size(), x;
    std::vector <Param> v;

    x = fread(&cnt, sizeof(int), 1, f);
    assert(x == 1);
    assert(cnt == (int)layers.size());

    for(int i = 0; i < (int)layers.size(); i++) {
      int sz = layers[i].info.size;

      v.resize(sz);
      x = fread(&v[0], sizeof(Param), sz, f);

      for(int j = 0; j < sz; j++)
        layers[i].bias[j] = v[j].value;

      assert(x == sz);

      for(int j = 0; i && j < layers[i - 1].info.size; j++) {
        x = fread(&v[0], sizeof(Param), sz, f);

        for(int k = 0; k < sz; k++)
          layers[i].weights[j][k] = v[k].value;

        assert(x == sz);
      }
    }

    fclose(f);
  }

  std::vector <Layer> layers;
};

