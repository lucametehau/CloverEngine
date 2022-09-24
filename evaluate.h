/*
  Clover is a UCI chess playing engine authored by Luca Metehau.
  <https://github.com/lucametehau/CloverEngine>

  Clover is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clover is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"

const int TEMPO = 20;

int evaluate(Board &board) {
    int eval = int(board.NN.getOutput(board.turn));
    //board.print();
    //std::cout << board.turn << " " << eval << "\n";

    /*NetInput inp = board.toNetInput();

    int amogus = int(board.NN.kekw(inp));

    if (abs(eval - amogus) > 1) {
        board.print();
        std::cout << board.NN.getOutput() << " " << board.NN.kekw(inp) << "\n";
        std::cout << eval << " " << amogus << "\n";
        exit(0);
    }*/

    //bool turn = board.turn;

    return eval + TEMPO;
}