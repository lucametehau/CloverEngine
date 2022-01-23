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