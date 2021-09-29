#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"

const int TEMPO = 20;

int evaluate(Board &board) {

  double eval2 = board.NN.getOutput();

  //board.print();

  /*NetInput input = board.toNetInput();
  double score = board.NN.calc(input);

  if(abs(eval2 - score) > 1e-9) {
    board.print();
    std::cout << eval2 << " " << score << "\n";
    assert(0);
  }*/

  return int(eval2) * (board.turn == WHITE ? 1 : -1) + TEMPO;
}
