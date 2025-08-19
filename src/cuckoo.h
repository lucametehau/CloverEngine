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
#include "attacks.h"
#include "defs.h"

namespace cuckoo
{

constexpr int CUCKOO_SIZE = (1 << 13);
constexpr int CUCKOO_MASK = CUCKOO_SIZE - 1;

inline std::array<Key, CUCKOO_SIZE> cuckoo;
inline std::array<Move, CUCKOO_SIZE> cuckoo_move;

inline int hash1(const Key key)
{
    return key & CUCKOO_MASK;
}
inline int hash2(const Key key)
{
    return (key >> 16) & CUCKOO_MASK;
}

inline void init()
{
    int count = 0;
    for (Piece piece = Pieces::BlackPawn; piece <= Pieces::WhiteKing; piece++)
    {
        // no pawns
        if (piece.type() == PieceTypes::PAWN)
            continue;
        for (Square from = 0; from < 64; from++)
        {
            for (Square to = from + 1; to < 64; to++)
            {
                if (attacks::genAttacksSq(Bitboard(0ull), from, piece.type()).has_square(to))
                {
                    Move move = Move(from, to, NO_TYPE);
                    Key key = hashKey[piece][from] ^ hashKey[piece][to] ^ 1;
                    int cuckoo_ind = hash1(key);
                    while (true)
                    {
                        std::swap(cuckoo[cuckoo_ind], key);
                        std::swap(cuckoo_move[cuckoo_ind], move);
                        if (!move)
                            break;
                        cuckoo_ind = cuckoo_ind == hash1(key) ? hash2(key) : hash1(key);
                    }
                    count++;
                }
            }
        }
    }
    assert(count == 3668);
}

}; // namespace cuckoo