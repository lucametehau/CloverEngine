#pragma once
#include <fstream>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <random>

const double Momentum = 0.3;

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

class Layer {
public:
  Layer(LayerInfo _info, int prevNumNeurons) {
    info = _info;

    int numNeurons = _info.size;

    bias.resize(numNeurons);

    output.resize(numNeurons);

    if(prevNumNeurons) {
      weights.resize(prevNumNeurons);
      for(int i = 0; i < numNeurons; i++) {
        weights[i].resize(prevNumNeurons);
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

    load("Clover_100mil_d9_e121_h256_shuffled.nn");
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
      temp = read(sz, in);
      layers[i].output = read(sz, in);
      temp = read(sz, in);
      temp = read(sz, in);

      for(int j = 0; i && j < layers[i - 1].info.size; j++) {
        layers[i].weights[j] = read(layers[i].info.size, in);
        temp = read(layers[i].info.size, in);
      }
    }
  }

  std::vector <Layer> layers;
};

