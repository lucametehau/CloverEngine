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

  uint8_t passerDistToEdge[2][2];
  uint8_t passerDistToKings[2][2];
  uint8_t doubledPawnsPenalty[2][2];
  uint8_t isolatedPenalty[2][2];
  uint8_t backwardPenalty[2][2];
  uint8_t pawnDefendedBonus[2][2];

  uint8_t threatByPawnPush[2][2];
  uint8_t threatMinorByMinor[2][2];

  uint8_t bishopSameColorAsPawns[2][2];

  uint8_t weakKingSq[2][2];

  uint8_t knightBehindPawn[2][2];

  uint8_t mat[2][2][7];

  uint8_t passedBonus[2][2][7];
  uint8_t blockedPassedBonus[2][2][7];
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

  uint8_t ocb, ocbPieceCount;
  uint8_t pawnsOn1Flank;
  uint8_t allPawnsCount;

  int8_t SafetyTable[2][2][100];
  int8_t kingShelter[2][2][4][8];
  int8_t kingStorm[2][2][4][8];
  int8_t blockedStorm[2][2][8];

  EvalTraceEntry entries[2000];
  int nrEntries;

  int mg, eg;
  int phase;

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

  int mg, eg;

  uint16_t nrEntries;

  uint16_t kingDanger[2][2];

  uint8_t phase;
  uint8_t scale;

  uint8_t ocb, ocbPieceCount;
  uint8_t pawnsOn1Flank;
  uint8_t allPawnsCount;

  bool turn;
};

const int TEMPO = 20;

int passerDistToEdge[2] = {-5, -5, };
int doubledPawnsPenalty[2] = {-5, -28, };
int isolatedPenalty[2] = {-4, -9, };
int backwardPenalty[2] = {-6, -16, };
int pawnDefendedBonus[2] = {12, 5, };

int threatByPawnPush[2] = {6, 9, };
int threatMinorByMinor[2] = {-9, -23, };

int bishopSameColorAsPawns[2] = {-3, -6, };

int knightBehindPawn[2] = {4, 13, };

int weakKingSq[2] = {-17, 1, };

const int phaseVal[] = {0, 0, 1, 1, 2, 4};
const int maxWeight = 16 * phaseVal[PAWN] + 4 * phaseVal[KNIGHT] + 4 * phaseVal[BISHOP] + 4 * phaseVal[ROOK] + 2 * phaseVal[QUEEN];

int passedBonus[2][7] = {
  {0, -5, -3, 3, 29, 38, 74},
  {0, 20, 26, 54, 83, 150, 143},
};
int blockedPassedBonus[2][7] = {
  {0, -3, 0, 13, 36, 41, 35},
  {0, -3, 8, 20, 28, 55, 41},
};
int connectedBonus[2][7] = {
  {0, 1, 3, 3, 6, 21, 70},
  {0, -1, 1, 3, 11, 21, 23},
};
int kingAttackWeight[] = {0, 0, 2, 2, 3, 5};
int SafetyTable[2][100] = {
  {
    0, -1, 4, 1, 0, 6, 9, 4, 17, 16,
    37, 28, 13, 38, 28, 62, 50, 25, 57, 47,
    79, 69, 53, 81, 65, 112, 98, 86, 113, 109,
    144, 133, 167, 178, 179, 200, 210, 224, 238, 248,
    260, 272, 284, 295, 307, 319, 330, 342, 354, 366,
    377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
    494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  },
  {
    0, -4, -1, -2, 0, -1, 7, 14, 14, 18,
    26, 20, 27, 26, 34, 46, 34, 44, 48, 52,
    67, 68, 74, 80, 77, 102, 100, 104, 116, 122,
    141, 140, 169, 179, 183, 201, 212, 224, 237, 248,
    260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
    377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
    494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
  },
};
int kingShelter[2][4][7] = {
  {
    {-20, 12, 17, 4, 2, 8, 2, },
    {-19, 22, 18, -9, -2, 8, 1, },
    {-16, 18, -4, -9, -1, 1, 5, },
    {-13, 10, 1, -6, -8, -20, -1, },
  },
  {
    {0, -36, -17, 0, 15, 20, 14, },
    {0, -14, -13, 5, 13, 15, 12, },
    {-11, -9, -5, -2, 7, 12, 10, },
    {-19, -16, -14, 1, 13, 12, 6, },
  },
};
int kingStorm[2][4][7] = {
  {
    {-12, 22, 17, -18, -9, 2, 0, },
    {-20, 7, -1, -23, -7, 12, 1, },
    {-11, 3, -5, -24, 0, 9, 7, },
    {2, 7, 1, -15, 2, 18, 18, },
  },
  {
    {-32, 42, 66, 30, -7, -25, -16, },
    {-26, 33, 55, 23, -3, -25, -26, },
    {-24, 30, 46, 19, -11, -24, -33, },
    {-12, 38, 52, 17, -6, -14, -12, },
  },
};
int blockedStorm[2][7] = {
  {0, 0, -16, 12, 13, 11, -6, },
  {0, 0, -6, -27, -40, -48, -28, },
};
int safeCheck[2][6] = {
  {0, 0, -64, -16, -52, -35},
  {0, 0, 10, -9, 5, -20},
};
int outpostBonus[2][4] = {
  {0, 0, 23, 24},
  {0, 0, -2, 6},
};
int outpostHoleBonus[2][4] = {
  {0, 0, 17, 19},
  {0, 0, 2, 9},
};

int rookOpenFile[2] = {31, 7, };
int rookSemiOpenFile[2] = {12, 13, };

int bishopPair[2] = {16, 78, };
int longDiagonalBishop[2] = {7, 17, };
int trappedRook[2] = {-28, -17, };

int mobilityBonus[7][2][30] = {
    {},
    {},
    {
        {-58, -6, 16, 24, 34, 36, 42, 50, 60, },
        {-30, -34, 7, 43, 53, 72, 74, 70, 48, },
    },
    {
        {-56, -8, 10, 15, 25, 30, 32, 33, 33, 38, 40, 57, 58, 63, },
        {-58, -64, -27, 7, 18, 39, 51, 56, 62, 64, 61, 47, 55, 33, },
    },
    {
        {-67, -29, -13, -7, -5, -4, -3, 1, 5, 9, 11, 16, 19, 31, 62, },
        {-92, -50, -18, 12, 32, 47, 59, 65, 72, 77, 83, 84, 85, 73, 49, },
    },
    {
        {-310, -148, -74, -8, 3, 16, 31, 32, 37, 38, 43, 45, 49, 51, 51, 50, 51, 46, 45, 41, 46, 52, 50, 56, 60, 123, 64, 124, },
        {-236, -90, -6, -85, -36, 5, -31, 10, 18, 44, 51, 72, 67, 79, 84, 92, 97, 101, 107, 110, 107, 103, 106, 107, 114, 69, 93, 132, },
    }
};

int ocbStart = 35;
int ocbStep = 31;
int pawnsOn1Flank = 21;
int pawnScaleStart = 48;
int pawnScaleStep = 12;

/// evaluate material

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

/// evaluate trapped rooks (TO DO: make code shorter and faster?)

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

/// evaluate pawns and pawn structure

void pawnEval(int color, EvalTools &tools, int pawnScore[], uint64_t &passedPawns) {
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
        //std::cout << "backward " << sq << "\n";
        pawnScore[MG] += backwardPenalty[MG];
        pawnScore[EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileUpMask[sq] & tools.pawns[WHITE]) {
        //std::cout << "doubled " << sq << "\n";
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
        //std::cout << "backward " << sq << "\n";
        pawnScore[MG] += backwardPenalty[MG];
        pawnScore[EG] += backwardPenalty[EG];

        if(TUNE)
          trace.backwardPenalty[color][MG]++, trace.backwardPenalty[color][EG]++;
      }
      if(fileDownMask[sq] & tools.pawns[BLACK]) {
        //std::cout << "doubled " << sq << "\n";
        pawnScore[MG] += doubledPawnsPenalty[MG];
        pawnScore[EG] += doubledPawnsPenalty[EG];

        if(TUNE)
          trace.doubledPawnsPenalty[color][MG]++, trace.doubledPawnsPenalty[color][EG]++;
      }
    }
    /// check for isolated pawn / connected pawns

    if(phalanx | defenders) {
      /// give bonus if pawn is connected, increase it if pawn makes a phalanx, decrease it if pawn is blocked

      //std::cout << sq << " " << phalanx << " " << defenders << "\n";
      int f = (phalanx ? 4 : 2) / (opposed ? 2 : 1);

      if(TUNE) {
        trace.connectedBonus[color][MG][(color == WHITE ? rank : 7 - rank)] += f;
        trace.connectedBonus[color][EG][(color == WHITE ? rank : 7 - rank)] += f;
        //std::cout << trace.connectedBonus[color][EG][(color == WHITE ? rank : 7 - rank)] << " " << phalanx << " " << opposed << "\n";
        trace.pawnDefendedBonus[color][MG] += count(defenders);
        trace.pawnDefendedBonus[color][EG] += count(defenders);
      }

      //std::cout << f << " " << connectedBonus[MG][(color == WHITE ? rank : 7 - rank)] << "\n";

      pawnScore[MG] += f * connectedBonus[MG][(color == WHITE ? rank : 7 - rank)] +
                                pawnDefendedBonus[MG] * count(defenders);
      pawnScore[EG] += f * connectedBonus[EG][(color == WHITE ? rank : 7 - rank)] +
                                pawnDefendedBonus[EG] * count(defenders);
    }

    /// no supporting pawns
    if(!neigh) {
      //std::cout << "isolated pawn on " << sq << "\n";
      pawnScore[MG] += isolatedPenalty[MG];
      pawnScore[EG] += isolatedPenalty[EG];

      if(TUNE)
        trace.isolatedPenalty[color][MG]++, trace.isolatedPenalty[color][EG]++;
    }


    pieces ^= b;
  }


  passedPawns = passers;
}

/// evaluate passers

void passersEval(Board &board, int color, EvalTools &tools, uint64_t passers) {
  /// blocked passers aren't given any passed bonus - to change? EDIT: changed

  while(passers) {
    uint64_t b = lsb(passers);
    int sq = Sq(b);
    //int frontSq = sqDir(color, NORTH, sq);
    int rank = (color == WHITE ? sq / 8 : 7 - sq / 8), file = sq % 8;

    if(!(b & shift(color, SOUTH, board.pieces[color ^ 1]))) {
      tools.score[color][MG] += passedBonus[MG][rank];
      tools.score[color][EG] += passedBonus[EG][rank];

      if(TUNE)
        trace.passedBonus[color][MG][rank]++, trace.passedBonus[color][EG][rank]++;
    } else {
      tools.score[color][MG] += blockedPassedBonus[MG][rank];
      tools.score[color][EG] += blockedPassedBonus[EG][rank];

      if(TUNE)
        trace.blockedPassedBonus[color][MG][rank]++, trace.blockedPassedBonus[color][EG][rank]++;
    }

    /// in the endgame, the closer to the edge, the better

    int distToEdge = (file > 3 ? 7 - file : file);

    tools.score[color][MG] += passerDistToEdge[MG] * distToEdge;
    tools.score[color][EG] += passerDistToEdge[EG] * distToEdge;

    if(TUNE)
      trace.passerDistToEdge[color][MG] += distToEdge, trace.passerDistToEdge[color][EG] += distToEdge;

    /// the following code doesn't gain elo, but in the future it might...
    /*/// distance to advancing square is better

    int distToEnemyKing = distance[frontSq][board.king(color ^ 1)], distToOurKing = distance[frontSq][board.king(color)];
    int deltaDist = distToEnemyKing - distToOurKing;

    deltaDist = std::max(-5, std::min(deltaDist, 5)); /// clamp distance between [-5, 5]

    tools.score[color][MG] += passerDistToKings[MG] * deltaDist;
    tools.score[color][EG] += passerDistToKings[EG] * deltaDist;

    if(TUNE)
      trace.passerDistToKings[color][MG] += deltaDist, trace.passerDistToKings[color][EG] += deltaDist;
    */

    passers ^= b;
  }
}

/// evaluate pieces and give specific bonuses for every piece

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

    if(b & shift(color, SOUTH, tools.pawns[color])) {
      tools.score[color][MG] += knightBehindPawn[MG];
      tools.score[color][EG] += knightBehindPawn[EG];

      //std::cout << "knight behind pawn at " << sq << "\n";

      if(TUNE)
        trace.knightBehindPawn[color][MG]++, trace.knightBehindPawn[color][EG]++;
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
    att = genAttacksBishop(all ^ board.bb[getType(BISHOP, color)], sq);
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

    cnt = count(tools.pawns[color] & ((sq / 8 + sq % 8) % 2 ? LIGHT_SQUARES : DARK_SQUARES));

    tools.score[color][MG] += bishopSameColorAsPawns[MG] * cnt;
    tools.score[color][EG] += bishopSameColorAsPawns[EG] * cnt;

    //std::cout << cnt << " pawns on the same color as the bishop on " << sq << "\n";

    if(TUNE) {
      trace.bishopSameColorAsPawns[color][MG] += cnt;
      trace.bishopSameColorAsPawns[color][EG] += cnt;
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
    att = genAttacksRook(all ^ board.bb[getType(ROOK, color)], sq);
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


    if(att & tools.kingRing[enemy]) {
      /// update king safety terms
      tools.kingAttackersWeight[color] += kingAttackWeight[ROOK] * count(att & tools.kingRing[enemy]);
      tools.kingAttackersCount[color]++;
    }

    pieces ^= b;
  }

  pieces = board.bb[getType(QUEEN, color)];
  while(pieces) {
    b = lsb(pieces);
    int sq = Sq(b);
    att = genAttacksQueen(all, sq);
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

/// evaluate king safety

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

  /// as on cpw (https://www.chessprogramming.org/King_Safety)

  if(tools.kingAttackersCount[enemy] >= 2) {
    int weight = std::min(99, tools.kingAttackersWeight[enemy]);

    //cout << weight << "\n";

    if(!board.bb[getType(QUEEN, enemy)])
      weight /= 2;

    if(!board.bb[getType(ROOK, enemy)])
      weight *= 0.75;

    tools.score[color][MG] -= SafetyTable[MG][weight];
    tools.score[color][EG] -= SafetyTable[EG][weight];

    if(TUNE) {
      trace.SafetyTable[color][MG][weight]--;
      trace.SafetyTable[color][EG][weight]--;
    }
  }

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

  /// penalty for weak squares

  int cntWeak = count(weak & tools.kingRing[color]);

  tools.score[color][MG] += weakKingSq[MG] * cntWeak;
  tools.score[color][EG] += weakKingSq[EG] * cntWeak;

  if(TUNE) {
    trace.weakKingSq[color][MG] += cntWeak, trace.weakKingSq[color][EG] += cntWeak;
  }

  /// evaluate king storm, shelter and blocked storms as on cpw

  int f = king & 7;

  f = (f < 1 ? 1 : (f > 6 ? 6 : f)); /// clamp king file between B and G files

  for(int file = f - 1; file <= f + 1; file++) {

    uint64_t b = fileMask[file] & tools.pawns[color];
    int ours = getFrontBit(enemy, b) / 8;
    int rankOurs = (b ? (color == WHITE ? ours : 7 - ours) : 0); /// our first pawn on this file

    b = fileMask[file] & tools.pawns[enemy];

    int theirs = getFrontBit(enemy, b) / 8;

    int rankTheirs = (b ? (color == WHITE ? theirs : 7 - theirs) : 0); /// their first pawn on this file

    int distToEdge = (file > 3 ? 7 - file : file);

    //std::cout << rankOurs << " " << rankTheirs << "\n";

    tools.score[color][MG] += kingShelter[MG][distToEdge][rankOurs];
    tools.score[color][EG] += kingShelter[EG][distToEdge][rankOurs];

    if(TUNE) {
      trace.kingShelter[color][MG][distToEdge][rankOurs]++;
      trace.kingShelter[color][EG][distToEdge][rankOurs]++;
    }

    if(rankOurs && rankOurs == rankTheirs - 1) {
      tools.score[color][MG] += blockedStorm[MG][rankTheirs];
      tools.score[color][EG] += blockedStorm[EG][rankTheirs];

      if(TUNE) {
        trace.blockedStorm[color][MG][rankTheirs]++;
        trace.blockedStorm[color][EG][rankTheirs]++;
      }
    } else {
      tools.score[color][MG] += kingStorm[MG][distToEdge][rankTheirs];
      tools.score[color][EG] += kingStorm[EG][distToEdge][rankTheirs];

      if(TUNE) {
        trace.kingStorm[color][MG][distToEdge][rankTheirs]++;
        trace.kingStorm[color][EG][distToEdge][rankTheirs]++;
      }
    }
  }
}

/// in testing phase:
/// evaluate piece threats

void threatsEval(Board &board, int color, EvalTools &tools) {
  bool enemy = 1 ^ color;
  uint64_t b = shift(color, NORTH, tools.pawns[color]) & ~(board.pieces[WHITE] | board.pieces[BLACK]); /// for pawn push threat
  uint64_t safe = ~tools.attackedBy[enemy] | tools.attackedBy[color];
  int cnt; /// for keeping count

  b |= shift(color, NORTH, b & rankMask[(color == WHITE ? 2 : 5)]) & ~(board.pieces[WHITE] | board.pieces[BLACK]); /// consider double pushes

  b &= ~tools.defendedByPawn[enemy] & safe; /// don't consider pawn pushes to unsafe squares

  b = getPawnAttacks(color, b) & (board.pieces[enemy] ^ board.bb[getType(PAWN, enemy)]); /// exclude pawn pushes that attack enemy pawns

  cnt = count(b);

  tools.score[color][MG] += threatByPawnPush[MG] * cnt;
  tools.score[color][EG] += threatByPawnPush[EG] * cnt;

  if(TUNE) {
    trace.threatByPawnPush[color][MG] += cnt;
    trace.threatByPawnPush[color][MG] += cnt;
  }

  b = (tools.attackedByPiece[enemy][KNIGHT] | tools.attackedByPiece[enemy][BISHOP]) &
      (board.bb[getType(KNIGHT, color)] | board.bb[getType(BISHOP, color)]); /// minors attacked by enemy minors

  cnt = count(b);

  tools.score[color][MG] += threatMinorByMinor[MG] * cnt;
  tools.score[color][EG] += threatMinorByMinor[EG] * cnt;

  if(TUNE) {
    trace.threatMinorByMinor[color][MG] += cnt;
    trace.threatMinorByMinor[color][MG] += cnt;
  }
}

/// scale score if we recognize some draw patterns

int scaleFactor(Board &board, EvalTrace &trace, int eg) {
  bool oppositeBishops = 0;
  int color = (eg > 0 ? WHITE : BLACK);
  int scale = 100;
  uint64_t allPawns = board.bb[WP] | board.bb[BP];

  if(count(board.bb[WB]) == 1 && count(board.bb[BB]) == 1 && count((board.bb[WB] | board.bb[BB]) & DARK_SQUARES) == 1) {
    oppositeBishops = 1;
  }

  /// scale down score for opposite color bishops, increase scale factor if we have any more pieces

  if(oppositeBishops) {
    if(TUNE) {
      trace.ocb = 1;
    }

    scale = ocbStart + ocbStep * (count(board.pieces[color] ^ board.bb[getType(PAWN, color)]) - 2); /// without the king and the bishop

    if(TUNE) {
      trace.ocbPieceCount = count(board.pieces[color] ^ board.bb[getType(PAWN, color)]) - 2;
    }
  } else {
    int cnt = count(board.bb[getType(PAWN, color)]);

    scale = pawnScaleStart + pawnScaleStep * cnt;

    if(TUNE) {
      trace.allPawnsCount = cnt;
    }
  }

  /// positions with pawns on only 1 flank tend to be drawish

  scale -= pawnsOn1Flank * (!((allPawns & flankMask[0]) && (allPawns & flankMask[1])));

  if(TUNE) {
    trace.pawnsOn1Flank = !((allPawns & flankMask[0]) && (allPawns & flankMask[1]));
  }

  scale = std::max(scale, 0);
  scale = std::min(scale, 100);

  return scale;
}

int scaleFactorTrace(TunePos &pos) {
  int scale = 100;
  if(pos.ocb)
    scale = ocbStart + ocbStep * pos.ocbPieceCount;
  else
    scale = pawnScaleStart + pawnScaleStep * pos.allPawnsCount;
  scale -= pawnsOn1Flank * pos.pawnsOn1Flank;

  scale = std::max(scale, 0);
  scale = std::min(scale, 100);
  return scale;
}

/// evaluate for color

void eval(Board &board, int color, EvalTools &tools) {
  //cout << color << "\n";
  //cout << "initially: " << score[color][MG] << "\n";
  rookEval(board, color, tools);
  kingEval(board, color, tools);
  threatsEval(board, color, tools);
}

void getTraceEntries(EvalTrace &trace) {
  int ind = 0;
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.passerDistToEdge[col][i]);
    ind++;
  }
  /*for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.passerDistToKings[col][i]);
    ind++;
  }*/
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
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.pawnDefendedBonus[col][i]);
    ind++;
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.threatByPawnPush[col][i]);
    ind++;
  }
  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.threatMinorByMinor[col][i]);
    ind++;
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.bishopSameColorAsPawns[col][i]);
    ind++;
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.knightBehindPawn[col][i]);
    ind++;
  }

  for(int i = MG; i <= EG; i++) {
    for(int col = 0; i == MG && col < 2; col++)
      trace.add(ind, ind + 1, col, trace.weakKingSq[col][i]);
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
        trace.add(ind, ind + 6, col, trace.blockedPassedBonus[col][s][i]);
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
    for(int i = 0; i < 100; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 100, col, trace.SafetyTable[col][s][i]);
      ind++;
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++) {
        for(int col = 0; s == MG && col < 2; col++)
          trace.add(ind, ind + 28, col, trace.kingShelter[col][s][i][j]);
        ind++;
      }
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 7; j++) {
        for(int col = 0; s == MG && col < 2; col++)
          trace.add(ind, ind + 28, col, trace.kingStorm[col][s][i][j]);
        ind++;
      }
    }
  }
  for(int s = MG; s <= EG; s++) {
    for(int i = 0; i < 7; i++) {
      for(int col = 0; s == MG && col < 2; col++)
        trace.add(ind, ind + 7, col, trace.blockedStorm[col][s][i]);
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

double evaluateTrace(TunePos &pos, double weights[]) {

  /*int score[2][2] = {
    {0, 0},
    {0, 0}
  };*/

  double sc[2] = {
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

  /*int mg = score[WHITE][MG] - score[BLACK][MG], eg = score[WHITE][EG] - score[BLACK][EG];

  if(mg != sc[MG] || eg != sc[EG]) {
    std::cout << "Wrong!\n";
    std::cout << "mg: " << mg << ", " << sc[MG] << "; eg: " << eg << ", " << sc[EG] << "\n";
  }*/

  double mg = sc[MG], eg = sc[EG];

  eg = eg * pos.scale / 100;

  double eval = (mg * phase + eg * (maxWeight - phase)) / maxWeight;

  return TEMPO + eval * (pos.turn == WHITE ? 1 : -1);
}

int evaluate(Board &board, Search *searcher = nullptr) {
  //return 0; -> this gives some interesting results when searching for mates
  EvalTools tools;

  tools.init();

  /// initialize evaluation tools

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

    pawnEval(WHITE, tools, pawnScore[WHITE], tmp);
    pawnEval(BLACK, tools, pawnScore[BLACK], tmp);

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

    pawnEval(WHITE, tools, pawnScore[WHITE], passers[WHITE]);
    pawnEval(BLACK, tools, pawnScore[BLACK], passers[BLACK]);

    passersEval(board, WHITE, tools, passers[WHITE]);
    passersEval(board, BLACK, tools, passers[BLACK]);

    mg = tools.score[WHITE][MG] - tools.score[BLACK][MG], eg = tools.score[WHITE][EG] - tools.score[BLACK][EG];

    int pScore[2] = {pawnScore[WHITE][MG] - pawnScore[BLACK][MG], pawnScore[WHITE][EG] - pawnScore[BLACK][EG]};

    mg += pScore[MG];
    eg += pScore[EG];

    if(searcher)
      searcher->PT.save(board.pawnKey, pScore, passers[WHITE] | passers[BLACK]);
  }

  if(TUNE) {
    trace.mg = mg;
    trace.eg = eg;
  }

  /// scale down eg score for winning side

  int scale = scaleFactor(board, trace, eg);

  eg = eg * scale / 100;

  if(TUNE)
    trace.scale = scale;

  tools.phase = std::min(tools.phase, maxWeight);

  if(TUNE)
    trace.phase = tools.phase;

  int score = (mg * tools.phase + eg * (maxWeight - tools.phase)) / maxWeight; /// interpolate mg and eg score

  return TEMPO + score * (board.turn == WHITE ? 1 : -1);
}
