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
#include "move.h"

uint64_t perft(Board& board, int depth, bool print = 0) {
    uint16_t moves[256];
    int nrMoves = genLegal(board, moves);

    if (depth == 1) {
        return nrMoves;
    }

    uint64_t nodes = 0;

    for (int i = 0; i < nrMoves; i++) {
        uint16_t move = moves[i];

        makeMove(board, move);

        bool p = print;

        uint64_t x = perft(board, depth - 1, p);

        nodes += x;
        undoMove(board, move);
    }
    return nodes;
}
