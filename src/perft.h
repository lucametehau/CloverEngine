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

template <bool RootNode>
uint64_t perft(Board& board, int depth) {
    MoveList moves, quiets, noisies;
    int nrMoves = gen_legal_moves(board, moves);
    int nrQuiets = gen_legal_quiet_moves(board, quiets);
    int nrNoisies = gen_legal_noisy_moves(board, noisies);
    if (nrQuiets + nrNoisies != nrMoves) {
        board.print();
        for (int i = 0; i < nrQuiets; i++) std::cout << move_to_string(quiets[i], false) << " ";
        std::cout << "\n";
        for (int i = 0; i < nrNoisies; i++) std::cout << move_to_string(noisies[i], false) << " ";
        std::cout << "\n";
        for (int i = 0; i < nrMoves; i++) std::cout << move_to_string(moves[i], false) << " ";
        std::cout << "\n";
        exit(0);
    }
    if (depth == 1) return nrMoves;

    uint64_t nodes = 0;

    for (int i = 0; i < nrMoves; i++) {
        Move move = moves[i];
        board.make_move(move);
        uint64_t x = perft<false>(board, depth - 1);
        nodes += x;
        board.undo_move(move);
        if constexpr (RootNode)
            std::cout << move_to_string(move, false) << " " << x << "\n";
    }
    return nodes;
}
