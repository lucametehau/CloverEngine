#pragma once
#include "board.h"
#include "defs.h"
#include "attacks.h"

inline bool isSqAttacked(Board &board, int color, int sq) {
  /*
  /// pawn attacks
  if(pawnAttacksMask[color ^ 1][sq] & board.bb[getType(PAWN, color)])
    return 1;
  if(knightBBAttacks[sq] & board.bb[getType(KNIGHT, color)])
    return 1;
  uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];
  if(genAttacksBishop(all, sq) & board.diagSliders(color))
    return 1;
  if(genAttacksRook(all, sq) & board.orthSliders(color))
    return 1;
  if(kingBBAttacks[sq] & board.bb[getType(KING, color)])
    return 1;
  */
  return getAttackers(board, color, board.pieces[WHITE] | board.pieces[BLACK], sq);
}

inline bool inCheck(Board &board) {
  return isSqAttacked(board, 1 ^ board.turn, board.king(board.turn));
}

inline uint16_t* addQuiets(uint16_t *moves, int &nrMoves, int pos, uint64_t att) {
  //nrMoves += count(att);
  while(att) {
    uint64_t b = lsb(att);
    int pos2 = Sq(b);
    *(moves++) = getMove(pos, pos2, 0, NEUT);
    nrMoves++;
    att ^= b;
  }
  return moves;
}

inline uint16_t* addCaps(uint16_t *moves, int &nrMoves, int pos, uint64_t att) {
  while(att) {
    uint64_t b = lsb(att);
    int pos2 = Sq(b);
    *(moves++) = getMove(pos, pos2, 0, NEUT);
    nrMoves++;
    att ^= b;
  }
  return moves;
}

inline uint16_t* addMoves(uint16_t *moves, int &nrMoves, int pos, uint64_t att) {
  while(att) {
    uint64_t b = lsb(att);
    int pos2 = Sq(b);
    *(moves++) = getMove(pos, pos2, 0, NEUT);
    nrMoves++;
    att ^= b;
  }
  return moves;
}

