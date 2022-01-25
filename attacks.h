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
#include "board.h"

uint64_t rookAttacksMask[64], bishopAttacksMask[64];
uint64_t pawnAttacksMask[2][64];
uint64_t rookTable[64][4096], bishopTable[64][1024];
uint64_t raysMask[64][8];
uint64_t knightBBAttacks[64], kingBBAttacks[64];
uint64_t kingRingMask[64], kingSquareMask[64], pawnShieldMask[64];

inline uint64_t genAttacksBishopSlow(uint64_t blockers, int sq) {
  uint64_t attacks = 0;

  attacks |= raysMask[sq][NORTHWEST];
  if(raysMask[sq][NORTHWEST] & blockers) {
    int sq2 = Sq(lsb(raysMask[sq][NORTHWEST] & blockers));
    attacks &= ~raysMask[sq2][NORTHWEST];
  }
  attacks |= raysMask[sq][NORTHEAST];
  if(raysMask[sq][NORTHEAST] & blockers) {
    int sq2 = Sq(lsb(raysMask[sq][NORTHEAST] & blockers));
    attacks &= ~raysMask[sq2][NORTHEAST];
  }
  attacks |= raysMask[sq][SOUTHWEST];
  if(raysMask[sq][SOUTHWEST] & blockers) {
    int sq2 = Sq(raysMask[sq][SOUTHWEST] & blockers);
    attacks &= ~raysMask[sq2][SOUTHWEST];
  }
  attacks |= raysMask[sq][SOUTHEAST];
  if(raysMask[sq][SOUTHEAST] & blockers) {
    int sq2 = Sq(raysMask[sq][SOUTHEAST] & blockers);
    attacks &= ~raysMask[sq2][SOUTHEAST];
  }
  return attacks;
}

inline uint64_t genAttacksRookSlow(uint64_t blockers, int sq) {
  uint64_t attacks = 0;

  attacks |= raysMask[sq][NORTH];
  if(raysMask[sq][NORTH] & blockers) {
    int sq2 = Sq(lsb(raysMask[sq][NORTH] & blockers));
    attacks &= ~raysMask[sq2][NORTH];
  }
  attacks |= raysMask[sq][SOUTH];
  if(raysMask[sq][SOUTH] & blockers) {
    int sq2 = Sq(raysMask[sq][SOUTH] & blockers);
    attacks &= ~raysMask[sq2][SOUTH];
  }
  attacks |= raysMask[sq][WEST];
  if(raysMask[sq][WEST] & blockers) {
    int sq2 = Sq(raysMask[sq][WEST] & blockers);
    attacks &= ~raysMask[sq2][WEST];
  }
  attacks |= raysMask[sq][EAST];
  if(raysMask[sq][EAST] & blockers) {
    int sq2 = Sq(lsb(raysMask[sq][EAST] & blockers));
    attacks &= ~raysMask[sq2][EAST];
  }
  return attacks;
}

inline uint64_t getBlocker(uint64_t mask, int ind) {
  int nr = __builtin_popcountll(mask);
  uint64_t blockers = 0;
  for(int i = 0; i < nr; i++) {
    uint64_t b = lsb(mask);
    if(ind & (1 << i))
      blockers |= b;
    mask ^= b;
  }
  return blockers;
}

inline void initKnightAndKingAttacks() {
  for(int i = 0; i < 64; i++) {
    int rank = i / 8, file = i % 8;
    for(int j = 0; j < 8; j++) {
      int rankTo = rank + knightDir[j].first, fileTo = file + knightDir[j].second;
      if(inTable(rankTo, fileTo))
        knightBBAttacks[i] |= (1ULL << getSq(rankTo, fileTo));
    }
    for(int j = 0; j < 8; j++) {
      int rankTo = rank + kingDir[j].first, fileTo = file + kingDir[j].second;
      if(inTable(rankTo, fileTo))
        kingBBAttacks[i] |= (1ULL << getSq(rankTo, fileTo));
    }
  }

  /// king area masks

  for(int i = 0; i < 64; i++) {
    int rank = i / 8, file = i % 8, sq = 0; /// board.king(color)
    if(rank < 1)
      sq = 1 * 8;
    else if(rank > 6)
      sq = 6 * 8;
    else
      sq = rank * 8;
    if(file < 1)
      sq += 1;
    else if(file > 6)
      sq += 6;
    else
      sq += file;

    kingRingMask[i] = (1ULL << sq) | kingBBAttacks[sq];
    kingSquareMask[i] = (1ULL << i) | kingBBAttacks[i];

    pawnShieldMask[i] = 0;

    if(rank <= 1)
      pawnShieldMask[i] = shift(WHITE, NORTH, kingSquareMask[i]);
    else if(rank >= 6)
      pawnShieldMask[i] = shift(WHITE, SOUTH, kingSquareMask[i]);
  }
}

inline void initPawnAttacks() {
  for(int i = 0; i < 64; i++) {
    int file = i % 8;
    if(file > 0) {
      if(i + 7 < 64)
        pawnAttacksMask[WHITE][i] |= (1ULL << (i + 7));
      if(i >= 9)
        pawnAttacksMask[BLACK][i] |= (1ULL << (i - 9));
    }
    if(file < 7) {
      if(i + 9 < 64)
        pawnAttacksMask[WHITE][i] |= (1ULL << (i + 9));
      if(i >= 7)
        pawnAttacksMask[BLACK][i] |= (1ULL << (i - 7));
    }
  }
}

inline void initRays() {
  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int r, f, sq = getSq(rank, file);
      r = rank, f = file;
      while(r > 0)
        r--, raysMask[sq][SOUTH] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(r < 7)
        r++, raysMask[sq][NORTH] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(f > 0)
        f--, raysMask[sq][WEST] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(f < 7)
        f++, raysMask[sq][EAST] |= (1ULL << getSq(r, f));

      r = rank, f = file;
      while(r > 0 && f > 0)
        r--, f--, raysMask[sq][SOUTHWEST] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(r < 7 && f > 0)
        r++, f--, raysMask[sq][NORTHWEST] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(r > 0 && f < 7)
        r--, f++, raysMask[sq][SOUTHEAST] |= (1ULL << getSq(r, f));
      r = rank, f = file;
      while(r < 7 && f < 7)
        r++, f++, raysMask[sq][NORTHEAST] |= (1ULL << getSq(r, f));

    }
  }
}

inline void initRookMagic() {
  //uint64_t edge = (fileMask[0] | fileMask[7] | rankMask[0] | rankMask[7]);
  for(int sq = 0; sq < 64; sq++) {
    rookAttacksMask[sq] = 0;
    rookAttacksMask[sq] |= raysMask[sq][NORTH] & ~rankMask[7];
    rookAttacksMask[sq] |= raysMask[sq][SOUTH] & ~rankMask[0];
    rookAttacksMask[sq] |= raysMask[sq][EAST] & ~fileMask[7];
    rookAttacksMask[sq] |= raysMask[sq][WEST] & ~fileMask[0];
    for(int blockerInd = 0; blockerInd < (1 << rookIndexBits[sq]); blockerInd++) {
      uint64_t blockers = getBlocker(rookAttacksMask[sq], blockerInd);
      rookTable[sq][(blockers * rookMagics[sq]) >> (64 - rookIndexBits[sq])] = genAttacksRookSlow(blockers, sq);
    }
  }
}

inline void initBishopMagic() {
  uint64_t edge = (fileMask[0] | fileMask[7] | rankMask[0] | rankMask[7]);
  for(int sq = 0; sq < 64; sq++) {
    bishopAttacksMask[sq] = (raysMask[sq][NORTHWEST] | raysMask[sq][SOUTHWEST] | raysMask[sq][NORTHEAST] | raysMask[sq][SOUTHEAST]) & (~edge);
    for(int blockerInd = 0; blockerInd < (1 << bishopIndexBits[sq]); blockerInd++) {
      uint64_t blockers = getBlocker(bishopAttacksMask[sq], blockerInd);
      bishopTable[sq][(blockers * bishopMagics[sq]) >> (64 - bishopIndexBits[sq])] = genAttacksBishopSlow(blockers, sq);
    }
  }
}

inline void initAttacks() {
  initRays();
  initPawnAttacks();
  initKnightAndKingAttacks();
  initBishopMagic();
  initRookMagic();
}

/// these 2 are not very useful, but I might use them for readability

inline uint64_t genAttacksPawn(int color, int sq) {
  return pawnAttacksMask[color][sq];
}

inline uint64_t genAttacksKnight(int sq) {
  return knightBBAttacks[sq];
}

inline uint64_t genAttacksBishop(uint64_t blockers, int sq) {
  blockers &= bishopAttacksMask[sq];
  return bishopTable[sq][(blockers * bishopMagics[sq]) >> (64 - bishopIndexBits[sq])];
}

inline uint64_t genAttacksRook(uint64_t blockers, int sq) {
  blockers &= rookAttacksMask[sq];
  return rookTable[sq][(blockers * rookMagics[sq]) >> (64 - rookIndexBits[sq])];
}

inline uint64_t genAttacksQueen(uint64_t blockers, int sq) {
  return genAttacksBishop(blockers, sq) | genAttacksRook(blockers, sq);
}

inline uint64_t genAttacksSq(uint64_t blockers, int sq, int pieceType) {
  switch(pieceType) {
    case KNIGHT:
      return knightBBAttacks[sq];
    case BISHOP:
      return genAttacksBishop(blockers, sq);
    case ROOK:
      return genAttacksRook(blockers, sq);
    case QUEEN:
      return genAttacksQueen(blockers, sq);
  }
}

inline uint64_t getAttackers(Board &board, int color, uint64_t blockers, int sq) {
  return (pawnAttacksMask[color ^ 1][sq] & board.bb[getType(PAWN, color)]) |
         (knightBBAttacks[sq] & board.bb[getType(KNIGHT, color)]) | (genAttacksBishop(blockers, sq) & board.diagSliders(color)) |
         (genAttacksRook(blockers, sq) & board.orthSliders(color)) | (kingBBAttacks[sq] & board.bb[getType(KING, color)]);
}

/// same as the below one, only difference is that b is known

inline uint64_t getPawnAttacks(int color, uint64_t b) {
  int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
  return shift(color, NORTHWEST, b & ~fileMask[fileA]) | shift(color, NORTHEAST, b & ~fileMask[fileH]);
}

inline uint64_t pawnAttacks(Board &board, int color) {
  uint64_t b = board.bb[getType(PAWN, color)];
  int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
  return shift(color, NORTHWEST, b & ~fileMask[fileA]) | shift(color, NORTHEAST, b & ~fileMask[fileH]);
}



