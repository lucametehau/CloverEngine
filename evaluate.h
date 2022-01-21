#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"

const int TEMPO = 20;

int evaluate(Board& board) {

    //float eval2 = board.NN.getOutput();

    //board.print();

    /*NetInput input = board.toNetInput();
    float score = board.NN.calc(input);

    if (abs(eval2 - score) > 1e-1) {
      board.print();
      std::cout << eval2 << " " << score << "\n";
      assert(0);
    }*/

    return int(board.NN.getOutput()) * (board.turn == WHITE ? 1 : -1) + TEMPO;
}
