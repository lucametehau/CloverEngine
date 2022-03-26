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
#include "board.h"
#include "defs.h"
#include "attacks.h"

inline bool isSqAttacked(Board& board, int color, int sq) {
    return getAttackers(board, color, board.pieces[WHITE] | board.pieces[BLACK], sq);
}

inline bool inCheck(Board& board) {
    return isSqAttacked(board, 1 ^ board.turn, board.king(board.turn));
}

inline uint16_t* addQuiets(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

inline uint16_t* addCaps(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

inline uint16_t* addMoves(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

