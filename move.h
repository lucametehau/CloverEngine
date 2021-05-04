#pragma once
#include <cassert>
#include "board.h"
#include "defs.h"
#include "movegen.h"
#include "attacks.h"
#include "evaluate.h"

inline void makeMove(Board &board, uint16_t mv) { /// assuming move is at least pseudo-legal
  int posFrom = sqFrom(mv), posTo = sqTo(mv);
  int pieceFrom = board.board[posFrom], pieceTo = board.board[posTo];

  int ply = board.gamePly;
  board.history[ply].enPas = board.enPas;
  board.history[ply].halfMoves = board.halfMoves;
  board.history[ply].moveIndex = board.moveIndex;
  board.history[ply].key = board.key;
  board.history[ply].castleRights = board.castleRights;
  board.history[ply].captured = board.captured;

  board.key ^= (board.enPas >= 0 ? enPasKey[board.enPas] : 0);

  board.halfMoves++;

  if(piece_type(pieceFrom) == PAWN)
    board.halfMoves = 0;

  board.captured = 0;
  board.enPas = -1;

  switch(type(mv)) {
  case NEUT:
    board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
    board.bb[pieceFrom]       ^= (1ULL << posFrom) ^ (1ULL << posTo);

    board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo];

    /// i moved a castle rook

    if(pieceFrom == WR)
      board.castleRights &= castleRightsDelta[WHITE][posFrom];
    else if(pieceFrom == BR)
      board.castleRights &= castleRightsDelta[BLACK][posFrom];

    if(pieceTo) {
      board.halfMoves = 0;

      board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
      board.bb[pieceTo]             ^= (1ULL << posTo);
      board.key                     ^= hashKey[pieceTo][posTo];



      /// special case: captured rook might have been a castle rook

      if(pieceTo == WR)
        board.castleRights &= castleRightsDelta[WHITE][posTo];
      else if(pieceTo == BR)
        board.castleRights &= castleRightsDelta[BLACK][posTo];
    }

    board.board[posFrom] = 0;
    board.board[posTo]   = pieceFrom;
    board.captured       = pieceTo;

    /// double push

    if(piece_type(pieceFrom) == PAWN && (posFrom ^ posTo) == 16) {
      board.enPas = sqDir(board.turn, NORTH, posFrom);
      board.key ^= enPasKey[board.enPas];
    }

    /// i moved the king

    if(pieceFrom == WK)
      board.castleRights &= castleRightsDelta[WHITE][posFrom];
    else if(pieceFrom == BK)
      board.castleRights &= castleRightsDelta[BLACK][posFrom];

    break;
  case ENPASSANT:
    {
        int pos = sqDir(board.turn, SOUTH, posTo), pieceCap = getType(PAWN, 1 ^ board.turn);

        board.halfMoves = 0;

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[pieceFrom]       ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^ hashKey[pieceCap][pos];

        board.pieces[1 ^ board.turn] ^= (1ULL << pos);
        board.bb[pieceCap]            ^= (1ULL << pos);

        board.board[posFrom] = 0;
        board.board[posTo]   = pieceFrom;
        board.board[pos]     = 0;
        //board.captured       = 0; /// even thought i capture a pawn
    }

    break;
  case CASTLE:
    {
        int rFrom, rTo, rPiece = getType(ROOK, board.turn);

        if(posTo == mirror(board.turn, C1)) {
          rFrom = mirror(board.turn, A1);
          rTo   = mirror(board.turn, D1);
        } else {
          rFrom = mirror(board.turn, H1);
          rTo   = mirror(board.turn, F1);
        }

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^
                                      (1ULL << rFrom) ^ (1ULL << rTo);
        board.bb[pieceFrom]       ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[rPiece]          ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^
                      hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        board.board[posFrom] = board.board[rFrom] = 0;
        board.board[posTo]   = pieceFrom;
        board.board[rTo]     = rPiece;
        board.captured       = 0;

        if(pieceFrom == WK)
          board.castleRights &= castleRightsDelta[WHITE][posFrom];
        else if(pieceFrom == BK)
          board.castleRights &= castleRightsDelta[BLACK][posFrom];
    }

    break;
  default: /// promotion
    {
        int promPiece = getType(promoted(mv) + KNIGHT, board.turn);

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[pieceFrom]       ^= (1ULL << posFrom);
        board.bb[promPiece]       ^= (1ULL << posTo);

        if(pieceTo) {
          board.bb[pieceTo]             ^= (1ULL << posTo);
          board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
          board.key                     ^= hashKey[pieceTo][posTo];

          /// special case: captured rook might have been a castle rook

          if(pieceTo == WR)
            board.castleRights &= castleRightsDelta[WHITE][posTo];
          else if(pieceTo == BR)
            board.castleRights &= castleRightsDelta[BLACK][posTo];
        }

        board.board[posFrom] = 0;
        board.board[posTo] = promPiece;
        board.captured = pieceTo;

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[promPiece][posTo];
    }

    break;
  }

  /// dirty trick

  int temp = board.castleRights ^ board.history[ply].castleRights;

  //cout << temp << "\n";

  board.key ^= castleKeyModifier[temp];

  /*while(temp) {
    int b = Sq(temp & -temp);
    board.key ^= castleKey[b / 2][b % 2];
    temp ^= (1 << b);
  }*/

  board.turn ^= 1;
  board.ply++;
  board.gamePly++;
  board.key ^= 1;
  if(board.turn == WHITE)
    board.moveIndex++;
}

inline void undoMove(Board &board, uint16_t move) {

  board.turn ^= 1;
  board.ply--;
  board.gamePly--;

  int ply = board.gamePly;
  board.halfMoves = board.history[ply].halfMoves;
  board.enPas = board.history[ply].enPas;
  board.moveIndex = board.history[ply].moveIndex;
  board.key = board.history[ply].key;
  board.castleRights = board.history[ply].castleRights;

  int posFrom = sqFrom(move), posTo = sqTo(move), piece = board.board[posTo], pieceCap = board.captured;

  switch(type(move)) {
  case NEUT:
    board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
    board.bb[piece]          ^= (1ULL << posFrom) ^ (1ULL << posTo);

    board.board[posFrom] = piece;
    board.board[posTo]   = pieceCap;

    if(pieceCap) {
      board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
      board.bb[pieceCap]           ^= (1ULL << posTo);
    }
    break;
  case CASTLE:
    {
        int rFrom, rTo, rPiece = getType(ROOK, board.turn);

        if(posTo == mirror(board.turn, C1)) {
          rFrom = mirror(board.turn, A1);
          rTo   = mirror(board.turn, D1);
        } else {
          rFrom = mirror(board.turn, H1);
          rTo   = mirror(board.turn, F1);
        }

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        board.bb[piece]          ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[rPiece]         ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board.board[posTo] = board.board[rTo] = 0;
        board.board[posFrom] = piece;
        board.board[rFrom]   = rPiece;
    }
    break;
  case ENPASSANT:
    {
        int pos = sqDir(board.turn, SOUTH, posTo);

        pieceCap = getType(PAWN, 1 ^ board.turn);

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[piece]          ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.pieces[1 ^ board.turn] ^= (1ULL << pos);
        board.bb[pieceCap]           ^= (1ULL << pos);

        board.board[posTo]   = 0;
        board.board[posFrom] = piece;
        board.board[pos]     = pieceCap;
    }
    break;
  default: /// promotion
    {
        int promPiece = getType(promoted(move) + KNIGHT, board.turn);

        piece = getType(PAWN, board.turn);

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[piece]          ^= (1ULL << posFrom);
        board.bb[promPiece]      ^= (1ULL << posTo);

        board.board[posTo]   = pieceCap;
        board.board[posFrom] = piece;

        if(pieceCap) {
          board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
          board.bb[pieceCap]           ^= (1ULL << posTo);
        }
    }
    break;
  }

  board.captured = board.history[ply].captured;
}

inline void makeNullMove(Board &board) {
  int ply = board.gamePly;

  board.history[ply].enPas = board.enPas;
  board.history[ply].halfMoves = board.halfMoves;
  board.history[ply].moveIndex = board.moveIndex;
  board.history[ply].key = board.key;
  board.history[ply].castleRights = board.castleRights;
  board.history[ply].captured = board.captured;

  board.key ^= (board.enPas >= 0 ? enPasKey[board.enPas] : 0);

  board.captured = 0;
  board.enPas = -1;
  board.turn ^= 1;
  board.key ^= 1;
  board.ply++;
  board.gamePly++;
  board.halfMoves++;
  board.moveIndex++;
}

inline void undoNullMove(Board &board) {
  board.turn ^= 1;
  board.ply--;
  board.gamePly--;
  int ply = board.gamePly;

  board.halfMoves = board.history[ply].halfMoves;
  board.enPas = board.history[ply].enPas;
  board.moveIndex = board.history[ply].moveIndex;
  board.key = board.history[ply].key;
  board.castleRights = board.history[ply].castleRights;
  board.captured = board.history[ply].captured;
}

inline int genLegal(Board &board, uint16_t *moves) {
  //Board *temp = new Board(board);
  int nrMoves = 0;
  int color = board.turn, enemy = color ^ 1;
  int king = board.king(color), enemyKing = board.king(enemy);
  uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
  uint64_t b, b1, b2, b3;
  uint64_t attacked = 0, pinned = 0, checkers = 0; /// squares attacked by enemy / pinned pieces
  uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
  uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

  attacked |= pawnAttacks(board, enemy);

  pieces = board.bb[getType(KNIGHT, enemy)];
  while(pieces) {
    b = lsb(pieces);
    attacked |= knightBBAttacks[Sq(b)];
    pieces ^= b;
  }

  pieces = enemyDiagSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  pieces = enemyOrthSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  attacked |= kingBBAttacks[enemyKing];

  //cout << "First\n";
  //printBB(kingBBAttacks[king]);
  //printBB(~(us | attacked));

  b1 = kingBBAttacks[king] & ~(us | attacked);

  //printBB(us);
  //printBB(attacked);

  //printBB(b1);
  //printBB(~them);

  moves = addMoves(moves, nrMoves, king, b1);
  //moves = addCaps(moves, nrMoves, king, b1 & them);


  mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

  checkers = knightBBAttacks[king] & board.bb[getType(KNIGHT, enemy)];

  if(enemy == WHITE)
    checkers |= pawnAttacksMask[BLACK][king] & board.bb[WP];
  else
    checkers |= pawnAttacksMask[WHITE][king] & board.bb[BP];

  while(mask) {
    b = lsb(mask);
    int pos = Sq(b);
    b2 = us & between[pos][king];
    if(!b2)
      checkers ^= b;
    else if(!(b2 & (b2 - 1)))
      pinned ^= b2;
    mask ^= b;
  }

  //board.checkers = checkers;

  uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

  int cnt = smallPopCount(checkers);

  if(cnt == 2) { /// double check
    /// only king moves are legal
    return nrMoves;
  } else if(cnt == 1) { /// one check
    int sq = Sq(lsb(checkers));
    switch(board.piece_type_at(sq)) {
    case PAWN:
      /// make en passant to cancel the check
      if(checkers == (1ULL << (sqDir(enemy, NORTH, board.enPas)))) {
        mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[getType(PAWN, color)];
        while(mask) {
          b = lsb(mask);
          *(moves++) = (getMove(Sq(b), board.enPas, 0, ENPASSANT));
          nrMoves++;
          mask ^= b;
        }
      }
      /// intentional fall through
    case KNIGHT:
      capMask = checkers;
      quietMask = 0;
      break;
    default:
      capMask = checkers;
      quietMask = between[king][sq];
    }
  } else {
    capMask = them;
    quietMask = ~all;

    if(board.enPas >= 0) {
      int ep = board.enPas, sq2 = sqDir(color, SOUTH, ep);
      b2 = pawnAttacksMask[enemy][ep] & board.bb[getType(PAWN, color)];
      b1 = b2 & notPinned;
      while(b1) {
        b = lsb(b1);
        if(!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
           !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
          *(moves++) = (getMove(Sq(b), ep, 0, ENPASSANT));
          nrMoves++;
        }
        b1 ^= b;
      }
      b1 = b2 & pinned & Line[ep][king];
      if(b1) {
        *(moves++) = (getMove(Sq(b1), ep, 0, ENPASSANT));
        nrMoves++;
      }
    }

    if((board.castleRights >> (2 * color)) & 1) {
      if(!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
        *(moves++) = (getMove(king, king - 2, 0, CASTLE));
        nrMoves++;
      }
    }

    /// castle king side

    if((board.castleRights >> (2 * color + 1)) & 1) {
      if(!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
        *(moves++) = (getMove(king, king + 2, 0, CASTLE));
        nrMoves++;
      }
    }

    /// for pinned pieces they move on the same line with the king

    b1 = ~notPinned & board.diagSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksBishop(all, sq) & Line[king][sq];
      moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
      moves = addCaps(moves, nrMoves, sq, b2 & capMask);
      b1 ^= b;
    }
    b1 = ~notPinned & board.orthSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksRook(all, sq) & Line[king][sq];
      moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
      moves = addCaps(moves, nrMoves, sq, b2 & capMask);
      b1 ^= b;
    }

    /// pinned pawns

    b1 = ~notPinned & board.bb[getType(PAWN, color)];
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
      if(sq / 8 == rank7) { /// promotion captures
        b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
        while(b2) {
          b3 = lsb(b2);
          for(int j = 0; j < 4; j++) {
            *(moves++) = (getMove(sq, Sq(b3), j, PROMOTION));
            nrMoves++;
          }
          b2 ^= b3;
        }
      } else {
        b2 = pawnAttacksMask[color][sq] & them & Line[king][sq];
        moves = addMoves(moves, nrMoves, sq, b2);

        /// single pawn push
        b2 = (1ULL << (sqDir(color, NORTH, sq))) & emptySq & Line[king][sq];
        if(b2) {
          *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
          nrMoves++;
        }

        /// double pawn push
        b3 = (1ULL << (sqDir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
        if(b3 && Sq(b2) / 8 == rank3) {
          *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
          nrMoves++;
        }
      }
      b1 ^= b;
    }
  }

  /// not pinned pieces (excluding pawns)

  //cout << nrMoves << "\n";

  //printBB(quietMask);
  //printBB(knightBBAttacks[1]);

  uint64_t mobMask = capMask | quietMask;

  mask = board.bb[getType(KNIGHT, color)] & notPinned;
  while(mask) {
    uint64_t b = lsb(mask);
    int sq = Sq(b);
    //cout << "generating moves for knight on position " << sq << "\n";
    //printBB(knightBBAttacks[sq] & quietMask);
    moves = addMoves(moves, nrMoves, sq, knightBBAttacks[sq] & mobMask);
    //cout << nrMoves << "\n";
    //moves = addCaps(moves, nrMoves, sq, knightBBAttacks[sq] & capMask);
    mask ^= b;
  }

  mask = board.diagSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksBishop(all, sq);
    moves = addMoves(moves, nrMoves, sq, b2 & mobMask);
    //moves = addCaps(moves, nrMoves, sq, b2 & capMask);
    mask ^= b;
  }

  mask = board.orthSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksRook(all, sq);
    moves = addMoves(moves, nrMoves, sq, b2 & mobMask);
    //moves = addCaps(moves, nrMoves, sq, b2 & capMask);
    mask ^= b;
  }

  int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
  int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
  b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

  b2 = shift(color, NORTH, b1) & ~all; /// single push
  b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
  b2 &= quietMask;

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, 0, NEUT));
    nrMoves++;
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b), sq2 = sqDir(color, SOUTH, sq);
    *(moves++) = (getMove(sqDir(color, SOUTH, sq2), sq, 0, NEUT));
    nrMoves++;
    b3 ^= b;
  }

  b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
  b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
  /// captures

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, 0, NEUT));
    nrMoves++;
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, 0, NEUT));
    nrMoves++;
    b3 ^= b;
  }

  b1 = board.bb[getType(PAWN, color)] & notPinned & rankMask[rank7];
  if(b1) {
    /// quiet promotions
    b2 = shift(color, NORTH, b1) & quietMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b2 ^= b;
    }

    /// capture promotions

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b2 ^= b;
    }
    while(b3) {
      b = lsb(b3);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b3 ^= b;
    }
  }

  return nrMoves;
}







/// noisy moves generator


inline int genLegalNoisy(Board &board, uint16_t *moves) {
  //Board *temp = new Board(board);
  int nrMoves = 0;
  int color = board.turn, enemy = color ^ 1;
  int king = board.king(color), enemyKing = board.king(enemy);
  uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
  uint64_t b, b1, b2, b3;
  uint64_t attacked = 0, pinned = 0, checkers = 0; /// squares attacked by enemy / pinned pieces
  uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
  uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];

  attacked |= pawnAttacks(board, enemy);

  pieces = board.bb[getType(KNIGHT, enemy)];
  while(pieces) {
    b = lsb(pieces);
    attacked |= knightBBAttacks[Sq(b)];
    pieces ^= b;
  }

  pieces = enemyDiagSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  pieces = enemyOrthSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  attacked |= kingBBAttacks[enemyKing];

  moves = addCaps(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & them);

  /// castle queen side


  mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

  checkers = knightBBAttacks[king] & board.bb[getType(KNIGHT, enemy)];

  if(enemy == WHITE)
    checkers |= pawnAttacksMask[BLACK][king] & board.bb[WP];
  else
    checkers |= pawnAttacksMask[WHITE][king] & board.bb[BP];

  while(mask) {
    b = lsb(mask);
    int pos = Sq(b);
    uint64_t b2 = us & between[pos][king];
    if(!b2)
      checkers ^= b;
    else if(!(b2 & (b2 - 1)))
      pinned ^= b2;
    mask ^= b;
  }

  uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

  int cnt = smallPopCount(checkers);

  if(cnt == 2) { /// double check
    /// only king moves are legal
    return nrMoves;
  } else if(cnt == 1) { /// one check
    int sq = Sq(lsb(checkers));
    switch(board.piece_type_at(sq)) {
    case PAWN:
      /// make en passant to cancel the check
      if(checkers == (1ULL << (sqDir(enemy, NORTH, board.enPas)))) {
        mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[getType(PAWN, color)];
        while(mask) {
          b = lsb(mask);
          *(moves++) = (getMove(Sq(b), board.enPas, 0, ENPASSANT));
          nrMoves++;
          mask ^= b;
        }
      }
      /// intentional fall through
    case KNIGHT:
      /*mask = getAttackers(board, color, all, sq) & notPinned;
      while(mask) {
        b = lsb(mask);
        moves.push_back(getMove(Sq(b), sq, 0, NEUT));
        mask ^= b;
      }
      return moves;*/
      capMask = checkers;
      quietMask = 0;
      break;
    default:
      capMask = checkers;
      quietMask = between[king][sq];
    }
  } else {
    capMask = them;
    quietMask = ~all;

    if(board.enPas >= 0) {
      int ep = board.enPas, sq2 = sqDir(color, SOUTH, ep);
      b2 = pawnAttacksMask[enemy][ep] & board.bb[getType(PAWN, color)];
      b1 = b2 & notPinned;
      while(b1) {
        b = lsb(b1);
        if(!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
           !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
          *(moves++) = (getMove(Sq(b), ep, 0, ENPASSANT));
          nrMoves++;
        }
        b1 ^= b;
      }
      b1 = b2 & pinned & Line[ep][king];
      if(b1) {
        *(moves++) = (getMove(Sq(b1), ep, 0, ENPASSANT));
        nrMoves++;
      }
    }


    /// for pinned pieces they move on the same line with the king

    b1 = pinned & board.diagSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksBishop(all, sq) & Line[king][sq];
      moves = addCaps(moves, nrMoves, sq, b2 & capMask);
      b1 ^= b;
    }
    b1 = pinned & board.orthSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksRook(all, sq) & Line[king][sq];
      moves = addCaps(moves, nrMoves, sq, b2 & capMask);
      b1 ^= b;
    }

    /// pinned pawns

    b1 = ~notPinned & board.bb[getType(PAWN, color)];
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1);
      if(sq / 8 == rank7) { /// promotion captures
        b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
        while(b2) {
          b3 = lsb(b2);
          for(int j = 0; j < 4; j++) {
            *(moves++) = (getMove(sq, Sq(b3), j, PROMOTION));
            nrMoves++;
          }
          b2 ^= b3;
        }
      } else {
        b2 = pawnAttacksMask[color][sq] & them & Line[king][sq];
        moves = addCaps(moves, nrMoves, sq, b2);
      }
      b1 ^= b;
    }
  }

  /// not pinned pieces (excluding pawns)

  mask = board.bb[getType(KNIGHT, color)] & notPinned;
  while(mask) {
    uint64_t b = lsb(mask);
    int sq = Sq(b);
    moves = addCaps(moves, nrMoves, sq, knightBBAttacks[sq] & capMask);
    mask ^= b;
  }

  mask = board.diagSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksBishop(all, sq);
    moves = addCaps(moves, nrMoves, sq, b2 & capMask);
    mask ^= b;
  }

  mask = board.orthSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksRook(all, sq);
    moves = addCaps(moves, nrMoves, sq, b2 & capMask);
    mask ^= b;
  }

  int rank7 = (color == WHITE ? 6 : 1);
  int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
  b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

  b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
  b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
  /// captures

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, 0, NEUT));
    nrMoves++;
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, 0, NEUT));
    nrMoves++;
    b3 ^= b;
  }

  b1 = board.bb[getType(PAWN, color)] & notPinned & rankMask[rank7];
  if(b1) {
    /// quiet promotions
    b2 = shift(color, NORTH, b1) & quietMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b2 ^= b;
    }

    /// capture promotions

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    while(b2) {
      b = lsb(b2);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b2 ^= b;
    }
    while(b3) {
      b = lsb(b3);
      int sq = Sq(b);
      for(int i = 0; i < 4; i++) {
        *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, i, PROMOTION));
        nrMoves++;
      }
      b3 ^= b;
    }
  }

  return nrMoves;
}




/// generate quiet moves

inline int genLegalQuiets(Board &board, uint16_t *moves) {
  //Board *temp = new Board(board);
  int nrMoves = 0;
  int color = board.turn, enemy = color ^ 1;
  int king = board.king(color), enemyKing = board.king(enemy), pos = board.king(board.turn);
  uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
  uint64_t b, b1, b2, b3;
  uint64_t attacked = 0, pinned = 0, checkers = 0; /// squares attacked by enemy / pinned pieces
  uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
  uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

  attacked |= pawnAttacks(board, enemy);

  pieces = board.bb[getType(KNIGHT, enemy)];
  while(pieces) {
    b = lsb(pieces);
    attacked |= knightBBAttacks[Sq(b)];
    pieces ^= b;
  }

  pieces = enemyDiagSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  pieces = enemyOrthSliders;
  while(pieces) {
    b = lsb(pieces);
    attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
    pieces ^= b;
  }

  attacked |= kingBBAttacks[enemyKing];

  moves = addQuiets(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & ~them);


  mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

  checkers = knightBBAttacks[king] & board.bb[getType(KNIGHT, enemy)];

  if(enemy == WHITE)
    checkers |= pawnAttacksMask[BLACK][king] & board.bb[WP];
  else
    checkers |= pawnAttacksMask[WHITE][king] & board.bb[BP];

  while(mask) {
    b = lsb(mask);
    int pos = Sq(b);
    uint64_t b2 = us & between[pos][king];
    if(!b2)
      checkers ^= b;
    else if(!(b2 & (b2 - 1)))
      pinned ^= b2;
    mask ^= b;
  }

  uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

  int cnt = smallPopCount(checkers);

  if(cnt == 2) { /// double check
    /// only king moves are legal
    return nrMoves;
  } else if(cnt == 1) { /// one check
    int sq = Sq(lsb(checkers));
    switch(board.piece_type_at(sq)) {
    case PAWN:

    case KNIGHT:
      return nrMoves;
    default:
      capMask = checkers;
      quietMask = between[king][sq];
    }
  } else {
    capMask = them;
    quietMask = ~all;

    if(board.castleRights & (1 << (2 * color))) {
      if(!(attacked & (7ULL << (pos - 2))) && !(all & (7ULL << (pos - 3)))) {
        *(moves++) = (getMove(pos, pos - 2, 0, CASTLE));
        nrMoves++;
      }
    }

    /// castle king side

    if(board.castleRights & (1 << (2 * color + 1))) {
      if(!(attacked & (7ULL << pos)) && !(all & (3ULL << (pos + 1)))) {
        *(moves++) = (getMove(pos, pos + 2, 0, CASTLE));
        nrMoves++;
      }
    }

    /// for pinned pieces they move on the same line with the king

    b1 = ~notPinned & board.diagSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksBishop(all, sq) & Line[king][sq];
      moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
      b1 ^= b;
    }
    b1 = ~notPinned & board.orthSliders(color);
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b);
      b2 = genAttacksRook(all, sq) & Line[king][sq];
      moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
      b1 ^= b;
    }

    /// pinned pawns

    b1 = ~notPinned & board.bb[getType(PAWN, color)];
    while(b1) {
      b = lsb(b1);
      int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
      if(sq / 8 == rank7) { /// promotion captures

      } else {

        /// single pawn push
        b2 = (1ULL << (sqDir(color, NORTH, sq))) & emptySq & Line[king][sq];
        if(b2) {
          *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
          nrMoves++;
        }

        /// double pawn push
        b3 = (1ULL << (sqDir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
        if(b3 && Sq(b2) / 8 == rank3) {
          *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
          nrMoves++;
        }
      }
      b1 ^= b;
    }
  }

  /// not pinned pieces (excluding pawns)

  mask = board.bb[getType(KNIGHT, color)] & notPinned;
  while(mask) {
    uint64_t b = lsb(mask);
    int sq = Sq(b);
    moves = addQuiets(moves, nrMoves, sq, knightBBAttacks[sq] & quietMask);
    mask ^= b;
  }

  mask = board.diagSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksBishop(all, sq);
    moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
    mask ^= b;
  }

  mask = board.orthSliders(color) & notPinned;
  while(mask) {
    b = lsb(mask);
    int sq = Sq(b);
    b2 = genAttacksRook(all, sq);
    moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
    mask ^= b;
  }

  int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);

  b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

  b2 = shift(color, NORTH, b1) & ~all; /// single push
  b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
  b2 &= quietMask;

  while(b2) {
    b = lsb(b2);
    int sq = Sq(b);
    *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, 0, NEUT));
    nrMoves++;
    b2 ^= b;
  }
  while(b3) {
    b = lsb(b3);
    int sq = Sq(b), sq2 = sqDir(color, SOUTH, sq);
    *(moves++) = (getMove(sqDir(color, SOUTH, sq2), sq, 0, NEUT));
    nrMoves++;
    b3 ^= b;
  }

  return nrMoves;
}

inline bool isNoisyMove(Board &board, uint16_t move) {
  if(type(move) && type(move) != CASTLE)
    return 1;

  return board.isCapture(move);
}


inline bool isRepetition(Board &board, int ply) {
  int cnt = 1;
  for(int i = board.gamePly - 2; i >= 0; i -= 2) {
    if(i < board.gamePly - board.halfMoves)
      break;
    if(board.history[i].key == board.key) {
      cnt++;
      if(i > board.gamePly - ply)
        return 1;
      if(cnt == 3)
        return 1;
    }
  }
  return 0;
}

bool isPseudoLegalMove(Board &board, uint16_t move) {
  if(!move)
    return 0;

  int from = sqFrom(move), to = sqTo(move), t = type(move), pt = board.piece_type_at(from), color = board.turn;
  uint64_t own = board.pieces[color], enemy = board.pieces[1 ^ color], occ = board.pieces[WHITE] | board.pieces[BLACK];

  if(color != board.board[from] / 7) /// different color
    return 0;

  if(!board.board[from]) /// there isn't a piece
    return 0;

  if(own & (1ULL << to)) /// can't move piece on the same square as one of our pieces
    return 0;

  /// check for normal moves

  if(pt == KNIGHT)
    return t == NEUT && (knightBBAttacks[from] & (1ULL << to));

  if(pt == BISHOP)
    return t == NEUT && (genAttacksBishop(occ, from) & (1ULL << to));

  if(pt == ROOK)
    return t == NEUT && (genAttacksRook(occ, from) & (1ULL << to));

  if(pt == QUEEN)
    return t == NEUT && (genAttacksQueen(occ, from) & (1ULL << to));

  /// pawn moves

  if(pt == PAWN) {

    uint64_t att = pawnAttacksMask[color][from];

    /// enpassant

    if(t == ENPASSANT)
      return to == board.enPas && (att & (1ULL << to));

    uint64_t push = shift(color, NORTH, (1ULL << from)) & ~occ;

    /// promotion

    if(t == PROMOTION)
      return (to / 8 == 0 || to / 8 == 7) && (((att & enemy) | push) & (1ULL << to));

    /// add double push to mask

    if(from / 8 == 1 || from / 8 == 6)
      push |= shift(color, NORTH, push) & ~occ;

    return (to / 8 && to / 8 != 7) && t == NEUT && (((att & enemy) | push) & (1ULL << to));
  }

  /// king moves (normal or castle)

  if(t == NEUT)
    return kingBBAttacks[from] & (1ULL << to);

  int side = (to % 8 == 6); /// queen side or king side

  if(board.castleRights & (1 << (2 * color + side))) { /// can i castle
    if(isSqAttacked(board, color ^ 1, from) || isSqAttacked(board, color ^ 1, to))
      return 0;

    /// castle queen side

    if(!side) {
      return !(occ & (7ULL << (from - 3))) && !isSqAttacked(board, color ^ 1, Sq(between[from][to]));
    } else {
      return !(occ & (3ULL << (from + 1))) && !isSqAttacked(board, color ^ 1, Sq(between[from][to]));
    }
  }

  return 0;

}

bool isLegalMoveSlow(Board &board, int move) {
  uint16_t moves[256];
  int nrMoves = 0;

  nrMoves = genLegal(board, moves);

  for(int i = 0; i < nrMoves; i++) {
    if(move == moves[i])
      return 1;
  }

  return 0;
}

bool isLegalMove(Board &board, int move) {
  if(!isPseudoLegalMove(board, move)) {
    return 0;
  }

  if(type(move) == CASTLE) {
    return 1;
  }

  makeMove(board, move);
  bool legal = !isSqAttacked(board, board.turn, board.king(board.turn ^ 1));
  undoMove(board, move);

  //std::cout << legal << "\n";

  //assert(legal == isLegalMoveSlow(board, move));

  return legal;
}

inline uint16_t ParseMove(Board &board, std::string movestr) {

  if(movestr[1] > '8' || movestr[1] < '1') return NULLMOVE;
  if(movestr[3] > '8' || movestr[3] < '1') return NULLMOVE;
  if(movestr[0] > 'h' || movestr[0] < 'a') return NULLMOVE;
  if(movestr[2] > 'h' || movestr[2] < 'a') return NULLMOVE;

  int from = getSq(movestr[1] - '1', movestr[0] - 'a');
  int to = getSq(movestr[3] - '1', movestr[2] - 'a');

  uint16_t moves[256];
  int nrMoves = genLegal(board, moves);

	for(int i = 0; i < nrMoves; i++) {
    int move = moves[i];
		if(sqFrom(move) == from && sqTo(move) == to) {
      int PromPce = promoted(move) + KNIGHT;
      if(type(move) == PROMOTION) {
        if(PromPce == ROOK && movestr[4] == 'r') {
          return move;
        } else if(PromPce == BISHOP && movestr[4] == 'b') {
          return move;
        } else if(PromPce == QUEEN && movestr[4] == 'q') {
          return move;
        } else if(PromPce == KNIGHT && movestr[4] == 'n') {
          return move;
        }
        continue;
      }
      return move;
		}
  }

  return NULLMOVE;
}

