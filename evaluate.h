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

int pawnScaleStart = 81;
int pawnScaleStep = 0;
int pawnsOn1Flank = -2;

int scale(Board& board) {
    int pawnCount = count(board.bb[getType(PAWN, board.turn)]);
    uint64_t allPawns = board.bb[WP] | board.bb[BP];

    return std::min(128, pawnScaleStart + pawnScaleStep * pawnCount - pawnsOn1Flank * !((allPawns & flankMask[0]) && (allPawns & flankMask[1])));
}

int evaluate(Board &board) {
    int eval = board.NN.getOutput(board.turn);

    eval = eval * scale(board) / 128;

    return eval + TEMPO;
}