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
#include "magic.h"
#include "defs.h"

namespace attacks {
std::array<Bitboard, 64> rookAttacksMask, bishopAttacksMask;
MultiArray<Bitboard, 2, 64> pawnAttacksMask;
MultiArray<Bitboard, 64, 4096> rookTable;
MultiArray<Bitboard, 64, 512> bishopTable;
MultiArray<Bitboard, 64, 8> raysMask;
std::array<Bitboard, 64> knightBBAttacks, kingBBAttacks;
std::array<Bitboard, 64> kingRingMask, kingSquareMask, pawnShieldMask;

inline Bitboard genAttacksBishopSlow(Bitboard blockers, Square sq) {
    Bitboard attacks;

    attacks |= raysMask[sq][NORTHWEST];
    if (raysMask[sq][NORTHWEST] & blockers) {
        Square sq2 = (raysMask[sq][NORTHWEST] & blockers).get_lsb_square();
        attacks &= ~raysMask[sq2][NORTHWEST];
    }
    attacks |= raysMask[sq][NORTHEAST];
    if (raysMask[sq][NORTHEAST] & blockers) {
        Square sq2 = (raysMask[sq][NORTHEAST] & blockers).get_lsb_square();
        attacks &= ~raysMask[sq2][NORTHEAST];
    }
    attacks |= raysMask[sq][SOUTHWEST];
    if (raysMask[sq][SOUTHWEST] & blockers) {
        Square sq2 = (raysMask[sq][SOUTHWEST] & blockers).get_msb_square(); // sq
        attacks &= ~raysMask[sq2][SOUTHWEST];
    }
    attacks |= raysMask[sq][SOUTHEAST];
    if (raysMask[sq][SOUTHEAST] & blockers) {
        Square sq2 = (raysMask[sq][SOUTHEAST] & blockers).get_msb_square(); // sq
        attacks &= ~raysMask[sq2][SOUTHEAST];
    }
    return attacks;
}

inline Bitboard genAttacksRookSlow(Bitboard blockers, Square sq) {
    Bitboard attacks;

    attacks |= raysMask[sq][NORTH];
    if (raysMask[sq][NORTH] & blockers) {
        Square sq2 = (raysMask[sq][NORTH] & blockers).get_lsb_square();
        attacks &= ~raysMask[sq2][NORTH];
    }
    attacks |= raysMask[sq][SOUTH];
    if (raysMask[sq][SOUTH] & blockers) {
        Square sq2 = (raysMask[sq][SOUTH] & blockers).get_msb_square(); // sq
        attacks &= ~raysMask[sq2][SOUTH];
    }
    attacks |= raysMask[sq][WEST];
    if (raysMask[sq][WEST] & blockers) {
        Square sq2 = (raysMask[sq][WEST] & blockers).get_msb_square(); // sq
        attacks &= ~raysMask[sq2][WEST];
    }
    attacks |= raysMask[sq][EAST];
    if (raysMask[sq][EAST] & blockers) {
        Square sq2 = (raysMask[sq][EAST] & blockers).get_lsb_square();
        attacks &= ~raysMask[sq2][EAST];
    }
    return attacks;
}

inline Bitboard get_blockers(Bitboard mask, int ind) {
    int nr = mask.count();
    Bitboard blockers;
    for (int i = 0; i < nr; i++) {
        Bitboard lsb = mask.lsb();
        if (ind & (1ULL << i)) blockers |= lsb;
        mask ^= lsb;
    }
    return blockers;
}

inline void initKnightAndKingAttacks() {
    for (Square i = 0; i < 64; i++) {
        int rank = i / 8, file = i % 8;
        for (int j = 0; j < 8; j++) {
            int rankTo = rank + knightDir[j].first, fileTo = file + knightDir[j].second;
            if (inside_board(rankTo, fileTo))
                knightBBAttacks[i] |= (1ULL << get_sq(rankTo, fileTo));
        }
        for (int j = 0; j < 8; j++) {
            int rankTo = rank + kingDir[j].first, fileTo = file + kingDir[j].second;
            if (inside_board(rankTo, fileTo))
                kingBBAttacks[i] |= (1ULL << get_sq(rankTo, fileTo));
        }
    }

    /// king area masks

    for (Square i = 0; i < 64; i++) {
        int rank = i / 8, file = i % 8, sq = 0; /// board.king(color)
        if (rank < 1)
            sq = 1 * 8;
        else if (rank > 6)
            sq = 6 * 8;
        else
            sq = rank * 8;
        if (file < 1)
            sq += 1;
        else if (file > 6)
            sq += 6;
        else
            sq += file;

        kingRingMask[i] = (1ULL << sq) | kingBBAttacks[sq];
        kingSquareMask[i] = (1ULL << i) | kingBBAttacks[i];

        if (rank <= 1)
            pawnShieldMask[i] = shift(WHITE, NORTH, kingSquareMask[i]);
        else if (rank >= 6)
            pawnShieldMask[i] = shift(WHITE, SOUTH, kingSquareMask[i]);
    }
}

inline void initPawnAttacks() {
    for (Square i = 0; i < 64; i++) {
        int file = i % 8;
        if (file > 0) {
            if (i + 7 < 64)
                pawnAttacksMask[WHITE][i] |= (1ULL << (i + 7));
            if (i >= 9)
                pawnAttacksMask[BLACK][i] |= (1ULL << (i - 9));
        }
        if (file < 7) {
            if (i + 9 < 64)
                pawnAttacksMask[WHITE][i] |= (1ULL << (i + 9));
            if (i >= 7)
                pawnAttacksMask[BLACK][i] |= (1ULL << (i - 7));
        }
    }
}

inline void initRays() {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int r, f;
            Square sq = get_sq(rank, file);
            r = rank, f = file;
            while (r > 0)
                r--, raysMask[sq][SOUTH] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (r < 7)
                r++, raysMask[sq][NORTH] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (f > 0)
                f--, raysMask[sq][WEST] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (f < 7)
                f++, raysMask[sq][EAST] |= (1ULL << get_sq(r, f));

            r = rank, f = file;
            while (r > 0 && f > 0)
                r--, f--, raysMask[sq][SOUTHWEST] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (r < 7 && f > 0)
                r++, f--, raysMask[sq][NORTHWEST] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (r > 0 && f < 7)
                r--, f++, raysMask[sq][SOUTHEAST] |= (1ULL << get_sq(r, f));
            r = rank, f = file;
            while (r < 7 && f < 7)
                r++, f++, raysMask[sq][NORTHEAST] |= (1ULL << get_sq(r, f));

        }
    }
}

inline void initRookMagic() {
    for (Square sq = 0; sq < 64; sq++) {
        rookAttacksMask[sq] |= raysMask[sq][NORTH] & ~rank_mask[7];
        rookAttacksMask[sq] |= raysMask[sq][SOUTH] & ~rank_mask[0];
        rookAttacksMask[sq] |= raysMask[sq][EAST] & ~file_mask[7];
        rookAttacksMask[sq] |= raysMask[sq][WEST] & ~file_mask[0];
        for (int blockerInd = 0; blockerInd < (1 << rookIndexBits[sq]); blockerInd++) {
            Bitboard blockers = get_blockers(rookAttacksMask[sq], blockerInd);
#ifndef PEXT_GOOD
            rookTable[sq][(blockers * rookMagics[sq]) >> (64 - rookIndexBits[sq])] = genAttacksRookSlow(blockers, sq);
#else
            rookTable[sq][_pext_u64(blockers, rookAttacksMask[sq])] = genAttacksRookSlow(blockers, sq);
#endif
        }
    }
}

inline void initBishopMagic() {
    Bitboard edge = (file_mask[0] | file_mask[7] | rank_mask[0] | rank_mask[7]);
    for (Square sq = 0; sq < 64; sq++) {
        bishopAttacksMask[sq] = (raysMask[sq][NORTHWEST] | raysMask[sq][SOUTHWEST] | raysMask[sq][NORTHEAST] | raysMask[sq][SOUTHEAST]) & (~edge);
        for (int blockerInd = 0; blockerInd < (1 << bishopIndexBits[sq]); blockerInd++) {
            Bitboard blockers = get_blockers(bishopAttacksMask[sq], blockerInd);
#ifndef PEXT_GOOD
            bishopTable[sq][(blockers * bishopMagics[sq]) >> (64 - bishopIndexBits[sq])] = genAttacksBishopSlow(blockers, sq);
#else
            bishopTable[sq][_pext_u64(blockers, bishopAttacksMask[sq])] = genAttacksBishopSlow(blockers, sq);
#endif
        }
    }
}

inline void init_attacks() {
    initRays();
    initPawnAttacks();
    initKnightAndKingAttacks();
    initBishopMagic();
    initRookMagic();
}

inline Bitboard genAttacksPawn(bool color, Square sq) {
    return pawnAttacksMask[color][sq];
}

inline Bitboard genAttacksKnight(Square sq) {
    return knightBBAttacks[sq];
}

inline Bitboard genAttacksBishop(Bitboard blockers, Square sq) {
#ifndef PEXT_GOOD
    return bishopTable[sq][((blockers & bishopAttacksMask[sq]) * bishopMagics[sq]) >> (64 - bishopIndexBits[sq])];
#else
    return bishopTable[sq][_pext_u64(blockers, bishopAttacksMask[sq])];
#endif
}

inline Bitboard genAttacksRook(Bitboard blockers, Square sq) {
#ifndef  PEXT_GOOD
    return rookTable[sq][((blockers & rookAttacksMask[sq]) * rookMagics[sq]) >> (64 - rookIndexBits[sq])];
#else
    return rookTable[sq][_pext_u64(blockers, rookAttacksMask[sq])];
#endif
}

inline Bitboard genAttacksQueen(Bitboard blockers, Square sq) {
    return genAttacksBishop(blockers, sq) | genAttacksRook(blockers, sq);
}

inline Bitboard genAttacksKing(Square sq) {
    return kingBBAttacks[sq];
}

inline Bitboard genAttacksSq(Bitboard blockers, Square sq, Piece pieceType) {
    switch (pieceType) {
    case KNIGHT:
        return genAttacksKnight(sq);
    case BISHOP:
        return genAttacksBishop(blockers, sq);
    case ROOK:
        return genAttacksRook(blockers, sq);
    case QUEEN:
        return genAttacksQueen(blockers, sq);
    }
    assert(0);
    return Bitboard();
}

/// same as the below one, only difference is that b is known
inline Bitboard getPawnAttacks(int color, Bitboard b) {
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    return shift(color, NORTHWEST, b & ~file_mask[fileA]) | shift(color, NORTHEAST, b & ~file_mask[fileH]);
}
};


