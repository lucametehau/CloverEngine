#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "board.h"

const int DATASET_SIZE = (int)1e8;
const float EVAL = 0.0;
const float GAME_RES = 1.0 - EVAL;
float K;

float sigmoid(float K, int value) {
    return 1.0 / (1.0 + exp(-K * value / 400.0));
}

struct Param {
    int value;
    std::string name;
};

Param params[10];
int nrParams;
int cnt[10];

struct Input {
    int eval, pawnCount, pawnsOn1Flank;

    int evaluate() {
        int scale = params[0].value + params[1].value * pawnCount - params[2].value * pawnsOn1Flank;
        scale = std::max(scale, 0);
        scale = std::min(scale, 100);
        cnt[pawnCount]++;
        return eval * scale / 100 + TEMPO;
    }
};

struct Dataset {
    int nr;
    float *output;
    Input *input;

    Dataset() {
        nr = 0;
    }

    void init(int pos) {
        nr = 0;
        input = (Input*)malloc(sizeof(Input) * pos);
        output = (float*)malloc(sizeof(float) * pos);
    }
};

Board board;

void readDataset(Dataset& dataset, int dataSize, std::string path) {
    std::ifstream is(path);
    int positions = 0;
    std::string fen, line;

    for (int id = 0; id < dataSize && getline(is, line); id++) {
        if (is.eof() || dataset.nr >= DATASET_SIZE) {
            //cout << id << "\n";
            break;
        }

        int p = line.find(" "), p2;
        char stm;

        fen = line.substr(0, p + 2);

        stm = line[p + 1];

        p = line.find("[") + 1;
        p2 = line.find("]");

        std::string res = line.substr(p, p2 - p);
        float gameRes;

        if (res == "0")
            gameRes = 0;
        else if (res == "0.5")
            gameRes = 0.5;
        else
            gameRes = 1;

        int evalNr = 0, sign = 1;

        p = p2 + 2;

        if (line[p] == '-')
            sign = -1, p++;

        while (p < (int)line.size())
            evalNr = evalNr * 10 + line[p++] - '0';

        evalNr *= sign;

        if (stm == 'b')
            gameRes = 1.0 - gameRes;

        float eval = sigmoid(K, evalNr);
        float score = EVAL * eval + GAME_RES * gameRes;

        board.setFen(fen);

        if (true) {
            positions++;

            uint64_t allPawns = board.bb[WP] | board.bb[BP];
            dataset.input[dataset.nr] = { evaluate(board) - TEMPO, count(board.bb[getType(PAWN, board.turn)]), !((allPawns & flankMask[0]) && (allPawns & flankMask[1])) };
            dataset.output[dataset.nr] = score;
            dataset.nr++;
        }
    }
}

void readMultipleDatasets(Dataset& dataset, int dataSize, std::string path, int nrFiles) {
    std::vector <std::string> paths(nrFiles);

    //nrThreads = 1;

    for (int i = 0; i < nrFiles; i++) {
        paths[i] = path;
        if (i < 10)
            paths[i] += char(i + '0');
        else
            paths[i] += char(i / 10 + '0'), paths[i] += char(i % 10 + '0');
        paths[i] += ".txt"; /// assuming all files have this format
    }

    float startTime = clock();
    int temp = dataset.nr;

    std::cout << "Reading files from: " << path << "\n";
    for (int t = 0; t < nrFiles; t++) {
        std::cout << paths[t] << " " << dataSize << "\n";
        readDataset(dataset, dataSize, paths[t]);
        std::cout << "Done with file #" << t << "\n";
    }

    std::cout << (clock() - startTime) / CLOCKS_PER_SEC << " seconds for loading " << (int)dataset.nr - temp << " files\n";
}

Dataset dataset;

float calcError(Dataset& dataset, int dataSize, float K) {
    float error = 0;

#pragma omp parallel for schedule(auto) num_threads(8) reduction(+ : error)
    for (int i = 0; i < dataSize; i++) {
        float value = sigmoid(K, dataset.input[i].evaluate());

        error += (value - dataset.output[i]) * (value - dataset.output[i]);
    }

    return error / dataSize;
}

float bestK(Dataset& dataset, int dataSize) {
    float K = 0.0, error = calcError(dataset, dataSize, K);
    for (int i = 0; i <= 10; i++) {
        double unit = pow(10.0, -i), range = 10.0 * unit, r = K + range;

        for (double k = std::max(K - range, 0.0); k <= r; k += unit) {
            float newError = calcError(dataset, dataSize, k);

            if (newError < error) {
                error = newError;
                K = k;
            }
        }

        std::cout << std::fixed << std::setprecision(10) << K << " " << error << "\n";
    }

    return K;
}

void tune(int dataSize, int epochs) {
    dataset.init(DATASET_SIZE);
    readMultipleDatasets(dataset, dataSize, "C:\\Users\\Luca\\source\\repos\\CloverEngine\\CloverData_3_2_v2_", 16);

    params[nrParams++] = { pawnScaleStart, "pawnScaleStart" };
    params[nrParams++] = { pawnScaleStep, "pawnScaleStep" };
    params[nrParams++] = { pawnsOn1Flank, "pawnsOn1Flank" };

    dataSize = dataset.nr;

    K = bestK(dataset, dataSize);

    float error = calcError(dataset, dataSize, K);
    for (int epoch = 1; epoch <= epochs; epoch++) {
        std::cout << "Epoch #" << epoch << "/" << epochs << "\n";
        std::cout << "Error = " << error << "\n";
        float prevError = error;
        for (int i = 0; i < nrParams; i++) {
            params[i].value--;
            float newError = calcError(dataset, dataSize, K);

            if (newError < error) {
                error = newError;
            }
            else {
                params[i].value += 2;
                newError = calcError(dataset, dataSize, K);

                if (newError < error) {
                    error = newError;
                }
                else {
                    params[i].value -= 1;
                }
            }

            std::cout << "int " << params[i].name << " = " << params[i].value << ";\n";
        }

        if (epoch == 1) {
            for (int i = 0; i <= 8; i++)
                std::cout << cnt[i] << " ";
            std::cout << "\n";
        }

        if (prevError == error) {
            std::cout << "Experiment ending!\n";
            exit(0);
        }
    }
}