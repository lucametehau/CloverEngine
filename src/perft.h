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
    if (depth == 0) return 1;

    MoveList moves;
    int nrMoves = board.gen_legal_moves<MovegenTypes::ALL_MOVES>(moves);

    uint64_t nodes = 0;

    HistoricalState next_state;
    for (int i = 0; i < nrMoves; i++) {
        if (!is_legal(board, moves[i])) continue;
        Move move = moves[i];
        //std::cout << move.to_string() << " at depth " << depth << '\n';
        board.make_move(move, next_state);
        uint64_t x;
        x = perft<false>(board, depth - 1);
        nodes += x;
        board.undo_move(move);
        if constexpr (RootNode)
            std::cout << move.to_string() << ": " << x << "\n";
    }
    return nodes;
}
