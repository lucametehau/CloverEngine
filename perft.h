#pragma once
#include "move.h"

uint64_t perft(Board &board, int depth, bool print = 0) {
  uint16_t moves[256];
  //uint16_t quiets[256], noisy[256];
  int nrMoves = genLegal(board, moves);
  //int nrQuiets = genLegalQuiets(board, quiets), nrNoisy = genLegalNoisy(board, noisy);

  /*if(nrMoves != nrQuiets + nrNoisy) {
    cout << "OH NO WE HAVE A PROBLEM\n";
    cout << "All moves: ";
    for(int i = 0; i < nrMoves; i++)
      cout << toString(moves[i]) << " ";
    cout << "\nQuiets moves: ";
    for(int i = 0; i < nrQuiets; i++)
      cout << toString(quiets[i]) << " ";
    cout << "\nNoisy moves: ";
    for(int i = 0; i < nrNoisy; i++)
      cout << toString(noisy[i]) << " ";

  }*/

  if(depth == 1) {
    return nrMoves;
  }

  uint64_t nodes = 0;

  for(int i = 0; i < nrMoves; i++) {
    uint16_t move = moves[i];

    makeMove(board, move);

    bool p = print;

    uint64_t x = perft(board, depth - 1, p);

    nodes += x;
    undoMove(board, move);
  }
  return nodes;
}
