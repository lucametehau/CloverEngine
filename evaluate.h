#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"

const int TEMPO = 20;

int evaluate(Board &board) {

  //double eval2 = board.NN.getOutput();

  return int(board.NN.getOutput()) * (board.turn == WHITE ? 1 : -1) + TEMPO;
}
