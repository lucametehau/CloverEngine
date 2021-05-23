#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"
#include "material.h"

struct EvalTools { /// to avoid data races, kinda dirty tho
  uint64_t kingRing[2], kingSquare[2], pawnShield[2], defendedByPawn[2], pawns[2], allPawns;
  uint64_t attackedBy[2], attackedBy2[2], attackedByPiece[2][7];

  int phase;

  int kingAttackersCount[2], kingAttackersWeight[2];
  int kingDanger[2];
  int score[2][2];

  void init() {
    phase = 0;

    memset(score, 0, sizeof(score));
    //memset(kingRing, 0, sizeof(kingRing));
    //memset(kingSquare, 0, sizeof(kingSquare));
    //memset(pawnShield, 0, sizeof(pawnShield));
    //memset(defendedByPawn, 0, sizeof(defendedByPawn));
    //memset(pawns, 0, sizeof(pawns));
    memset(attackedByPiece, 0, sizeof(attackedByPiece));
    memset(kingAttackersCount, 0, sizeof(kingAttackersCount));
    memset(kingAttackersWeight, 0, sizeof(kingAttackersWeight));
    memset(kingDanger, 0, sizeof(kingDanger));
    allPawns = 0;
  }
};

struct EvalTraceEntry {
  uint16_t mgind;
  //uint8_t wval, bval;
  int8_t dval, deltaind;
};

class EvalTrace { /// when tuning, we can keep the count of every evaluation term used for speed up
public:
  int phase;

  uint8_t doubledPawnsPenalty[2][2];
  uint8_t isolatedPenalty[2][2];
  uint8_t backwardPenalty[2][2];

  uint8_t mat[2][2][7];

  uint8_t passedBonus[2][2][7];
  uint8_t connectedBonus[2][2][7];

  uint8_t safeCheck[2][2][6];

  uint8_t pawnShield[2][2][4];

  uint8_t outpostBonus[2][2][4];
  uint8_t outpostHoleBonus[2][2][4];

  uint8_t rookOpenFile[2][2];
  uint8_t rookSemiOpenFile[2][2];

  uint8_t bishopPair[2][2];
  uint8_t longDiagonalBishop[2][2];

  uint8_t trappedRook[2][2];

  uint8_t mobilityBonus[2][7][2][30];

  uint8_t bonusTable[2][7][2][64];

  uint8_t scale;

  uint16_t kingDanger[2];

  uint16_t others[2];

  EvalTraceEntry entries[1000];
  int nrEntries;

  void add(int mgind, int egind, bool color, int val) {
    if(!val && (color == BLACK || !entries[nrEntries].dval))
      return;
    entries[nrEntries].mgind = mgind;
    entries[nrEntries].deltaind = egind - mgind;
    if(color == WHITE) {
      entries[nrEntries].dval += val;
      //entries[nrEntries].dval = entries[nrEntries].wval - entries[nrEntries].bval;
      nrEntries++;
    } else {
      entries[nrEntries].dval = -val;
    }
  }

} trace;

class TunePos {
public:
  EvalTraceEntry *entries;

  uint16_t nrEntries;

  uint16_t others[2], kingDanger[2];

  uint8_t phase;
  uint8_t scale;

  bool turn;
};

const int TEMPO = 20;

int doubledPawnsPenalty[2] = {0, -25, };
int isolatedPenalty[2] = {-11, -1, };
int backwardPenalty[2] = {-10, -13, };

const int phaseVal[] = {0, 0, 1, 1, 2, 4};
const int maxWeight = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];
int passedBonus[2][7] = {
  {0, -1, -10, -1, 20, 27, 96},
  {0, 1, 13, 41, 71, 126, 146},
};
int connectedBonus[2][7] = {
  {0, 1, 2, 4, 9, 25, 80},
  {0, 1, 3, 3, 9, 22, 28},
};
int kingAttackWeight[] = {0, 0, 2, 2, 3, 5};
int SafetyTable[100] = {
  0, 0, 1, 2, 3, 5, 7, 9, 12, 15,
  18, 22, 26, 30, 35, 39, 44, 50, 56, 62,
  68, 75, 82, 85, 89, 97, 105, 113, 122, 131,
  140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
  260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
  377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
  494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
};
int safeCheck[2][7] = {
  {0, 0, -91, -23, -110, -33},
  {0, 0, 18, -29, 15, -19},
};
int pawnShield[2][4] = {
  {0, 6, 37, 65},
  {0, 15, 5, -4},
};
int outpostBonus[2][4] = {
  {0, 0, 33, 32},
  {0, 0, 9, 9},
};
int outpostHoleBonus[2][4] = {
  {0, 0, 20, 26},
  {0, 0, 22, 8},
};
int rookOpenFile[2] = {41, 3, };
int rookSemiOpenFile[2] = {16, 7, };
int bishopPair[2] = {32, 66, };
int longDiagonalBishop[2] = {21, 15, };
int trappedRook[2] = {-19, -21, };
int mobilityBonus[7][2][30] = {
    {},
    {},
    {
        {-60, -5, 14, 24, 37, 42, 50, 61, 73, },
        {-31, -15, 24, 36, 41, 51, 53, 50, 42, },
    },
    {
        {-60, -15, 0, 14, 26, 34, 40, 44, 46, 45, 49, 57, 58, 67, },
        {-54, -47, -12, 11, 20, 31, 37, 41, 46, 49, 44, 38, 45, 35, },
    },
    {
        {-70, -21, -5, 3, 6, 6, 8, 11, 15, 16, 17, 20, 20, 22, 18, },
        {-87, -39, 3, 21, 27, 35, 42, 49, 54, 62, 68, 70, 72, 68, 67, },
    },
    {
        {-216, -146, -77, -10, 4, 15, 28, 31, 37, 39, 42, 45, 46, 48, 50, 52, 49, 50, 42, 45, 49, 58, 55, 58, 64, 120, 63, 126, },
        {-153, -93, -10, -81, -35, 3, -33, 5, 12, 27, 36, 47, 66, 69, 78, 88, 99, 104, 112, 119, 114, 113, 115, 113, 115, 73, 94, 142, },
    }
};



void matEval(Board &board, int color, EvalTools &tools) {
  uint64_t pieces = board.pieces[color] ^ board.bb[getType(KING, color)];

  while(pieces) {
    uint64_t b = lsb(pieces);
    int sq = Sq(b), sq2 = mirror(1 - color, sq), piece = board.piece_type_at(sq);

    /// piece values and psqt

    tools.score[color][MG] += mat[MG][piece];
    tools.score[color][EG] += mat[EG][piece];

    if(TUNE)
      trace.mat[color][MG][piece]++, trace.mat[color][EG][piece]++;

    tools.score[color][MG] += bonusTable[piece][MG][sq2];
    tools.score[color][EG] += bonusTable[piece][EG][sq2];

    if(TUNE)
      trace.bonusTable[color][piece][MG][sq2]++, trace.bonusTable[color][piece][EG][sq2]++;

    pieces &= pieces - 1;
  }
}

bool isOpenFile(int file, EvalTools &tools) {
  return !(tools.allPawns & fileMask[file]);
}

bool isHalfOpenFile(int color, int file, EvalTools &tools) { /// is file half-open for color
  return !(tools.pawns[color] & fileMask[file]);
}

void rookEval(Board &board, int color, EvalTools &tools) {
  int king = getType(KING, color);

  /// evaluation pattern: apply a penaly if rook is trapped by king

  if(color == WHITE) {
    if(board.board[A1] == WR && (board.bb[king] & between[A1][E1])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    } else if(board.board[H1] == WR && (board.bb[king] & between[H1][E1])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    }
  } else {
    if(board.board[A8] == BR && (board.bb[king] & between[A8][E8])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    } else if(board.board[H8] == BR && (board.bb[king] & between[H8][E8])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    }
  }
}

void pawnEval(Board &board, int color, EvalTools &tools, int pawnScore[], uint64_t &passedPawns) {
  uint64_t pieces = tools.pawns[color], passers = 0;

  while(pieces) {
    uint64_t b = lsb(pieces);
    int sq = Sq(b), rank = sq >> 3, frontSq = sqDir(color, NORTH, sq);

    uint64_t neigh = neighFilesMask[sq] & tools.pawns[color], phalanx = neigh & rankMask[rank], defenders = neigh & shift(color, SOUTH, rankMask[rank]);
    uint64_t opposed = tools.pawns[1 ^ color] & shift(color, NORTH, b);

    /// tools.phase += phaseVal[PAWN];

    /// check for passed pawn

    if(color == WHITE) {
      if(!(neighFileUpMask[sq] & tools.pawns[BLACK])) { /// no enemy pawns on the neighbour files on all ranks in front of it
        /*if(!(fileUpMask[sq] & tools.pawns[WHITE])) /// in case of double pawns, the one in the front is passed, but the other is not*/
          passers |= b;
      }

      if(neigh && !((neighFileDownMask[frontSq] ^ b) & tools.pawns[WHITE]) && ((1ULL << frontSq) & tools.defendedByPawn[BLACK])) {
        pawnScore[MG] += backwardPenalty[MG];
        pawnScore[EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileUpMask[sq] & tools.pawns[WHITE]) {
        pawnScore[MG] += doubledPawnsPenalty[MG];
        pawnScore[EG] += doubledPawnsPenalty[EG];

        if(TUNE)
          trace.doubledPawnsPenalty[color][MG]++, trace.doubledPawnsPenalty[color][EG]++;
      }
    } else {
      if(!(neighFileDownMask[sq] & tools.pawns[WHITE])) { /// no enemy pawns on the neighbour files on all ranks in front of it
        /*if(!(fileDownMask[sq] & tools.pawns[BLACK])) /// in case of double pawns, the one in the front is passed, but the other is not*/
          passers |= b;
      }

      if(neigh && !((neighFileUpMask[frontSq] ^ b) & tools.pawns[BLACK]) && ((1ULL << frontSq) & tools.defendedByPawn[WHITE])) {
        pawnScore[MG] += backwardPenalty[MG];
        pawnScore[EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileDownMask[sq] & tools.pawns[BLACK]) {
        pawnScore[MG] += doubledPawnsPenalty[MG];
        pawnScore[EG] += doubledPawnsPenalty[EG];

        if(TUNE)
          trace.doubledPawnsPenalty[color][MG]++, trace.doubledPawnsPenalty[color][EG]++;
      }
    }
    /// check for isolated pawn / connected pawns

    if(phalanx | defenders) {
      /// give bonus if pawn is connected, increase it if pawn makes a phalanx, decrease it if pawn is blocked

      int f = (phalanx ? 4 : 2) / (opposed ? 2 : 1);

      if(TUNE) {
        trace.connectedBonus[color][MG][(color == WHITE ? rank : 7 - rank)] += f;
        trace.connectedBonus[color][EG][(color == WHITE ? rank : 7 - rank)] += f;
        //std::cout << trace.connectedBonus[color][EG][(color == WHITE ? rank : 7 - rank)] << " " << phalanx << " " << opposed << "\n";
        trace.others[color] += 10 * count(defenders);
      }

      //std::cout << f << " " << connectedBonus[MG][(color == WHITE ? rank : 7 - rank)] << "\n";

      pawnScore[MG] += f * connectedBonus[MG][(color == WHITE ? rank : 7 - rank)] +
                                10 * count(defenders);
      pawnScore[EG] += f * connectedBonus[EG][(color == WHITE ? rank : 7 - rank)] +
                                10 * count(defenders);
    }

    /// no supporting pawns
    if(!neigh) {
      pawnScore[MG] += isolatedPenalty[MG];
      pawnScore[EG] += isolatedPenalty[EG];

      if(TUNE)
        trace.isolatedPenalty[color][MG]++, trace.isolatedPenalty[color][EG]++;
    }


    pieces ^= b;
  }


  passedPawns = passers;
}

void passersEval(Board &board, int color, EvalTools &tools, uint64_t passers) {
  /// blocked passers aren't given any passed bonus - to change?
  uint64_t blockedPassers = passers & shift(color, SOUTH, board.pieces[color ^ 1]);
  passers ^= blockedPassers;
  while(passers) {
    uint64_t b = lsb(passers);
    int rank = (color == WHITE ? Sq(b) / 8 : 7 - Sq(b) / 8);
    tools.score[color][MG] += passedBonus[MG][rank];
    tools.score[color][EG] += passedBonus[EG][rank];

    if(TUNE)
      trace.passedBonus[color][MG][rank]++, trace.passedBonus[color][EG][rank]++;
    passers ^= b;
  }
}

void pieceEval(Board &board, int color, EvalTools &tools) {
  bool enemy = 1 ^ color;
  uint64_t b, att, mob, pieces = 0, mobilityArea;
  uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];
  uint64_t outpostRanks = (color == WHITE ? rankMask[3] | rankMask[4] | rankMask[5] : rankMask[2] | rankMask[3] | rankMask[4]);

  /// mobility area is all squares without our king, squares attacked by enemy pawns and our blocked pawns

  mobilityArea = ~(board.bb[getType(KING, color)] | tools.defendedByPawn[enemy] | (tools.pawns[color] & shift(color, SOUTH, all)));

  pieces = board.bb[getType(KNIGHT, color)];

  while(pieces) {
    b = lsb(pieces);
    int sq = Sq(b), file = sq % 8;
    att = knightBBAttacks[sq];
    mob = att & mobilityArea;

    int cnt = count(mob);

    tools.score[color][MG] += mobilityBonus[KNIGHT][MG][cnt];
    tools.score[color][EG] += mobilityBonus[KNIGHT][EG][cnt];

    if(TUNE)
      trace.mobilityBonus[color][KNIGHT][MG][cnt]++, trace.mobilityBonus[color][KNIGHT][EG][cnt]++;

    tools.phase += phaseVal[KNIGHT];

    if(b & (outpostRanks | tools.defendedByPawn[color])) {
      /// assign bonus if on outpost or outpost hole (black: f5, d5, e6 then e5 is a hole)
      if((color == WHITE && !(tools.pawns[enemy] & (neighFileUpMask[sq] ^ fileUpMask[sq]))) || (color == BLACK && !(tools.pawns[enemy] & (neighFileDownMask[sq] ^ fileDownMask[sq])))) {
        if(isHalfOpenFile(color, file, tools)) {
          tools.score[color][MG] += outpostBonus[MG][KNIGHT];
          tools.score[color][EG] += outpostBonus[EG][KNIGHT];

          if(TUNE)
            trace.outpostBonus[color][MG][KNIGHT]++, trace.outpostBonus[color][EG][KNIGHT]++;
        } else {
          tools.score[color][MG] += outpostHoleBonus[MG][KNIGHT];
          tools.score[color][EG] += outpostHoleBonus[EG][KNIGHT];

          if(TUNE)
            trace.outpostHoleBonus[color][MG][KNIGHT]++, trace.outpostHoleBonus[color][EG][KNIGHT]++;
        }
      }
    }

    tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][KNIGHT] |= att;

    /// update king safety terms
    if(att & tools.kingRing[enemy]) {
      tools.kingAttackersWeight[color] += kingAttackWeight[KNIGHT] * count(att & tools.kingRing[enemy]);
      tools.kingAttackersCount[color]++;
    }

    pieces ^= b;
  }

  pieces = board.bb[getType(BISHOP, color)];

  /// assign bonus for bishop pair

  if(smallPopCount(pieces) >= 2) {
    tools.score[color][MG] += bishopPair[MG];
    tools.score[color][EG] += bishopPair[EG];

    if(TUNE)
      trace.bishopPair[color][MG]++, trace.bishopPair[color][EG]++;
  }

  while(pieces) {
    b = lsb(pieces);
    int sq = Sq(b), file = sq % 8;
    att = genAttacksBishop(all, sq);
    mob = att & mobilityArea;

    int cnt = count(mob);

    tools.score[color][MG] += mobilityBonus[BISHOP][MG][cnt];
    tools.score[color][EG] += mobilityBonus[BISHOP][EG][cnt];

    if(TUNE)
      trace.mobilityBonus[color][BISHOP][MG][cnt]++, trace.mobilityBonus[color][BISHOP][EG][cnt]++;

    tools.phase += phaseVal[BISHOP];

    if(b & (outpostRanks | tools.defendedByPawn[color])) {
      /// same as knight outposts
      if((color == WHITE && !(tools.pawns[enemy] & (neighFileUpMask[sq] ^ fileUpMask[sq]))) || (color == BLACK && !(tools.pawns[enemy] & (neighFileDownMask[sq] ^ fileDownMask[sq])))) {
        if(isHalfOpenFile(color, file, tools)) {
          tools.score[color][MG] += outpostBonus[MG][BISHOP];
          tools.score[color][EG] += outpostBonus[EG][BISHOP];

          if(TUNE)
            trace.outpostBonus[color][MG][BISHOP]++, trace.outpostBonus[color][EG][BISHOP]++;
        } else {
          tools.score[color][MG] += outpostHoleBonus[MG][BISHOP];
          tools.score[color][EG] += outpostHoleBonus[EG][BISHOP];

          if(TUNE)
            trace.outpostHoleBonus[color][MG][BISHOP]++, trace.outpostHoleBonus[color][EG][BISHOP]++;
        }
      }
    }

    /// assign bonus if bishop is on a long diagonal (controls 2 squares in the center)

    if(smallPopCount(att & CENTER) >= 2 && ((1ULL << sq) & (LONG_DIAGONALS & ~CENTER))) {
      tools.score[color][MG] += longDiagonalBishop[MG];
      tools.score[color][EG] += longDiagonalBishop[EG];

      if(TUNE)
        trace.longDiagonalBishop[color][MG]++, trace.longDiagonalBishop[color][EG]++;
    }

    tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][BISHOP] |= att;

    /// update king safety terms
    if(att & tools.kingRing[enemy]) {
      tools.kingAttackersWeight[color] += kingAttackWeight[BISHOP] * count(att & tools.kingRing[enemy]);
      tools.kingAttackersCount[color]++;
    }

    pieces ^= b;
  }

  pieces = board.bb[getType(ROOK, color)];
  while(pieces) {
    b = lsb(pieces);
    int sq = Sq(b), file = sq % 8;
    att = genAttacksRook(all, sq);
    mob = att & mobilityArea;

    int cnt = count(mob);

    tools.score[color][MG] += mobilityBonus[ROOK][MG][cnt];
    tools.score[color][EG] += mobilityBonus[ROOK][EG][cnt];

    if(TUNE)
      trace.mobilityBonus[color][ROOK][MG][cnt]++, trace.mobilityBonus[color][ROOK][EG][cnt]++;

    /// assign bonus for rooks on open/semi-open files

    if(isOpenFile(file, tools)) {
      tools.score[color][MG] += rookOpenFile[MG];
      tools.score[color][EG] += rookOpenFile[EG];

      if(TUNE)
        trace.rookOpenFile[color][MG]++, trace.rookOpenFile[color][EG]++;
    } else if(isHalfOpenFile(color, file, tools)) {
      tools.score[color][MG] += rookSemiOpenFile[MG];
      tools.score[color][EG] += rookSemiOpenFile[EG];

      if(TUNE)
        trace.rookSemiOpenFile[color][MG]++, trace.rookSemiOpenFile[color][EG]++;
    }

    tools.phase += phaseVal[ROOK];

    tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][ROOK] |= att;

    /// update king safety terms
    if(att & tools.kingRing[enemy]) {
      tools.kingAttackersWeight[color] += kingAttackWeight[ROOK] * count(att & tools.kingRing[enemy]);
      tools.kingAttackersCount[color]++;
    }

    pieces ^= b;
  }

  pieces = board.bb[getType(QUEEN, color)];
  while(pieces) {
    b = lsb(pieces);
    int sq = Sq(b);
    att = genAttacksBishop(all ^ board.bb[getType(BISHOP, color)], sq) |
          genAttacksRook(all ^ board.bb[getType(ROOK, color)], sq);
    mob = att & mobilityArea;

    int cnt = count(mob);

    tools.score[color][MG] += mobilityBonus[QUEEN][MG][cnt];
    tools.score[color][EG] += mobilityBonus[QUEEN][EG][cnt];

    if(TUNE)
      trace.mobilityBonus[color][QUEEN][MG][cnt]++, trace.mobilityBonus[color][QUEEN][EG][cnt]++;

    tools.phase += phaseVal[QUEEN];

    tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][QUEEN] |= att;

    /// update king safety terms
    if(att & tools.kingRing[enemy]) {
      tools.kingAttackersWeight[color] += kingAttackWeight[QUEEN] * count(att & tools.kingRing[enemy]);
      tools.kingAttackersCount[color]++;
    }

    pieces ^= b;
  }

  //cout << "piece score for " << (color == WHITE ? "white " : "black ") << " : " << score << "\n";
}

void kingEval(Board &board, int color, EvalTools &tools) {
  int king = board.king(color), pos = mirror(1 ^ color, king);
  //int rank = king / 8, file = king % 8;
  //uint64_t camp = (color == WHITE ? ALL ^ rankMask[5] ^ rankMask[6] ^ rankMask[7] : ALL ^ rankMask[0] ^ rankMask[1] ^ rankMask[2]), weak;
  bool enemy = 1 ^ color;

  /// king psqt

  if(TUNE) {
    tools.score[color][MG] += bonusTable[KING][MG][pos];
    tools.score[color][EG] += bonusTable[KING][EG][pos];
  }

  if(TUNE)
    trace.bonusTable[color][KING][MG][pos]++, trace.bonusTable[color][KING][EG][pos]++;

  uint64_t shield = tools.pawnShield[color] & tools.pawns[color];
  int shieldCount = std::min(3, count(shield));

  /// as on cpw (https://www.chessprogramming.org/King_Safety)

  if(tools.kingAttackersCount[enemy] >= 2) {
    int weight = std::min(99, tools.kingAttackersWeight[enemy]);

    //cout << weight << "\n";

    if(!board.bb[getType(QUEEN, enemy)])
      weight /= 2;

    if(!board.bb[getType(ROOK, enemy)])
      weight *= 0.75;

    tools.kingDanger[color] = SafetyTable[weight];

    /// apply penalty if enemy can give safe checks

    /// weak squares are those attacked by the enemy, undefended or defended once by our king or queen

    uint64_t weak = tools.attackedBy[enemy] & ~tools.attackedBy2[color] &
                    (~tools.attackedBy[color] | tools.attackedByPiece[color][QUEEN] | tools.attackedByPiece[color][KING]);

    /// safe squares are those which are not attacked by us or weak squares attacked twice by the enemy

    uint64_t safe = ~board.pieces[enemy] & (~tools.attackedBy[color] | (weak & tools.attackedBy2[enemy])),
             occ = board.pieces[WHITE] | board.pieces[BLACK];
    int knightChecksCount = count(knightBBAttacks[king] & tools.attackedByPiece[enemy][KNIGHT] & safe);
    int bishopChecksCount = count(genAttacksBishop(occ, king) & tools.attackedByPiece[enemy][BISHOP] & safe);
    int rookChecksCount = count(genAttacksRook(occ, king) & tools.attackedByPiece[enemy][ROOK] & safe);
    int queenChecksCount = count(genAttacksQueen(occ, king) & tools.attackedByPiece[enemy][QUEEN] & safe);

    tools.score[color][MG] += safeCheck[MG][KNIGHT] * knightChecksCount;
    tools.score[color][EG] += safeCheck[EG][KNIGHT] * knightChecksCount;

    tools.score[color][MG] += safeCheck[MG][BISHOP] * bishopChecksCount;
    tools.score[color][EG] += safeCheck[EG][BISHOP] * bishopChecksCount;

    tools.score[color][MG] += safeCheck[MG][ROOK] * rookChecksCount;
    tools.score[color][EG] += safeCheck[EG][ROOK] * rookChecksCount;

    tools.score[color][MG] += safeCheck[MG][QUEEN] * queenChecksCount;
    tools.score[color][EG] += safeCheck[EG][QUEEN] * queenChecksCount;

    //std::cout << knightChecksCount << " " << bishopChecksCount << " " << rookChecksCount << " " << queenChecksCount << "\n";

    if(TUNE) {
      trace.safeCheck[color][MG][KNIGHT] += knightChecksCount, trace.safeCheck[color][EG][KNIGHT] += knightChecksCount;
      trace.safeCheck[color][MG][BISHOP] += bishopChecksCount, trace.safeCheck[color][EG][BISHOP] += bishopChecksCount;
      trace.safeCheck[color][MG][ROOK] += rookChecksCount, trace.safeCheck[color][EG][ROOK] += rookChecksCount;
      trace.safeCheck[color][MG][QUEEN] += queenChecksCount, trace.safeCheck[color][EG][QUEEN] += queenChecksCount;
    }
  }

  /// assign bonus for pawns in king ring

  tools.score[color][MG] += pawnShield[MG][shieldCount];
  tools.score[color][EG] += pawnShield[EG][shieldCount];

  if(TUNE)
    trace.pawnShield[color][MG][shieldCount]++, trace.pawnShield[color][EG][shieldCount]++;

  tools.score[color][MG] -= tools.kingDanger[color];
  tools.score[color][EG] -= tools.kingDanger[color];
}

/// scale score if we recognize some draw patterns

int scaleFactor(Board &board, int eg) {
  bool oppositeBishops = 0;
  int color = (eg > 0 ? WHITE : BLACK);
  int scale = 100;
  if(count(board.bb[WB]) == 1 && count(board.bb[BB]) == 1) {
    int sq1 = Sq(board.bb[WB]), sq2 = Sq(board.bb[BB]);
    if(oppositeColor(sq1, sq2))
      oppositeBishops = 1;
  }

  /// scale down score for opposite color bishops, increase scale factor if we have any more pieces

  if(oppositeBishops)
    scale = 50 + 10 * (count(board.pieces[color] ^ board.bb[getType(PAWN, color)]) - 2); /// without the king and the bishop
  /* // doesn't work (loses elo)
  else {
    /// thanks to https://hxim.github.io/Stockfish-Evaluation-Guide/ for this idea
    scale = 80 +  5 * count(board.bb[getType(PAWN, color)]);
  }*/

  scale = std::min(scale, 100);

  return scale;
}

void eval(Board &board, int color, EvalTools &tools) {
  //cout << color << "\n";
  //cout << "initially: " << score[color][MG] << "\n";
  rookEval(board, color, tools);
  kingEval(board, color, tools);
}

void getTraceEntries(EvalTrace &trace) {
  int ind = 0;
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.doubledPawnsPenalty[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.isolatedPenalty[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.backwardPenalty[col][i]);
    ind++;
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 5, col, trace.mat[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 6, col, trace.passedBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 6, col, trace.connectedBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= QUEEN; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 4, col, trace.safeCheck[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 4; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 3, col, trace.pawnShield[col][s][i]);
      ind++;
    }
  }


  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 2, col, trace.outpostBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 2, col, trace.outpostHoleBonus[col][s][i]);
      ind++;
    }
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.rookOpenFile[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.rookSemiOpenFile[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.bishopPair[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.longDiagonalBishop[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.trappedRook[col][i]);
    ind++;
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 9, col, trace.mobilityBonus[col][KNIGHT][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 14, col, trace.mobilityBonus[col][BISHOP][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 15, col, trace.mobilityBonus[col][ROOK][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 28, col, trace.mobilityBonus[col][QUEEN][s][i]);
      ind++;
    }
  }

  //cout << ind << "\n";

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++) {
        for(int col = 0; s == MG && col < 2; col++) {
          trace.add(ind, ind + 64, col, trace.bonusTable[col][i][s][j]);
        }
        ind++;
      }
    }
  }
}

int evaluateTrace(TunePos &pos, int weights[]) {

  /*int score[2][2] = {
    {0, 0},
    {0, 0}
  };*/

  int sc[2] = {
    0, 0
  };

  int phase = pos.phase;

  //std::cout << weights.size() << "\n";

  for(int i = 0; i < pos.nrEntries; i++) {
    //std::cout << weights[trace.entries[i].ind] << " " << trace.entries[i].ind << " " << trace.entries[i].val << " " << trace.entries[i].color << " " << trace.entries[i].phase << "\n";
    /*score[WHITE][MG] += weights[pos.entries[i].mgind] * pos.entries[i].wval;
    score[BLACK][MG] += weights[pos.entries[i].mgind] * pos.entries[i].bval;
    score[WHITE][EG] += weights[pos.entries[i].egind] * pos.entries[i].wval;
    score[BLACK][EG] += weights[pos.entries[i].egind] * pos.entries[i].bval;*/

    sc[MG] += weights[pos.entries[i].mgind] * pos.entries[i].dval;
    sc[EG] += weights[pos.entries[i].mgind + pos.entries[i].deltaind] * pos.entries[i].dval;
  }

  /*score[WHITE][MG] += pos.others[WHITE];
  score[WHITE][EG] += pos.others[WHITE];
  score[BLACK][MG] += pos.others[BLACK];
  score[BLACK][EG] += pos.others[BLACK];*/

  sc[MG] += pos.others[WHITE] - pos.others[BLACK];
  sc[EG] += pos.others[WHITE] - pos.others[BLACK];

  /*score[WHITE][MG] -= pos.kingDanger[WHITE];
  score[WHITE][EG] -= pos.kingDanger[WHITE];
  score[BLACK][MG] -= pos.kingDanger[BLACK];
  score[BLACK][EG] -= pos.kingDanger[BLACK];*/

  sc[MG] -= pos.kingDanger[WHITE] - pos.kingDanger[BLACK];
  sc[EG] -= pos.kingDanger[WHITE] - pos.kingDanger[BLACK];

  /*int mg = score[WHITE][MG] - score[BLACK][MG], eg = score[WHITE][EG] - score[BLACK][EG];

  if(mg != sc[MG] || eg != sc[EG]) {
    std::cout << "Wrong!\n";
    std::cout << "mg: " << mg << ", " << sc[MG] << "; eg: " << eg << ", " << sc[EG] << "\n";
  }*/

  int mg = sc[MG], eg = sc[EG];

  eg = eg * pos.scale / 100;

  int eval = (mg * phase + eg * (maxWeight - phase)) / maxWeight;

  return TEMPO + eval * (pos.turn == WHITE ? 1 : -1);
}

int evaluate(Board &board, Search *searcher = nullptr) {
  EvalTools tools;

  tools.init();

  for(int col = BLACK; col <= WHITE; col++) {
    int king = board.king(col); /// board.king(color)

    tools.pawns[col] = board.bb[getType(PAWN, col)];
    tools.allPawns |= tools.pawns[col];

    tools.kingRing[col] = kingRingMask[king];
    tools.kingSquare[col] = kingSquareMask[king];

    tools.pawnShield[col] = pawnShieldMask[king];

    //printBB(pawnShield[col]);

    /// squares which are defended by 2 of our pawns are safe

    uint64_t doubleAttacked = shift(col, NORTHEAST, tools.pawns[col] & ~fileMask[(col == WHITE ? 7 : 0)]) &
                              shift(col, NORTHWEST, tools.pawns[col] & ~fileMask[(col == WHITE ? 0 : 7)]);

    tools.kingRing[col] &= ~doubleAttacked;

    tools.attackedBy[col] = tools.attackedByPiece[col][KING] = kingBBAttacks[board.king(col)];
    tools.defendedByPawn[col] = pawnAttacks(board, col);

    tools.attackedBy2[col] = tools.attackedBy[col] & tools.defendedByPawn[col];
    tools.attackedBy[col] |= tools.defendedByPawn[col];
  }

  int mg = 0, eg = 0;

  pieceEval(board, WHITE, tools);
  pieceEval(board, BLACK, tools);

  eval(board, WHITE, tools);
  eval(board, BLACK, tools);

  if(!TUNE) {
    tools.score[WHITE][MG] += board.score[MG];
    tools.score[WHITE][EG] += board.score[EG];
  } else {
    matEval(board, WHITE, tools);
    matEval(board, BLACK, tools);
  }


  if(TUNE)
    trace.kingDanger[WHITE] = tools.kingDanger[WHITE], trace.kingDanger[BLACK] = tools.kingDanger[BLACK];

  /// mg and eg score

  ptEntry entry = {};

  if(searcher && searcher->PT.probe(board.pawnKey, entry)) {
    passersEval(board, WHITE, tools, entry.passers & board.bb[WP]);
    passersEval(board, BLACK, tools, entry.passers & board.bb[BP]);

    mg = tools.score[WHITE][MG] - tools.score[BLACK][MG], eg = tools.score[WHITE][EG] - tools.score[BLACK][EG];

    mg += entry.eval[MG];
    eg += entry.eval[EG];

    /*int pawnScore[2][2] = {
      {0, 0},
      {0, 0},
    };

    uint64_t tmp = 0;

    pawnEval(board, WHITE, tools, pawnScore[WHITE], tmp);
    pawnEval(board, BLACK, tools, pawnScore[BLACK], tmp);

    int pScore[2] = {pawnScore[WHITE][MG] - pawnScore[BLACK][MG], pawnScore[WHITE][EG] - pawnScore[BLACK][EG]};

    if(entry.eval[MG] != pScore[MG] || entry.eval[EG] != pScore[EG]) {
      std::cout << board.pawnKey << "\n";
      board.print();
      std::cout << entry.eval[MG] << " " << pScore[MG] << " " << entry.eval[EG] << " " << pScore[EG] << "\n";
    }*/
  } else {
    int pawnScore[2][2] = {
      {0, 0},
      {0, 0},
    };

    uint64_t passers[2] = {0, 0};

    pawnEval(board, WHITE, tools, pawnScore[WHITE], passers[WHITE]);
    pawnEval(board, BLACK, tools, pawnScore[BLACK], passers[BLACK]);

    passersEval(board, WHITE, tools, passers[WHITE]);
    passersEval(board, BLACK, tools, passers[BLACK]);

    mg = tools.score[WHITE][MG] - tools.score[BLACK][MG], eg = tools.score[WHITE][EG] - tools.score[BLACK][EG];

    int pScore[2] = {pawnScore[WHITE][MG] - pawnScore[BLACK][MG], pawnScore[WHITE][EG] - pawnScore[BLACK][EG]};

    mg += pScore[MG];
    eg += pScore[EG];

    if(searcher)
      searcher->PT.save(board.pawnKey, pScore, passers[WHITE] | passers[BLACK]);
  }

  /// scale down eg score for winning side

  int scale = scaleFactor(board, eg);

  eg = eg * scale / 100;

  if(TUNE)
    trace.scale = scale;

  tools.phase = std::min(tools.phase, maxWeight);

  if(TUNE)
    trace.phase = tools.phase;

  int score = (mg * tools.phase + eg * (maxWeight - tools.phase)) / maxWeight; /// interpolate mg and eg score

  return TEMPO + score * (board.turn == WHITE ? 1 : -1);
}
