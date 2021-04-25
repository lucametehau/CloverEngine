#include "move.h"
#pragma once

uint64_t perft(Board &board, int depth) {
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
    /*if(board.ply == 0) {
      for(int i = 0; i < nrMoves; i++)
        cout << toString(moves[i]) << ": 1\n";
    }*/
    return nrMoves;
  }
  uint64_t nodes = 0;

  for(int i = 0; i < nrMoves; i++) {
    uint16_t move = moves[i];
    makeMove(board, move);
    uint64_t x = perft(board, depth - 1);
    //std::cout << "depth = " << depth << ", move = " << toString(move) << "\n";
    /*if(board.ply == 1) {
      cout << toString(move) << ": " << x << "\n";
    }*/
    nodes += x;
    undoMove(board, move);
  }
  return nodes;
}
