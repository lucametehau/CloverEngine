#pragma once
#include <cstring>
#include "attacks.h"
#include "defs.h"
#include "board.h"

bool TUNE = false; /// false by default

struct EvalTools { /// to avoid data races, kinda dirty tho
  uint64_t kingRing[2], kingSquare[2], pawnShield[2], defendedByPawn[2], pawns[2], allPawns;
  //uint64_t attackedBy[2], attackedBy2[2], attackedByPiece[2][7];

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
    //memset(attackedByPiece, 0, sizeof(attackedByPiece));
    memset(kingAttackersCount, 0, sizeof(kingAttackersCount));
    memset(kingAttackersWeight, 0, sizeof(kingAttackersWeight));
    memset(kingDanger, 0, sizeof(kingDanger));
    allPawns = 0;
  }
};

struct EvalTraceEntry {
  uint16_t ind;
  uint16_t val;
  bool phase, color;
};

class EvalTrace { /// when tuning, we can keep the count of every evaluation term used for speed up
public:
  int phase;

  int doubledPawnsPenalty[2][2];
  int isolatedPenalty[2][2];
  int backwardPenalty[2][2];

  int mat[2][2][7];

  int passedBonus[2][2][7];
  int connectedBonus[2][2][7];

  int pawnShield[2][2][4];

  int outpostBonus[2][2][4];
  int outpostHoleBonus[2][2][4];

  int rookOpenFile[2][2];
  int rookSemiOpenFile[2][2];

  int bishopPair[2][2];
  int longDiagonalBishop[2][2];

  int trappedRook[2][2];

  int mobilityBonus[2][7][2][30];

  int bonusTable[2][7][2][64];

  int kingDanger[2];

  int others[2];

  EvalTraceEntry entries[1700];
  int nrEntries;

  void add(int ind, bool phase, bool color, int val) {
    if(!val)
      return;
    entries[nrEntries].val = val;
    entries[nrEntries].color = color;
    entries[nrEntries].phase = phase;
    entries[nrEntries].ind = ind;
    nrEntries++;
  }

} trace;

class TunePos {
public:
  EvalTraceEntry *entries;

  int nrEntries;

  int others[2], kingDanger[2];

  int phase;

  bool turn;
};

const int TEMPO = 20;

int doubledPawnsPenalty[2] = {10, -19, };
int isolatedPenalty[2] = {-17, 1, };
int backwardPenalty[2] = {-17, -23, };

int mat[2][7] = {
    {0, 82, 355, 369, 522, 1075, 0},
    {0, 90, 307, 341, 605, 1125, 0},
};

int phaseVal[] = {0, 0, 1, 1, 2, 4};

const int maxWeight = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];

int passedBonus[2][7] = {
  {0, 9, 12, 30, 56, 114, 195},
  {0, 9, 12, 30, 56, 114, 195},
};
int connectedBonus[2][7] = {
  {0, 2, 3, 5, 9, 21, 26},
  {0, 2, 3, 5, 9, 21, 26},
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

int pawnShield[2][4] = {
  {0, 10, 20, 30},
  {0, 10, 20, 30},
};

int outpostBonus[2][4] = {
  {0, 0, 23, 18},
  {0, 0, 23, 18},
};
int outpostHoleBonus[2][4] = {
  {0, 0, 28, 25},
  {0, 0, 28, 25},
};

int rookOpenFile[2] = {50, 1, };
int rookSemiOpenFile[2] = {14, 9, };

int bishopPair[2] = {49, 64, };
int longDiagonalBishop[2] = {16, 6, };

int trappedRook[2] = {-28, -28};

int mobilityBonus[7][2][30] = {
    {},
    {},
    {
        {-21, 28, 8, 25, 37, 53, 54, 58, 60, },
        {19, -26, 22, 23, 23, 31, 29, 38, 33, },
    },
    {
        {-75, -39, 0, 16, 27, 36, 38, 45, 47, 54, 57, 57, 71, 67, },
        {-71, -26, -31, -8, 9, 16, 26, 32, 37, 34, 40, 36, 34, 37, },
    },
    {
        {-88, -47, -11, -7, -3, -1, 4, 11, 18, 15, 27, 29, 34, 41, 23, },
        {-31, -57, -11, 6, 28, 33, 42, 48, 50, 60, 65, 69, 72, 75, 73, },
    },
    {
        {-47, -38, -79, 20, 19, 16, 22, 23, 23, 29, 31, 34, 31, 36, 35, 36, 37, 45, 39, 45, 58, 56, 76, 84, 93, 96, 99, 85, },
        {-44, -34, -8, 1, -13, -41, -39, -16, -16, -8, 4, 14, 43, 42, 58, 69, 76, 89, 91, 103, 107, 109, 113, 117, 116, 117, 118, 113, },
    }
};

int bonusTable[7][2][64] = {
    {},
    {
        {
            11, 1, 1, -1, 1, 1, 3, 1,
            104, 62, 50, 68, 55, 75, 71, 35,
            5, -5, 15, 21, 61, 81, 40, -3,
            -1, 13, 11, 35, 36, 30, 21, -10,
            -14, -14, 3, 23, 33, 16, 0, -19,
            -12, -23, -4, -10, 4, 2, 14, -9,
            -18, -4, -15, -4, 3, 31, 35, -12,
            14, 9, -4, 2, 2, 0, 0, 0,

        },
        {
            0, -3, 0, 1, 0, 0, 0, -1,
            107, 97, 77, 50, 63, 47, 81, 108,
            86, 84, 57, 26, 10, 23, 57, 70,
            46, 30, 17, -5, 2, 9, 24, 31,
            27, 22, 5, -5, -3, -2, 10, 12,
            15, 19, 5, 11, 10, 7, 0, 2,
            28, 19, 23, 13, 22, 10, 6, 3,
            12, 0, 4, 0, 4, 0, 0, 0,

        },
    },
    {
        {
            -114, -73, -32, -13, 29, -71, -16, -79,
            -78, -53, 62, 19, 6, 61, 10, -10,
            -66, 21, 15, 28, 57, 79, 54, 15,
            -9, 16, 4, 49, 30, 70, 20, 26,
            -7, 11, 13, 7, 32, 19, 30, 5,
            -9, -1, 18, 24, 38, 32, 39, 4,
            -1, -27, -4, 17, 22, 29, 14, 20,
            -68, 8, -36, -19, 16, -4, 10, 5,

        },
        {
            -48, -19, 17, -8, 1, -16, -50, -83,
            1, 25, -3, 28, 17, -8, -2, -36,
            -28, -15, 10, 10, -6, -7, -26, -47,
            -14, 4, 26, 20, 27, 11, 11, -22,
            -13, -4, 20, 32, 24, 26, 12, -14,
            0, 21, 17, 32, 28, 13, -3, 3,
            -28, -1, 11, 9, 16, 1, -3, -30,
            -21, -29, 4, 14, 2, 6, -30, -51,

        },
    },
    {
        {
            -29, -14, -74, -59, -30, -32, -3, 1,
            -48, -6, -34, -37, 21, 33, -5, -70,
            -43, 4, 5, 1, 4, 16, 8, -18,
            -18, -2, -9, 31, 16, 17, 1, -12,
            -12, 9, 0, 15, 26, -2, 9, 13,
            9, 22, 4, 11, 15, 23, 27, 18,
            21, 15, 15, 5, 9, 36, 38, 18,
            -23, 17, 3, 0, 12, -3, -21, -17,

        },
        {
            -4, -4, 4, 6, 12, 4, 6, -15,
            23, 3, 18, 2, 6, 5, -1, 7,
            1, -12, -14, -12, -10, -14, -7, -6,
            -8, 1, 8, 3, 7, -2, -6, -5,
            -13, -4, 8, 15, 3, 12, -7, -12,
            -1, 5, 10, 14, 20, 2, 6, 2,
            -4, -12, -1, 4, 10, 0, -9, -15,
            -7, 0, -1, 7, 4, 6, 12, -5,

        },
    },
    {
        {
            19, 17, 0, 21, 25, -5, 10, 12,
            10, 22, 32, 37, 41, 34, 1, 18,
            -24, 0, -9, -9, -26, 9, 30, -8,
            -30, -23, -4, 10, -3, 9, -15, -19,
            -37, -29, -26, -17, -4, -24, 11, -18,
            -34, -19, -16, -20, 0, 0, 3, -17,
            -32, -8, -16, -1, 10, 12, 8, -50,
            2, 1, 10, 19, 21, 22, -13, 15,

        },
        {
            26, 24, 32, 28, 26, 27, 23, 22,
            28, 24, 25, 20, 9, 19, 26, 22,
            26, 22, 20, 22, 19, 10, 11, 12,
            25, 19, 26, 10, 14, 17, 12, 21,
            20, 21, 23, 14, 7, 10, 0, 5,
            8, 9, 0, 5, -3, -3, 1, -3,
            8, 3, 7, 7, -5, -3, -5, 15,
            9, 11, 10, 5, 3, 4, 12, -6,

        },
    },
    {
        {
            -10, 11, 22, 21, 62, 39, 48, 58,
            -15, -43, 1, 28, -13, 12, 23, 56,
            -9, -9, 5, -17, 19, 47, 25, 43,
            -20, -23, -21, -27, -12, -3, 3, -4,
            3, -28, -9, -20, -6, -10, 7, 3,
            -5, 27, -3, 9, 9, 13, 27, 28,
            -8, 6, 27, 30, 39, 45, 15, 36,
            2, 7, 14, 37, 20, -3, -3, -31,

        },
        {
            24, 56, 51, 54, 60, 51, 52, 68,
            19, 42, 46, 55, 69, 59, 45, 60,
            15, 21, 19, 73, 64, 58, 50, 57,
            46, 47, 44, 66, 73, 60, 75, 69,
            23, 61, 47, 70, 57, 55, 63, 62,
            37, -16, 36, 17, 30, 39, 44, 48,
            14, 6, -6, 1, 1, -8, 4, 11,
            13, -1, 11, -3, 26, 12, 20, -4,

        },
    },
    {
        {
            -68, 16, 20, -16, -44, -25, 2, 1,
            17, 1, -5, 9, -11, 6, -16, -47,
            18, 9, 12, -39, -14, 11, 24, 5,
            -1, -4, -22, -56, -42, -31, 0, -28,
            -37, 0, -30, -68, -64, -41, -40, -51,
            8, 7, -22, -46, -52, -32, -2, -7,
            9, 8, -38, -75, -59, -42, 8, 20,
            -6, 44, 21, -47, -2, -19, 47, 36,

        },
        {
            -107, -53, -37, -34, -17, 13, -6, -26,
            -24, 12, 12, 13, 19, 36, 18, 8,
            7, 22, 24, 24, 25, 58, 54, 5,
            -19, 23, 34, 42, 38, 44, 32, 0,
            -27, -4, 30, 39, 43, 34, 15, -15,
            -33, -6, 17, 30, 35, 26, 10, -16,
            -53, -32, 3, 14, 13, 2, -23, -41,
            -89, -69, -40, -17, -36, -25, -59, -83,

        },
    },
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
  uint64_t pieces = 0;

  /// evaluation pattern: apply a penaly if rook is trapped by king

  if(color == WHITE) {
    pieces = board.bb[WR];
    if((pieces & (1ULL << A1)) && (board.bb[getType(KING, color)] & between[A1][E1])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    } else if((pieces & (1ULL << H1)) && (board.bb[getType(KING, color)] & between[H1][E1])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    }
  } else {
    pieces = board.bb[BR];
    if((pieces & (1ULL << A8)) && (board.bb[getType(KING, color)] & between[A8][E8])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    } else if((pieces & (1ULL << H8)) && (board.bb[getType(KING, color)] & between[H8][E8])) {
      tools.score[color][MG] += trappedRook[MG];
      tools.score[color][EG] += trappedRook[EG];

      if(TUNE)
        trace.trappedRook[color][MG]++, trace.trappedRook[color][EG]++;
    }
  }

  while(pieces) {
    uint64_t b = lsb(pieces);
    int sq = Sq(b), file = sq % 8;
    /// check if open file
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
    pieces ^= b;
  }
}

void pawnEval(Board &board, int color, EvalTools &tools) {
  uint64_t pieces = tools.pawns[color], passers = 0, blockedPassers = 0;

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
        tools.score[color][MG] += backwardPenalty[MG];
        tools.score[color][EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileUpMask[sq] & tools.pawns[WHITE]) {
        tools.score[color][MG] += doubledPawnsPenalty[MG];
        tools.score[color][EG] += doubledPawnsPenalty[EG];

        if(TUNE)
          trace.doubledPawnsPenalty[color][MG]++, trace.doubledPawnsPenalty[color][EG]++;
      }
    } else {
      if(!(neighFileDownMask[sq] & tools.pawns[WHITE])) { /// no enemy pawns on the neighbour files on all ranks in front of it
        /*if(!(fileDownMask[sq] & tools.pawns[BLACK])) /// in case of double pawns, the one in the front is passed, but the other is not*/
          passers |= b;
      }

      if(neigh && !((neighFileUpMask[frontSq] ^ b) & tools.pawns[BLACK]) && ((1ULL << frontSq) & tools.defendedByPawn[WHITE])) {
        tools.score[color][MG] += backwardPenalty[MG];
        tools.score[color][EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileDownMask[sq] & tools.pawns[BLACK]) {
        tools.score[color][MG] += doubledPawnsPenalty[MG];
        tools.score[color][EG] += doubledPawnsPenalty[EG];

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

      tools.score[color][MG] += f * connectedBonus[MG][(color == WHITE ? rank : 7 - rank)] +
                                10 * count(defenders);
      tools.score[color][EG] += f * connectedBonus[EG][(color == WHITE ? rank : 7 - rank)] +
                                10 * count(defenders);
    }

    /// no supporting pawns
    if(!neigh) {
      tools.score[color][MG] += isolatedPenalty[MG];
      tools.score[color][EG] += isolatedPenalty[EG];

      if(TUNE)
        trace.isolatedPenalty[color][MG]++, trace.isolatedPenalty[color][EG]++;
    }


    pieces ^= b;
  }

  /// blocked passers aren't given any passed bonus - to change?

  blockedPassers = passers & shift(color, SOUTH, board.pieces[color ^ 1]);
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

    /*tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][KNIGHT] |= att;*/

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

    /*tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][BISHOP] |= att;*/

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
    int sq = Sq(b);
    att = genAttacksRook(all, sq);
    mob = att & mobilityArea;

    int cnt = count(mob);

    tools.score[color][MG] += mobilityBonus[ROOK][MG][cnt];
    tools.score[color][EG] += mobilityBonus[ROOK][EG][cnt];

    if(TUNE)
      trace.mobilityBonus[color][ROOK][MG][cnt]++, trace.mobilityBonus[color][ROOK][EG][cnt]++;

    tools.phase += phaseVal[ROOK];

    /*tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][ROOK] |= att;*/

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

    /*tools.attackedBy2[color] |= tools.attackedBy[color] & att;
    tools.attackedBy[color] |= att;
    tools.attackedByPiece[color][QUEEN] |= att;*/

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

  tools.score[color][MG] += bonusTable[KING][MG][pos];
  tools.score[color][EG] += bonusTable[KING][EG][pos];

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
  }

  /// assign bonus for pawns in king ring

  tools.score[color][MG] += pawnShield[MG][shieldCount];
  tools.score[color][EG] += pawnShield[EG][shieldCount];

  if(TUNE)
    trace.pawnShield[color][MG][shieldCount]++, trace.pawnShield[color][EG][shieldCount]++;

  tools.score[color][MG] -= tools.kingDanger[color];
  tools.score[color][EG] -= tools.kingDanger[color];
}

void initEval() {
  /*for(int color = BLACK; color <= WHITE; color++) {
    for(int p = PAWN; p <= QUEEN; p++) {
      for(int s = MG; s <= EG; s++) {
        for(int sq = A1; sq <= H8; sq++) {
          int r = sq / 8, f = sq % 8;
          if(color == WHITE)
            r = 7 - r;
          psqtBonus[color][p][s][sq] = bonusTable[p][MG][getSq(r, f)];
        }
      }
    }
  }*/
}


void eval(Board &board, int color, EvalTools &tools) {
  //cout << color << "\n";
  //cout << "initially: " << score[color][MG] << "\n";
   matEval(board, color, tools);
  rookEval(board, color, tools);
  pawnEval(board, color, tools);
  kingEval(board, color, tools);
}

void getTraceEntries(EvalTrace &trace) {
  int ind = 0;
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.doubledPawnsPenalty[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.isolatedPenalty[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.backwardPenalty[col][i]);
    ind++;
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = PAWN; i <= QUEEN; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.mat[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.passedBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 7; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.connectedBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 1; i < 4; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.pawnShield[col][s][i]);
      ind++;
    }
  }


  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.outpostBonus[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = KNIGHT; i <= BISHOP; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.outpostHoleBonus[col][s][i]);
      ind++;
    }
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.rookOpenFile[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.rookSemiOpenFile[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.bishopPair[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.longDiagonalBishop[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; col < 2; col++)
      trace.add(ind, i, col, trace.trappedRook[col][i]);
    ind++;
  }

  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 9; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.mobilityBonus[col][KNIGHT][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 14; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.mobilityBonus[col][BISHOP][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 15; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.mobilityBonus[col][ROOK][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 28; i++) {
      for(int col = 0; col < 2; col++)
        trace.add(ind, s, col, trace.mobilityBonus[col][QUEEN][s][i]);
      ind++;
    }
  }

  //cout << ind << "\n";

  for(int i = PAWN; i <= KING; i++) {
    for(int s = MG; s <= EG; s++) {
      for(int j = A1; j <= H8; j++) {
        for(int col = 0; col < 2; col++) {
          trace.add(ind, s, col, trace.bonusTable[col][i][s][j]);
        }
        ind++;
      }
    }
  }
}

int evaluateTrace(TunePos &pos, int weights[]) {

  int score[2][2] = {
    {0, 0},
    {0, 0}
  };

  int phase = pos.phase;

  //std::cout << weights.size() << "\n";

  for(int i = 0; i < pos.nrEntries; i++) {
    //std::cout << weights[trace.entries[i].ind] << " " << trace.entries[i].ind << " " << trace.entries[i].val << " " << trace.entries[i].color << " " << trace.entries[i].phase << "\n";
    score[pos.entries[i].color][pos.entries[i].phase] += weights[pos.entries[i].ind] * pos.entries[i].val;
  }

  score[WHITE][MG] += pos.others[WHITE];
  score[WHITE][EG] += pos.others[WHITE];
  score[BLACK][MG] += pos.others[BLACK];
  score[BLACK][EG] += pos.others[BLACK];

  score[WHITE][MG] -= pos.kingDanger[WHITE];
  score[WHITE][EG] -= pos.kingDanger[WHITE];
  score[BLACK][MG] -= pos.kingDanger[BLACK];
  score[BLACK][EG] -= pos.kingDanger[BLACK];

  int mg = score[WHITE][MG] - score[BLACK][MG], eg = score[WHITE][EG] - score[BLACK][EG];

  //std::cout << mg << " " << eg << "\n";

  int eval = (mg * phase + eg * (maxWeight - phase)) / maxWeight;

  return TEMPO + eval * (pos.turn == WHITE ? 1 : -1);
}

int evaluate(Board &board) {
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

    //uint64_t doubleAttacked = shift(col, NORTHEAST, tools.pawns[col] & ~fileMask[fileH]) & shift(col, NORTHWEST, tools.pawns[col] & ~fileMask[fileA]);

    //kingRing[col] &= ~doubleAttacked;

    //tools.attackedBy[col] = kingBBAttacks[board.king(col)] | pawnAttacks(board, col);
    tools.defendedByPawn[col] = pawnAttacks(board, col);
    //tools.attackedBy2[col] = doubleAttacked | (tools.defendedByPawn[col] & kingBBAttacks[board.king(col)]);
  }

  pieceEval(board, WHITE, tools);
  pieceEval(board, BLACK, tools);

  //cout << score << "\n";

  //cout << score << " piece score\n";
  eval(board, WHITE, tools);
  eval(board, BLACK, tools);

  if(TUNE)
    trace.kingDanger[WHITE] = tools.kingDanger[WHITE], trace.kingDanger[BLACK] = tools.kingDanger[BLACK];

  /// mg and eg score

  int mg = tools.score[WHITE][MG] - tools.score[BLACK][MG], eg = tools.score[WHITE][EG] - tools.score[BLACK][EG];

  //board.print();

  //cout << "mg = " << mg << ", eg = " << eg << ", weight = " << weight << ", score = " << (mg * weight + eg * (maxWeight - weight)) / maxWeight << "\n";

  tools.phase = std::min(tools.phase, maxWeight);

  //std::cout << mg << " " << eg << "\n";

  if(TUNE)
    trace.phase = tools.phase;

  int score = (mg * tools.phase + eg * (maxWeight - tools.phase)) / maxWeight; /// interpolate mg and eg score

  return TEMPO + score * (board.turn == WHITE ? 1 : -1);
}
