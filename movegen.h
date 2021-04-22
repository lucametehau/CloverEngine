#include "board.h"
#include "defs.h"
#include "attacks.h"
#pragma once

using namespace std;

inline bool isSqAttacked(Board *board, int color, int sq) {
  /*
  /// pawn attacks
  if(pawnAttacksMask[color ^ 1][sq] & board->bb[getType(PAWN, color)])
    return 1;
  if(knightBBAttacks[sq] & board->bb[getType(KNIGHT, color)])
    return 1;
  uint64_t all = board->pieces[WHITE] | board->pieces[BLACK];
  if(genAttacksBishop(all, sq) & board->diagSliders(color))
    return 1;
  if(genAttacksRook(all, sq) & board->orthSliders(color))
    return 1;
  if(kingBBAttacks[sq] & board->bb[getType(KING, color)])
    return 1;
  */
  return getAttackers(board, color, board->pieces[WHITE] | board->pieces[BLACK], sq);
}

inline bool inCheck(Board *board) {
  return isSqAttacked(board, 1 ^ board->turn, board->king(board->turn));
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

/*vector <uint16_t> genMovesKnights(Board *board, int pieceType) {
  vector <uint16_t> moves;
  uint64_t knights = board->bb[pieceType], notOwn = ~board->pieces[board->turn];
  while(knights) {
    uint64_t b = lsb(knights);
    int pos = Sq(b);
    uint64_t attacks = knightBBAttacks[pos] & notOwn;
    addMoves(moves, board, pos, attacks);
    knights ^= b;
  }
  return moves;
}

vector <uint16_t> genMovesBishops(Board *board, int pieceType) {
  vector <uint16_t> moves;
  uint64_t bishops = board->bb[pieceType], notOwn = ~board->pieces[board->turn], all = board->pieces[WHITE] | board->pieces[BLACK];
  while(bishops) {
    uint64_t b = lsb(bishops);
    int pos = Sq(b);
    uint64_t attacks = genAttacksBishop(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    bishops ^= b;
  }
  return moves;
}

vector <uint16_t> genMovesRooks(Board *board, int pieceType) {
  vector <uint16_t> moves;
  uint64_t rooks = board->bb[pieceType], notOwn = ~board->pieces[board->turn], all = board->pieces[WHITE] | board->pieces[BLACK];
  while(rooks) {
    uint64_t b = lsb(rooks);
    int pos = Sq(b);
    uint64_t attacks = genAttacksRook(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    rooks ^= b;
  }
  return moves;
}

vector <uint16_t> genMovesQueens(Board *board, int pieceType) {
  vector <uint16_t> moves;
  uint64_t queens = board->bb[pieceType], notOwn = ~board->pieces[board->turn], all = board->pieces[WHITE] | board->pieces[BLACK];
  bool turn = board->turn;
  while(queens) {
    uint64_t b = lsb(queens);
    int pos = Sq(b);
    uint64_t attacks = genAttacksQueen(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    queens ^= b;
  }
  return moves;
}

vector <uint16_t> genMovesKing(Board *board, int pieceType) {
  vector <uint16_t> moves;
  int pos = Sq(board->bb[pieceType]);
  uint64_t att = kingBBAttacks[pos] & (~board->pieces[board->turn]);
  addMoves(moves, board, pos, att);
  return moves;
}

vector <uint16_t> genCastles(Board *board) {
  int pos = board->king(board->turn), rank = pos / 8, file = pos % 8;
  uint64_t all = board->pieces[WHITE] | board->pieces[BLACK];
  bool turn = board->turn;

  vector <uint16_t> moves;
  if(board->castleRights & (1 << (2 * turn))) {
    int rankTo = rank, fileTo = file - 2, posTo = getSq(rankTo, fileTo);
    //cout << ((all >> (pos - 3)) & 7) << " XDDDDDDDD????????\n";
    if(!((all >> (pos - 3)) & 7)) {
      //cout << pos << " "  << isSqAttacked(board, 1 ^ turn, pos) << " " << isSqAttacked(board, 1 ^ turn, pos - 1) << " " << isSqAttacked(board, 1 ^ turn, pos - 2) << "\n";
      if(!isSqAttacked(board, 1 ^ turn, pos) && !isSqAttacked(board, 1 ^ turn, pos - 1))
        moves.push_back(getMove(pos, posTo, 0, CASTLE));
    }
  }
  if(board->castleRights & (1 << (2 * turn + 1))) {
    int rankTo = rank, fileTo = file + 2, posTo = getSq(rankTo, fileTo);
    if(!((all >> (pos + 1)) & 3)) {
      if(!isSqAttacked(board, 1 ^ turn, pos) && !isSqAttacked(board, 1 ^ turn, pos + 1))
        moves.push_back(getMove(pos, posTo, 0, CASTLE));
    }
  }
  return moves;
}

vector <uint16_t> genMovesWhitePawns(Board *board) {
  uint64_t pawns = board->bb[WP], all = board->pieces[WHITE] | board->pieces[BLACK];
  vector <uint16_t> moves;
  while(pawns) {
    uint64_t b = lsb(pawns);
    int pos = Sq(b), rank = pos / 8, file = pos % 8, rankTo, fileTo, posTo;
    rankTo = rank + 1, posTo = pos + 8;
    if(board->board[posTo] == '.') {
      if(rankTo < 7)
        moves.push_back(getMove(pos, posTo, 0, NEUT));
      else { // promotion
        for(int i = 0; i < 4; i++)
          moves.push_back(getMove(pos, posTo, i, PROMOTION));
      }
      if(rank == 1) {
        posTo = pos + 16;
        if(board->board[posTo] == '.')
          moves.push_back(getMove(pos, posTo, 0, NEUT));
      }
    }
    for(int i = 0; i < 2; i++) {
      rankTo = rank + pawnCapDirWhite[i].first, fileTo = file + pawnCapDirWhite[i].second, posTo = getSq(rankTo, fileTo);
      if(inTable(rankTo, fileTo)) {
        if((board->pieces[BLACK] >> posTo) & 1) {
          if(rankTo < 7) { /// capture
            moves.push_back(getMove(pos, posTo, 0, NEUT));
          } else { /// capture + promotion
            for(int i = 0; i < 4; i++)
              moves.push_back(getMove(pos, posTo, i, PROMOTION));
          }
        } else if(posTo == board->enPas) { /// maybe we have an en passant
          moves.push_back(getMove(pos, posTo, 0, ENPASSANT));
        }
      }
    }
    pawns ^= b;
  }
  return moves;
}

vector <uint16_t> genMovesBlackPawns(Board *board) {
  uint64_t pawns = board->bb[BP], all = board->pieces[WHITE] | board->pieces[BLACK];
  vector <uint16_t> moves;
  while(pawns) {
    uint64_t b = lsb(pawns);
    int pos = Sq(b), rank = pos / 8, file = pos % 8, rankTo, fileTo, posTo;
    rankTo = rank - 1, fileTo = file, posTo = pos - 8;
    if(board->board[posTo] == '.') {
      if(rankTo > 0)
        moves.push_back(getMove(pos, posTo, 0, NEUT));
      else { // promotion
        for(int i = 0; i < 4; i++)
          moves.push_back(getMove(pos, posTo, i, PROMOTION));
      }
      if(rank == 6) {
        posTo = pos - 16;
        if(board->board[posTo] == '.')
          moves.push_back(getMove(pos, posTo, 0, NEUT));
      }
    }
    for(int i = 0; i < 2; i++) {
      rankTo = rank + pawnCapDirBlack[i].first, fileTo = file + pawnCapDirBlack[i].second, posTo = getSq(rankTo, fileTo);
      if(inTable(rankTo, fileTo)) {
        if((board->pieces[WHITE] >> posTo) & 1) {
          if(rankTo > 0) { /// capture
            moves.push_back(getMove(pos, posTo, 0, NEUT));
          } else { /// capture + promotion
            for(int i = 0; i < 4; i++)
              moves.push_back(getMove(pos, posTo, i, PROMOTION));
          }
        } else if(posTo == board->enPas) { /// maybe we have an en passant
          moves.push_back(getMove(pos, posTo, 0, ENPASSANT));
        }
      }
    }
    pawns ^= b;
  }
  return moves;
}

vector <uint16_t> genPawnMoves(Board *board) {
  vector <uint16_t> moves;

  bool color = board->turn;
  uint64_t capMask = board->pieces[1 ^ color], all = board->pieces[WHITE] | board->pieces[BLACK], quietMask = ~all;
  uint64_t b, b1, b2, b3;
  int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
  int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
  b1 = board->bb[getType(PAWN, color)] & ~rankMask[rank7];

  b2 = shift(color, NORTH, b1) & ~all; /// single push
  b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
  b2 &= quietMask;

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    moves.push_back(getMove(sqDir(color, SOUTH, sq), sq, 0, NEUT));
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b), sq2 = sqDir(color, SOUTH, sq);
    moves.push_back(getMove(sqDir(color, SOUTH, sq2), sq, 0, NEUT));
    b3 ^= b;
  }

  b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
  b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
  /// captures

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    moves.push_back(getMove(sqDir(color, SOUTHEAST, sq), sq, 0, NEUT));
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b);
    moves.push_back(getMove(sqDir(color, SOUTHWEST, sq), sq, 0, NEUT));
    b3 ^= b;
  }

  b1 = board->bb[getType(PAWN, color)] & rankMask[rank7];
  if(b1) {
    /// quiet promotions
    b2 = shift(color, NORTH, b1) & quietMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++)
        moves.push_back(getMove(sqDir(color, SOUTH, sq), sq, i, PROMOTION));
      b2 ^= b;
    }

    /// capture promotions

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++)
        moves.push_back(getMove(sqDir(color, SOUTHEAST, sq), sq, i, PROMOTION));
      b2 ^= b;
    }
    while(b3) {
      b = lsb(b3);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++)
        moves.push_back(getMove(sqDir(color, SOUTHWEST, sq), sq, i, PROMOTION));
      b3 ^= b;
    }
  }

  if(board->enPas >= 0) {
    b1 = board->bb[getType(PAWN, color)] & pawnAttacksMask[1 ^ color][board->enPas];

    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      moves.push_back(getMove(sq, board->enPas, 0, ENPASSANT));
      b1 ^= b;
    }
  }

  return moves;
}

vector <uint16_t> genPseudoLegalMoves(Board *board) {
  vector <uint16_t> moves;
  bool color = board->turn;

  moves = genPawnMoves(board);
  uint64_t pieces = board->bb[getType(KNIGHT, color)], notOwn = ~board->pieces[color], all = board->pieces[WHITE] | board->pieces[BLACK];
  while(pieces) {
    uint64_t b = lsb(pieces);
    int pos = Sq(b);
    uint64_t attacks = knightBBAttacks[pos] & notOwn;
    addMoves(moves, board, pos, attacks);
    pieces ^= b;
  }

  pieces = board->bb[getType(BISHOP, color)];
  while(pieces) {
    uint64_t b = lsb(pieces);
    int pos = Sq(b);
    uint64_t attacks = genAttacksBishop(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    pieces ^= b;
  }

  pieces = board->bb[getType(ROOK, color)];
  while(pieces) {
    uint64_t b = lsb(pieces);
    int pos = Sq(b);
    uint64_t attacks = genAttacksRook(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    pieces ^= b;
  }

  pieces = board->bb[getType(QUEEN, color)];
  while(pieces) {
    uint64_t b = lsb(pieces);
    int pos = Sq(b);
    uint64_t attacks = genAttacksQueen(all, pos) & notOwn;
    addMoves(moves, board, pos, attacks);
    pieces ^= b;
  }

  int sq = board->king(color);
  addMoves(moves, board, sq, kingBBAttacks[sq] & notOwn);

  join(moves, genCastles(board));
  return moves;
}*/

