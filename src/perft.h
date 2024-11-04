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
#include "movegen.h"

template <bool RootNode>
uint64_t perft(Board& board, int depth) {
    MoveList moves;
    int nrMoves = board.gen_legal_moves(moves);
    if (depth == 1) return nrMoves;

    uint64_t nodes = 0;

    for (int i = 0; i < nrMoves; i++) {
        Move move = moves[i];
        board.make_move(move);
        uint64_t x = perft<false>(board, depth - 1);
        nodes += x;
        board.undo_move(move);
        if constexpr (RootNode)
            std::cout << move.to_string() << ": " << x << "\n";
    }
    return nodes;
}
