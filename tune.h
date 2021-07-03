//#include <filesystem>
#pragma once
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

const int PRECISION = 8;
const int NPOS = 9999740; /// 9999740 2500002
const int TERMS = 1316;
const int SCALE_TERMS = 3;
const int BUCKET_SIZE = 1LL * NPOS * TERMS / 64;
const double TUNE_K = 2.67213609;

struct Position {
  std::string fen;
  float result;
  int16_t staticEval;
};

TunePos *position;
Search emptySearcher[1];

int nrPos;
Position *texelPos;
EvalTraceEntry *stack;
int stackSize = BUCKET_SIZE;

double sigmoid(double k, double s) {
  return 1.0 / (1.0 + exp(-k * s / 400.0));
}

std::mutex M; /// for debugging data races
double weights[TERMS + 5];
int scaleWeights[SCALE_TERMS + 5];
EvalTrace empty;

/// TEXEL tuning method

bool openFile(std::ifstream &stream, std::string path) {
  if(stream.is_open())
    stream.close();
  stream.open(path, std::ifstream::in);
  return stream.is_open();
}

void load(std::ifstream &stream) {

  position = (TunePos*) calloc(NPOS + 5, sizeof(TunePos));
  texelPos = (Position*) calloc(NPOS + 5, sizeof(Position));

  std::string line;
  Info info[1];
  Board board;

  info->timeset = 0;
  info->startTime = 0;

  uint64_t totalEntries = 0, kek = 0, d = 0;
  int kc = 0, defp = 0, weak = 0;
  uint64_t kekw[28], totPieceInKingRing[6];

  memset(kekw, 0, sizeof(kekw));

  std::cout << sizeof(EvalTraceEntry) << " " << sizeof(TunePos) << "\n";


  while(getline(stream, line)) {
    Position pos;

    if(nrPos % 100000 == 0) { /// to check that everything is working, also some info about some terms
      std::cout << nrPos << ", entries = " << totalEntries << ", kek = " << kek << ", kc = " << kc << ", d = " << d << ", defp = " << defp
                << ", weak = " << weak << ", pieceInKingRing: ";
      for(int i = KNIGHT; i <= QUEEN; i++)
        std::cout << totPieceInKingRing[i] << " ";
      std::cout << "\n";
      /*for(int i = 0; i < 28; i++)
        std::cout << kekw[i] << " ";
      std::cout << "\n";*/
    }

    auto fenEnd = line.find_last_of(' ', std::string::npos) + 1;

    std::string result = "";
    int i = fenEnd + 1;
    while(i < (int)line.size() && line[i] != '"' && line[i] != ']')
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
    else if(result == "0.0") /// for ethereal positions
      pos.result = 0.0;
    else if(result == "0.5")
      pos.result = 0.5;
    else if(result == "1.0")
      pos.result = 1.0;

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

    //std::cout << pos.fen << "\n";

    board.setFen(pos.fen);

    trace = empty;
    //std::cout << trace.phase << "\n";
    int initScore = evaluate(board, emptySearcher);

    pos.staticEval = initScore;

    getTraceEntries(trace);

    int num = trace.nrEntries;
    totalEntries += num;

    if(num > stackSize) {
      stackSize = BUCKET_SIZE;
      stack = (EvalTraceEntry*)calloc(BUCKET_SIZE, sizeof(EvalTraceEntry));
    }

    position[nrPos].entries = stack;
    stack += num;
    stackSize -= num;

    position[nrPos].nrEntries = num;
    for(int i = 0; i < num; i++)
      position[nrPos].entries[i] = trace.entries[i];

    position[nrPos].phase = trace.phase;
    position[nrPos].mg = trace.mg;
    position[nrPos].eg = trace.eg;
    position[nrPos].turn = board.turn;
    position[nrPos].scale = trace.scale;

    position[nrPos].ocb = trace.ocb;
    position[nrPos].ocbPieceCount = trace.ocbPieceCount;
    position[nrPos].pawnsOn1Flank = trace.pawnsOn1Flank;

    kek += sizeof(position[nrPos]);

    weak += trace.weakKingSq[WHITE][MG] + trace.weakKingSq[BLACK][MG];

    kc += trace.safeCheck[WHITE][MG][KNIGHT] + trace.safeCheck[BLACK][MG][KNIGHT];

    defp += trace.pawnDefendedBonus[WHITE][MG] + trace.pawnDefendedBonus[BLACK][MG];

    d += trace.passerDistToEdge[WHITE][MG] + trace.passerDistToEdge[BLACK][MG];

    for(int i = KNIGHT; i <= QUEEN; i++)
      totPieceInKingRing[i] += trace.pieceInKingRing[WHITE][MG][i] + trace.pieceInKingRing[BLACK][MG][i];

    /*for(int i = 0; i < 28; i++)
      kekw[i] += trace.mobilityBonus[WHITE][QUEEN][MG][i] + trace.mobilityBonus[WHITE][QUEEN][MG][i];*/

    /*if(trace.scale) {
      std::cout << "xd????????????\n";
      std::cout << int(trace.scale) << "\n";
      board.print();
    }*/

    /*double traceScore = evaluateTrace(position[nrPos], weights);

    if(abs(initScore - traceScore) >= 10) {
      for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
          for(int k = KNIGHT; k <= QUEEN; k++)
            std::cout << int(trace.safeCheck[i][j][k]) << " ";
        }
      }
      std::cout << "\n";
      std::cout << initScore << " " << traceScore << "\n";
      board.print();
    }*/

    texelPos[nrPos++] = pos;
  }
}

void loadWeights() {
  int ind = 0;
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (passerDistToEdge[i]);
  /*for(int i = MG; i <= EG; i++)
    weights[ind++] = (passerDistToKings[i]);*/
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (doubledPawnsPenalty[i]);
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (isolatedPenalty[i]);
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (backwardPenalty[i]);
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (pawnDefendedBonus[i]);

  for(int i = MG; i <= EG; i++)
    weights[ind++] = (threatByPawnPush[i]);
  for(int i = MG; i <= EG; i++)
    weights[ind++] = (threatMinorByMinor[i]);

  for(int i = MG; i <= EG; i++)
    weights[ind++] = (knightBehindPawn[i]);

  for(int i = MG; i <= EG; i++)
    weights[ind++] = (weakKingSq[i]);

  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++)
      weights[ind++] = (mat[s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      weights[ind++] = (passedBonus[s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      weights[ind++] = (blockedPassedBonus[s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      weights[ind++] = (connectedBonus[s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 100; i++)
      weights[ind++] = (SafetyTable[s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++)
        weights[ind++] = kingShelter[s][i][j];
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++)
        weights[ind++] = kingStorm[s][i][j];
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 7; i++)
      weights[ind++] = blockedStorm[s][i];
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= QUEEN; i++)
      weights[ind++] = (safeCheck[s][i]);
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++)
      weights[ind++] = (outpostBonus[s][i]);
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++)
      weights[ind++] = (outpostHoleBonus[s][i]);
  }
  for(int s = MG; s <= EG; s++)
    weights[ind++] = (rookOpenFile[s]);
  for(int s = MG; s <= EG; s++)
    weights[ind++] = (rookSemiOpenFile[s]);
  for(int s = MG; s <= EG; s++)
    weights[ind++] = (bishopPair[s]);
  for(int s = MG; s <= EG; s++)
    weights[ind++] = (longDiagonalBishop[s]);

  for(int s = MG; s <= EG; s++) {
    weights[ind++] = (trappedRook[s]);
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++)
      weights[ind++] = (mobilityBonus[KNIGHT][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++)
      weights[ind++] = (mobilityBonus[BISHOP][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++)
      weights[ind++] = (mobilityBonus[ROOK][s][i]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++)
      weights[ind++] = (mobilityBonus[QUEEN][s][i]);
  }

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++)
        weights[ind++] = (bonusTable[i][s][j]);
    }
  }
  std::cout << ind << " terms\n";

  ind = 0;
  scaleWeights[ind++] = ocbStart;
  scaleWeights[ind++] = ocbStep;
  scaleWeights[ind++] = pawnsOn1Flank;
  std::cout << ind << " scale terms\n";
}

void saveWeights() {
  int ind = 0;
  for(int i = MG; i <= EG; i++)
    passerDistToEdge[i] = std::round(weights[ind++]);
  /*for(int i = MG; i <= EG; i++)
    passerDistToKings[i] = std::round(weights[ind++]);*/
  for(int i = MG; i <= EG; i++)
    doubledPawnsPenalty[i] = std::round(weights[ind++]);
  for(int i = MG; i <= EG; i++)
    isolatedPenalty[i] = std::round(weights[ind++]);
  for(int i = MG; i <= EG; i++)
    backwardPenalty[i] = std::round(weights[ind++]);
  for(int i = MG; i <= EG; i++)
    pawnDefendedBonus[i] = std::round(weights[ind++]);

  for(int i = MG; i <= EG; i++)
    threatByPawnPush[i] = std::round(weights[ind++]);
  for(int i = MG; i <= EG; i++)
    threatMinorByMinor[i] = std::round(weights[ind++]);

  for(int i = MG; i <= EG; i++)
    knightBehindPawn[i] = std::round(weights[ind++]);

  for(int i = MG; i <= EG; i++)
    weakKingSq[i] = std::round(weights[ind++]);

  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++)
      mat[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      passedBonus[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      blockedPassedBonus[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++)
      connectedBonus[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 100; i++)
      SafetyTable[s][i] = std::round(weights[ind++]);
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++)
        kingShelter[s][i][j] = std::round(weights[ind++]);
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++)
        kingStorm[s][i][j] = std::round(weights[ind++]);
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 7; i++)
      blockedStorm[s][i] = std::round(weights[ind++]);
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= QUEEN; i++)
      safeCheck[s][i] = std::round(weights[ind++]);
  }


  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++)
      outpostBonus[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++)
      outpostHoleBonus[s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++)
    rookOpenFile[s] = std::round(weights[ind++]);
  for(int s = MG; s <= EG; s++)
    rookSemiOpenFile[s] = std::round(weights[ind++]);
  for(int s = MG; s <= EG; s++)
    bishopPair[s] = std::round(weights[ind++]);
  for(int s = MG; s <= EG; s++)
    longDiagonalBishop[s] = std::round(weights[ind++]);

  for(int s = MG; s <= EG; s++)
    trappedRook[s] = std::round(weights[ind++]);
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++)
      mobilityBonus[KNIGHT][s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++)
      mobilityBonus[BISHOP][s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++)
      mobilityBonus[ROOK][s][i] = std::round(weights[ind++]);
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++)
      mobilityBonus[QUEEN][s][i] = std::round(weights[ind++]);
  }

  //cout << ind << "\n";

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++)
        bonusTable[i][s][j] = std::round(weights[ind++]);
    }
  }

  ind = 0;

  ocbStart = scaleWeights[ind++];
  ocbStep = scaleWeights[ind++];
  pawnsOn1Flank = scaleWeights[ind++];
}

void printWeights(int iteration) {
  int ind = 0;

  std::ofstream out ("Weights.txt");

  int newWeights[TERMS];

  for(int i = 0; i < TERMS; i++)
    newWeights[i] = std::round(weights[i]);

  out << "Iteration: " << iteration << "\n";

  out << "int passerDistToEdge[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  /*out << "int passerDistToKings[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";*/

  out << "int doubledPawnsPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int isolatedPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int backwardPenalty[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int pawnDefendedBonus[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int threatByPawnPush[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int threatMinorByMinor[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int knightBehindPawn[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int weakKingSq[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int mat[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "    {0, ";
    for(int i = PAWN; i <= QUEEN; i++)
      out << newWeights[ind++] << ", ";
    out << "0},\n";
  }
  out << "};\n\n";
  out << "const int phaseVal[] = {0, 0, 1, 1, 2, 4};\n";
  out << "const int maxWeight = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];\n\n";

  out << "int passedBonus[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0";
    for(int i = 1; i < 7; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n";

  out << "int blockedPassedBonus[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0";
    for(int i = 1; i < 7; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n";

  out << "int connectedBonus[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0";
    for(int i = 1; i < 7; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n";

  out << "int kingAttackWeight[] = {0, 0";
  for(int i = KNIGHT; i <= QUEEN; i++)
    out << ", " << kingAttackWeight[i];
  out << "};\n";

  out << "int SafetyTable[2][100] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {\n";
    for(int i = 0; i < 10; i++) {
      out << "    ";
      for(int j = 0; j < 10; j++)
        out << newWeights[ind++] << ", ";
      out << "\n";
    }
    out << "  },\n";
  }
  out << "};\n";

  out << "int kingShelter[2][4][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {\n";
    for(int i = 0; i < 4; i++) {
      out << "    {";
      for(int j = 0; j < 7; j++)
        out << newWeights[ind++] << ", ";
      out << "},\n";
    }
    out << "  },\n";
  }
  out << "};\n";

  out << "int kingStorm[2][4][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {\n";
    for(int i = 0; i < 4; i++) {
      out << "    {";
      for(int j = 0; j < 7; j++)
        out << newWeights[ind++] << ", ";
      out << "},\n";
    }
    out << "  },\n";
  }
  out << "};\n";

  out << "int blockedStorm[2][7] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {";
    for(int i = 0; i < 7; i++)
      out << newWeights[ind++] << ", ";
    out << "},\n";
  }
  out << "};\n";

  out << "int safeCheck[2][6] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0, 0";
    for(int i = KNIGHT; i <= QUEEN; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n";

  out << "int outpostBonus[2][4] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0, 0";
    for(int i = KNIGHT; i <= BISHOP; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n";

  out << "int outpostHoleBonus[2][4] = {\n";
  for(int s = MG; s <= EG; s++) {
    out << "  {0, 0";
    for(int i = KNIGHT; i <= BISHOP; i++)
      out << ", " << newWeights[ind++];
    out << "},\n";
  }
  out << "};\n\n";

  out << "int rookOpenFile[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int rookSemiOpenFile[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int bishopPair[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int longDiagonalBishop[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n";

  out << "int trappedRook[2] = {";
  for(int i = MG; i <= EG; i++)
    out << newWeights[ind++] << ", ";
  out << "};\n\n";

  out << "int mobilityBonus[7][2][30] = {\n";
  out << "    {},\n";
  out << "    {},\n";
  out << "    {\n";
  for(int s = MG; s <= EG; s++) {
    out << "        {";
    for(int i = 0; i < 9; i++)
      out << newWeights[ind++] << ", ";
    out << "},\n";
  }
  out << "    },\n";
  out << "    {\n";
  for(int s = MG; s <= EG; s++) {
    out << "        {";
    for(int i = 0; i < 14; i++)
      out << newWeights[ind++] << ", ";
    out << "},\n";
  }
  out << "    },\n";
  out << "    {\n";
  for(int s = MG; s <= EG; s++) {
    out << "        {";
    for(int i = 0; i < 15; i++)
      out << newWeights[ind++] << ", ";
    out << "},\n";
  }
  out << "    },\n";
  out << "    {\n";
  for(int s = MG; s <= EG; s++) {
    out << "        {";
    for(int i = 0; i < 28; i++)
      out << newWeights[ind++] << ", ";
    out << "},\n";
  }
  out << "    }\n";
  out << "};\n";

  out << "int bonusTable[7][2][64] = {\n";
  out << "    {},\n";
  for(int i = PAWN; i <= KING; i++) {
    out << "    {\n";
    for(int s = MG; s <= EG; s++) {
      out << "        {\n            ";
      for(int j = A1; j <= H8; j++) {
        out << newWeights[ind++] << ", ";
        if(j % 8 == 7)
          out << "\n            ";
      }
      out << "\n        },\n";
    }
    out << "    },\n";
  }
  out << "};\n\n";

  ind = 0;

  out << "int ocbStart = " << scaleWeights[ind++] << ";\n";
  out << "int ocbStep = " << scaleWeights[ind++] << ";\n";
  out << "int pawnsOn1Flank = " << scaleWeights[ind++] << ";\n";

}

void rangeEvalError(std::atomic <double> & error, double k, int l, int r, bool isStaticEval) {

  double errorRange = 0;
  //Board board;

  for(int i = l; i <= r; i++) {
    //board.setFen(texelPos[i].fen);
    //board.print();
    double eval;
    if(TUNE_FLAG & TUNE_SCALE) {
      int mg = position[i].mg, eg = position[i].eg;

      eg = eg * scaleFactorTrace(position[i]) / 100;

      eval = (mg * position[i].phase + eg * (maxWeight - position[i].phase)) / maxWeight;

      eval = TEMPO + eval * (position[i].turn == WHITE ? 1 : -1);
    }
    if(TUNE_FLAG & TUNE_TERMS) {
      if(!isStaticEval) {
        eval = evaluateTrace(position[i], weights);
      } else {
        eval = texelPos[i].staticEval;
      }
    }
    //int evalInit = evaluate(board);
    //assert(evalInit == eval);
    /*
    if(evalInit != evaltr) {
      M.lock();
      std::cout << i << "\n";
      board.print();
      for(int j = 0; j < position[i].nrEntries; j++)
        std::cout << position[i].entries[j].ind << " " << position[i].entries[j].val << "\n";
      std::cout << evalInit << " " << evaltr << "\n";
      M.unlock();
      assert(0);
    }*/
    double score = eval * (position[i].turn == WHITE ? 1 : -1);

    errorRange += pow(texelPos[i].result - sigmoid(k, score), 2);
    //cout << errorRange << "\n";
  }
  M.lock();

  error = error + errorRange;

  M.unlock();
}

double evalError(double k, int nrThreads, bool isStaticEval = 0) {
  std::atomic <double> error{};
  double share = 1.0 * nrPos / nrThreads;
  double ind = 0;

  std::vector <std::thread> threads(nrThreads);

  for(auto &t : threads) {
    t = std::thread{ rangeEvalError, ref(error), k, int(floor(ind)), int(floor(ind + share) - 1), isStaticEval };
    ind += share;
  }

  for(auto &t : threads)
    t.join();

  assert(floor(ind) == nrPos);

  return error / nrPos;
}

double bestK(int nrThreads, long double ST) {
  double k = 0;
  double mn = std::numeric_limits <double> :: max();

  std::cout.precision(15);

  for(int i = 0; i <= PRECISION; i++) {
    std::cout << "iteration " << i + 1 << " ...\n";

    double unit = pow(10.0, -i), range = 10.0 * unit, r = k + range;

    for(double curr = std::max(k - range, 0.0); curr <= r; curr += unit) {
      double error = evalError(curr, nrThreads, 1);

      std::cout << "current K = " << curr << ", current error = " << error << ", time = " << (getTime() - ST) / 1000.0 << "s\n";

      if(error < mn) {
        mn = error;
        k = curr;
      }
    }

    std::cout << "K = " << k << ", error = " << mn << std::endl;
  }

  return k;
}

void calcSingleGradient(double k, int ind, double grad[]) {
  double s = sigmoid(k, evaluateTrace(position[ind], weights) * (position[ind].turn == WHITE ? 1 : -1));
  double x = (texelPos[ind].result - s) * s * (1.0 - s);
  double mgMult = x * (position[ind].phase / 24.0), egMult = x * (1.0 - position[ind].phase / 24.0);

  for(int i = 0; i < position[ind].nrEntries; i++) {
    int mgind = position[ind].entries[i].mgind, egind = mgind + position[ind].entries[i].deltaind;

    grad[mgind] = grad[mgind] + mgMult * position[ind].entries[i].dval;
    grad[egind] = grad[egind] + egMult * position[ind].entries[i].dval * position[ind].scale / 100.0;
  }
}

void calcRangeGradient(double grad[], double k, int l, int r) {
  //std::cout << l << " " << r << "\n";
  for(int i = l; i <= r; i++) {
    calcSingleGradient(k, i, grad);
  }
}

void calcGradient(double k, double grad[], int nrThreads) {
  double share = 1.0 * nrPos / nrThreads;
  double ind = 0;

  double tGrad[10][TERMS + 5];

  memset(tGrad, 0, sizeof(tGrad));
  std::vector <std::thread> threads(nrThreads);

  int tInd = 0;

  for(auto &t : threads) {
    t = std::thread{ calcRangeGradient, tGrad[tInd], k, int(floor(ind)), int(floor(ind + share) - 1) };
    ind += share;
    tInd++;
  }

  for(auto &t : threads)
    t.join();

  for(int i = 0; i < nrThreads; i++) {
    for(int j = 0; j < TERMS; j++)
      grad[j] += tGrad[i][j];
  }
}

bool isBetter(double &mn, double k, int nrThreads) {
  saveWeights();

  double error = evalError(k, nrThreads);

  if(error < mn) {
    if(mn - error > mn / nrThreads / 2) {
      return 0;
    }
    mn = error;
    return 1;
  }

  return 0;
}

void tune(int nrThreads, std::string path) {
  //int nrThreads = 8;

  emptySearcher->clearStack();

  stack = (EvalTraceEntry*)calloc(BUCKET_SIZE, sizeof(EvalTraceEntry));

  std::ifstream stream;
  if(openFile(stream, path))
    std::cout << "opening file.." << std::endl;

  loadWeights();

  for(int i = 0; i < TERMS; i++)
    std::cout << weights[i] << " ";

  std::cout << "\n";

  saveWeights();

  double grad[TERMS + 5], adagrad[TERMS + 5];

  printWeights(0);

  load(stream);

  double start = getTime();

  double k = bestK(nrThreads, start);
  //double k  = TUNE_K;

  double startTime = getTime();

  double errorStart = evalError(k, nrThreads), errorMin = errorStart;

  std::cout << "best K = " << k << "\n";

  std::cout << "start evaluation error: " << errorStart << std::endl;

  //cout << evaluate(texelPos[0].board) << endl;

  if(TUNE_FLAG & TUNE_TERMS) { /// tune normal terms
    memset(adagrad, 0, sizeof(adagrad));

    double rate = 1.0;

    for(int i = 1; i <= 100000; i++) {
      std::cout << "epoch " << i << " starting..." << std::endl;

      double ST = getTime();

      memset(grad, 0, sizeof(grad));

      calcGradient(k, grad, nrThreads);

      for(int j = 0; j < TERMS; j++) {
        adagrad[j] += pow((k / 200) * grad[j] / NPOS, 2.0);

        weights[j] += (k / 200) * (grad[j] / NPOS) * (rate / sqrt(1e-8 + adagrad[j]));
      }

      if(i % 100 == 0)
        rate /= 1.0;

      double ET = getTime();

      if(i % 10 == 0) {
        std::cout << "time taken for epoch: " << (double)(ET - ST) / 1000.0 << " s\n";
        printWeights(i);
        //saveWeights();

        errorMin = evalError(k, nrThreads);

        std::cout << "evaluation error: " << errorMin << "\n";
      }
    }
  }
  if(TUNE_FLAG & TUNE_SCALE) {
    std::cout << "Tuning scale terms...\n";
    for(int i = 1; i <= 1000; i++) {
      long double t1 = getTime();
      std::cout << "epoch " << i << " starting..." << std::endl;
      for(int j = 0; j < SCALE_TERMS; j++) {
        int temp = scaleWeights[j];
        bool improve = 0;

        //cout << "parameter " << ind << "..\n";

        scaleWeights[j] += 1;

        if(isBetter(errorMin, k, nrThreads))
          improve = 1;
        else {
          scaleWeights[j] -= 2;
          improve = isBetter(errorMin, k, nrThreads);
        }

        if(!improve)
          scaleWeights[j] = temp;
      }
      long double t2 = getTime();
      std::cout << "time taken for epoch: " << (t2 - t1) / 1000.0 << " s\n";
      printWeights(i);
      errorMin = evalError(k, nrThreads);
      saveWeights();
      std::cout << "evaluation error: " << errorMin << "\n";
    }
  }

  //cout << passedBonus[6] << "\n";

  //errorMin = evalError(texelPos, k);

  std::cout << "evaluation error: " << errorStart << " -> " << errorMin << std::endl;

  std::cout << "Time taken: " << (getTime() - startTime) / 1000.0 << " s" << std::endl;
}
