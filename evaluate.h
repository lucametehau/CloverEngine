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

const int phaseVal[] = { 0, 0, 1, 1, 2, 4 };
const int maxPhase = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];

int evaluate(Board &board) {
    //board.print();
    int eval = int(board.NN.getOutput());

    bool turn = board.turn;
    int phase = count(board.bb[WP] | board.bb[BP]) * phaseVal[PAWN] +
        count(board.bb[WN] | board.bb[BN]) * phaseVal[KNIGHT] +
        count(board.bb[WB] | board.bb[BB]) * phaseVal[BISHOP] +
        count(board.bb[WR] | board.bb[BR]) * phaseVal[ROOK] +
        count(board.bb[WQ] | board.bb[BQ]) * phaseVal[QUEEN];

    //eval += contempt;

    eval = eval * (maxPhase / 2 + phase) / maxPhase;

    return eval * (turn == WHITE ? 1 : -1) + TEMPO;
}