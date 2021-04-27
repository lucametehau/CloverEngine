//#include <filesystem>
#include <thread>
#include <memory>
#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <fstream>
#include <cassert>
#include "search.h"
#include "evaluate.h"
#include "search.h"
#pragma once

const int PRECISION = 5;

struct Position {
  std::string fen;
  double result;
};

double sigmoid(double k, int s) {
  return 1.0 / (1.0 + pow(10.0, -k * s / 400.0));
}

std::mutex M;

/// TEXEL tuning method

bool openFile(std::ifstream &stream) {
  if(stream.is_open())
    stream.close();
  stream.open("C:\\Users\\LMM\\Desktop\\lichess-quiet.txt", std::ifstream::in);
  return stream.is_open();
}

void load(std::vector <Position> &texelPos, std::ifstream &stream, Search &searcher) {

  std::string line;
  int ind = 0;
  Info info[1];

  info->timeset = 0;
  info->startTime = 0;

  while(getline(stream, line)) {
    Position pos;

    ind++;

    if(ind % 100000 == 0)
      std::cout << ind << "\n";

    auto fenEnd = line.find_last_of(' ', std::string::npos) + 1;

    std::string result = "";
    int i = fenEnd + 1;
    while(i < (int)line.size() && line[i] != '"')
      result += line[i++];

    if(result == "1/2-1/2") /// for zurichess positions
      pos.result = 0.5;
    else if(result == "1-0")
      pos.result = 1.0;
    else if(result == "0-1")
      pos.result = 0.0;
    else if(result == "Draw") /// for lichess-quiet positions
      pos.result = 0.5;
    else if(result == "White")
      pos.result = 1.0;
    else if(result == "Black")
      pos.result = 0.0;

    //cout << pos.result << "\n";

    fenEnd -= 2;

    while(line[fenEnd] != ' ')
      fenEnd--;

    std::string fenString = line.substr(0, fenEnd);
    fenString += " 0 0";


    pos.fen = fenString;

    /*searcher._setFen(pos.fen);
    searcher.setTime(info);

    searcher.quiesce(-INF, INF);

    for(int i = 0; i < searcher.pvTableLen[0]; i++)
      searcher._makeMove(searcher.pvTable[0][i]);

    pos.fen = searcher.board.fen();*/


    //Board &board[1];

    //board.setFen(pos.fen);
    //board.print();


    texelPos.push_back(pos);
  }
}

void loadWeights(std::vector <int> &weights) {
  for(int i = MG; i <= EG; i++)
    weights.push_back(doubledPawnsPenalty[i]);
  for(int i = MG; i <= EG; i++)
    weights.push_back(isolatedPenalty[i]);
  for(int i = MG; i <= EG; i++)
    weights.push_back(backwardPenalty[i]);
  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++)
      weights.push_back(mat[s][i]);
  }
  for(int i = 1; i < 7; i++)
    weights.push_back(passedBonus[i]);
  for(int i = 1; i < 7; i++)
    weights.push_back(connectedBonus[i]);

  /// skipping king attack parameters

  for(int i = KNIGHT; i <= BISHOP; i++)
    weights.push_back(outpostBonus[i]);
  for(int i = KNIGHT; i <= BISHOP; i++)
    weights.push_back(outpostHoleBonus[i]);
  for(int s = MG; s <= EG; s++)
    weights.push_back(rookOpenFile[s]);
  for(int s = MG; s <= EG; s++)
    weights.push_back(rookSemiOpenFile[s]);
  for(int s = MG; s <= EG; s++)
    weights.push_back(bishopPair[s]);
  for(int s = MG; s <= EG; s++)
    weights.push_back(longDiagonalBishop[s]);

  weights.push_back(trappedRook);

  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++)
      weights.push_back(mobilityBonus[KNIGHT][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++)
      weights.push_back(mobilityBonus[BISHOP][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++)
      weights.push_back(mobilityBonus[ROOK][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++)
      weights.push_back(mobilityBonus[QUEEN][s][i]);
  }

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++)
        weights.push_back(bonusTable[i][s][j]);
    }
  }
}

void saveWeights(std::vector <int> &weights) {
  int ind = 0;
  for(int i = MG; i <= EG; i++)
    doubledPawnsPenalty[i] = weights[ind++];
  for(int i = MG; i <= EG; i++)
    isolatedPenalty[i] = weights[ind++];
  for(int i = MG; i <= EG; i++)
    backwardPenalty[i] = weights[ind++];
  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++)
      mat[s][i] = weights[ind++];
  }
  for(int i = 1; i < 7; i++)
    passedBonus[i] = weights[ind++];
  for(int i = 1; i < 7; i++)
    connectedBonus[i] = weights[ind++];


  for(int i = KNIGHT; i <= BISHOP; i++)
    outpostBonus[i] = weights[ind++];
  for(int i = KNIGHT; i <= BISHOP; i++)
    outpostHoleBonus[i] = weights[ind++];
  for(int s = MG; s <= EG; s++)
    rookOpenFile[s] = weights[ind++];
  for(int s = MG; s <= EG; s++)
    rookSemiOpenFile[s] = weights[ind++];
  for(int s = MG; s <= EG; s++)
    bishopPair[s] = weights[ind++];
  for(int s = MG; s <= EG; s++)
    longDiagonalBishop[s] = weights[ind++];

  trappedRook = weights[ind++];
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++)
      mobilityBonus[KNIGHT][s][i] = weights[ind++];
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++)
      mobilityBonus[BISHOP][s][i] = weights[ind++];
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++)
      mobilityBonus[ROOK][s][i] = weights[ind++];
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++)
      mobilityBonus[QUEEN][s][i] = weights[ind++];
  }

  //cout << ind << "\n";

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++)
        bonusTable[i][s][j] = weights[ind++];
    }
  }
}

void printWeights(std::vector <int> weights) {
  int ind = 0;

  std::cout << "int doubledPawnsPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";

  std::cout << "int isolatedPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";

  std::cout << "int backwardPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";

  std::cout << "int mat[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    std::cout << "    {0, ";
    for(int i = PAWN; i <= QUEEN; i++)
      std::cout << weights[ind++] << ", ";
    std::cout << "0},\n";
  }
  std::cout << "};\n";
  std::cout << "int phaseVal[] = {0, 0, 1, 1, 2, 4};\n";
  std::cout << "const int maxWeight = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];\n";
  std::cout << "int passedBonus[] = {0";
  for(int i = 1; i <= 6; i++)
    std::cout << ", " << weights[ind++];
  std::cout << "};\n";
  std::cout << "int connectedBonus[] = {0";
  for(int i = 1; i <= 6; i++)
    std::cout << ", " << weights[ind++];
  std::cout << "};\n";
  std::cout << "int kingAttackWeight[] = {0, 0";
  for(int i = KNIGHT; i <= QUEEN; i++)
    std::cout << ", " << kingAttackWeight[i];
  std::cout << "};\n";
  std::cout << "int SafetyTable[100] = {\n";
  for(int i = 0; i < 10; i++) {
    std::cout << "  ";
    for(int j = 0; j < 10; j++) {
      std::cout << SafetyTable[i * 10 + j] << ", ";
    }
    std::cout << "\n";
  }
  std::cout << "};\n";
  std::cout << "int outpostBonus[] = {0, 0";
  for(int i = KNIGHT; i <= BISHOP; i++)
    std::cout << ", " << weights[ind++];
  std::cout << "};\n";
  std::cout << "int outpostHoleBonus[] = {0, 0";
  for(int i = KNIGHT; i <= BISHOP; i++)
    std::cout << ", " << weights[ind++];
  std::cout << "};\n";
  std::cout << "int rookOpenFile[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";
  std::cout << "int rookSemiOpenFile[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";
  std::cout << "int bishopPair[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";
  std::cout << "int longDiagonalBishop[2] = {";
  for(int i = MG; i <= EG; i++)
    std::cout << weights[ind++] << ", ";
  std::cout << "};\n";

  std::cout << "int trappedRook = " << weights[ind++] << ";\n";

  std::cout << "int mobilityBonus[7][2][30] = {\n";
  std::cout << "    {},\n";
  std::cout << "    {},\n";
  std::cout << "    {\n";
  for(int s = MG; s <= EG; s++) {
    std::cout << "        {";
    for(int i = 0; i < 9; i++)
      std::cout << weights[ind++] << ", ";
    std::cout << "},\n";
  }
  std::cout << "    },\n";
  std::cout << "    {\n";
  for(int s = MG; s <= EG; s++) {
    std::cout << "        {";
    for(int i = 0; i < 14; i++)
      std::cout << weights[ind++] << ", ";
    std::cout << "},\n";
  }
  std::cout << "    },\n";
  std::cout << "    {\n";
  for(int s = MG; s <= EG; s++) {
    std::cout << "        {";
    for(int i = 0; i < 15; i++)
      std::cout << weights[ind++] << ", ";
    std::cout << "},\n";
  }
  std::cout << "    },\n";
  std::cout << "    {\n";
  for(int s = MG; s <= EG; s++) {
    std::cout << "        {";
    for(int i = 0; i < 28; i++)
      std::cout << weights[ind++] << ", ";
    std::cout << "},\n";
  }
  std::cout << "    }\n";
  std::cout << "};\n";

  std::cout << "int bonusTable[7][2][64] = {\n";
  std::cout << "    {},\n";
  for(int i = PAWN; i <= KING; i++) {
    std::cout << "    {\n";
    for(int s = MG; s <= EG; s++) {
      std::cout << "        {\n            ";
      for(int j = A1; j <= H8; j++) {
        std::cout << weights[ind++] << ", ";
        if(j % 8 == 7)
          std::cout << "\n            ";
      }
      std::cout << "\n        },\n";
    }
    std::cout << "    },\n";
  }
  std::cout << "};\n";
}

void rangeEvalError(std::vector <Position> &texelPos, std::atomic <double> & error, double k, int l, int r) {

  double errorRange = 0;
  Board board;

  for(int i = l; i < r; i++) {
    board.setFen(texelPos[i].fen);
    //board.print();
    int score = evaluate(board) * (board.turn == WHITE ? 1 : -1);

    errorRange += pow(texelPos[i].result - sigmoid(k, score), 2);
    //cout << errorRange << "\n";
  }
  M.lock();

  error = error + errorRange;

  M.unlock();
}

double evalError(std::vector <Position> &texelPos, double k, int nrThreads) {
  std::atomic <double> error{};
  double share = 1.0 * texelPos.size() / nrThreads;
  double ind = 0;

  std::vector <std::thread> threads(nrThreads);

  for(auto &t : threads) {
    t = std::thread{ rangeEvalError, ref(texelPos), ref(error), k, int(floor(ind)), int(floor(ind + share) - 1) };
    ind += share;
  }

  for(auto &t : threads)
    t.join();

  assert(floor(ind) == texelPos.size());

  return error / texelPos.size();
}

double bestK(std::vector <Position> &texelPos, int nrThreads) {
  double k = 0;
  double mn = std::numeric_limits <double> :: max();

  std::cout.precision(PRECISION + 2);

  for(int i = 0; i <= PRECISION; i++) {
    std::cout << "iteration " << i + 1 << " ...\n";

    double unit = pow(10.0, -i), range = 10.0 * unit, r = k + range;

    for(double curr = std::max(k - range, 0.0); curr <= r; curr += unit) {
      double error = evalError(texelPos, curr, nrThreads);

      std::cout << "current K = " << curr << ", current error = " << error << "\n";

      if(error < mn) {
        mn = error;
        k = curr;
      }
    }

    std::cout << "K = " << k << ", error = " << mn << std::endl;
  }

  return k;
}

bool isBetter(std::vector <int> &weights, std::vector <Position> &texelPos, double &mn, double k, int nrThreads, int ind = 0) {
  saveWeights(weights);

  double error = evalError(texelPos, k, nrThreads);

  if(error < mn) {
    if(mn - error > mn / nrThreads / 2) {
      return 0;
    }
    mn = error;
    return 1;
  }

  return 0;
}

void tune(Search &searcher) {
  int nrThreads = 8;

  std::ifstream stream;
  if(openFile(stream))
    std::cout << "opening file.." << std::endl;

  //cout << sizeof(Board) << endl;

  std::vector <Position> texelPos;
  load(texelPos, stream, searcher);

  //cout << texelPos.size() << " positions stored" << endl;

  /*for(auto &pos : texelPos) {
    std::cout << pos.board.fen() << " " << pos.result << "\n";
  }*/

  //cout << "memory allocated: " << (sizeof(Position) * texelPos.size() >> 20) << " MB" << endl;

  std::vector <int> weights;
  loadWeights(weights);

  saveWeights(weights);

  //printWeights(weights);

  //cout << weights.size() << endl;

  //return;
  //return;

  /*Board &board;

  for(auto &pos : texelPos) {
    board.setFen(pos.fen);
    int score = evaluate(board) * (board.turn == WHITE ? 1 : -1);

  }

  return;*/

  double k = bestK(texelPos, nrThreads);

  double startTime = getTime();

  double errorStart = evalError(texelPos, k, nrThreads), errorMin = errorStart;
  double otherTime = getTime();
  int i = 1;

  std::cout << "best K = " << k << "\n";

  std::cout << "start evaluation error: " << errorStart << std::endl;
  std::cout << "expected iteration time: " << (uint64_t)(otherTime - startTime) * weights.size() * 2 / 1000 << "s" << std::endl;

  //cout << evaluate(texelPos[0].board) << endl;

  while(true) {
    std::cout << "epoch " << i << " starting..." << std::endl;

    double ST = getTime();
    double lst = errorMin;
    int ind = 0;

    for(auto &w : weights) {
      int temp = w;
      bool improve = 0;

      //cout << "parameter " << ind << "..\n";

      w += 1;

      if(isBetter(weights, texelPos, errorMin, k, nrThreads, ind))
        improve = 1;
      else {
        w -= 2;
        improve = isBetter(weights, texelPos, errorMin, k, nrThreads, ind);
      }

      if(!improve)
        w = temp;

      ind++;
    }

    double ET = getTime();

    std::cout << "time taken for iteration: " << (uint64_t)(ET - ST) / 1000 << " s\n";
    i++;
    printWeights(weights);
    saveWeights(weights);

    std::cout << "evaluation error: " << errorMin << "\n";

    if(errorMin == lst)
      break;
  }

  //cout << passedBonus[6] << "\n";

  //errorMin = evalError(texelPos, k);

  std::cout << "evaluation error: " << errorStart << " -> " << errorMin << std::endl;

  std::cout << "Time taken: " << (clock() - startTime) / 1000.0 << " s" << std::endl;
}
