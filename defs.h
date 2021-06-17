#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <ratio>
#include <random>
#include <cassert>

std::mt19937_64 gen(0xBEEF);
std::uniform_int_distribution <uint64_t> rng;

enum {
  A1 = 0, B1, C1, D1, E1, F1, G1, H1,
  A2    , B2, C2, D2, E2, F2, G2, H2,
  A3    , B3, C3, D3, E3, F3, G3, H3,
  A4    , B4, C4, D4, E4, F4, G4, H4,
  A5    , B5, C5, D5, E5, F5, G5, H5,
  A6    , B6, C6, D6, E6, F6, G6, H6,
  A7    , B7, C7, D7, E7, F7, G7, H7,
  A8    , B8, C8, D8, E8, F8, G8, H8,
};

enum {
  PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum {
  BP = 1, BN, BB, BR, BQ, BK,
  WP    , WN, WB, WR, WQ, WK
};

enum {
  BLACK = 0, WHITE
};

enum {
  NEUT = 0, PROMOTION, CASTLE, ENPASSANT
};

enum {
  NORTH = 0, EAST, SOUTH, WEST, NORTHEAST, NORTHWEST, SOUTHEAST, SOUTHWEST
};

enum {
  NONE = 0, UPPER, LOWER, EXACT
};

enum {
  MG = 0, EG
};

const int NULLMOVE = 0;
const int HALFMOVES = 100;
const int INF = 32000;
const int MATE = 31000;
const int TB_WIN_SCORE = 22000;
const int ABORT = 0;
const int DEPTH = 1000;
const uint64_t CENTER = 103481868288ULL;
const uint64_t ALL = 18446744073709551615ULL;
const uint64_t LONG_DIAGONALS =  9314046665258451585ULL;
const std::string START_POS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

bool TUNE = false; /// false by default, automatically set to true when tuning

int cod[256];
uint64_t hashKey[13][64], castleKey[2][2], enPasKey[64];
uint64_t castleKeyModifier[16];
int captureValue[7][7];
char piece[13];
uint64_t fileMask[8], rankMask[8];
uint64_t fileUpMask[64], neighFileUpMask[64];
uint64_t fileDownMask[64], neighFileDownMask[64];
uint64_t neighFilesMask[64];
uint64_t between[64][64], Line[64][64];
uint64_t flankMask[8];
int mirrorSq[2][64];
int distance[64][64];

const std::pair <int, int> knightDir[] = {{-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}, {1, 2}, {2, 1}, {2, -1}, {1, -2}};
const std::pair <int, int> rookDir[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
const std::pair <int, int> bishopDir[] = {{-1, 1}, {-1, -1}, {1, -1}, {1, 1}};
const std::pair <int, int> kingDir[] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
const std::pair <int, int> pawnCapDirWhite[] = {{1, -1}, {1, 1}};
const std::pair <int, int> pawnCapDirBlack[] = {{-1, -1}, {-1, 1}};

int deltaPos[8]; /// how does my position change when moving in direction D

const int castleRightsDelta[2][64] = {
  {
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
       14, 255, 255, 255,   12, 255, 255,   13,
  },
  {
       11, 255, 255, 255,    3, 255, 255,    7,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
      255, 255, 255, 255,  255, 255, 255,  255,
  }
};

inline long double getTime() { /// thanks Terje!
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return ((uint64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000);
}

inline uint64_t lsb(uint64_t nr) {
  return nr & -nr;
}

inline int piece_type(int piece) {
  return (piece > 6 ? piece - 6 : piece);
}

inline int hashVal(int value, int ply) {
  if(value >= MATE)
    return value - ply;
  else if(value <= -MATE)
    return value + ply;
  return value;
}

inline int smallPopCount(uint64_t b) { /// better than count when b has small number of bits
  int count = 0;
  while(b) {
    count++;
    b &= b - 1;
  }
  return count;
}

inline int count(uint64_t b) {
  return __builtin_popcountll(b);
}

inline int getSq(int rank, int file) {
  return (rank << 3) | file;
}

inline int mirror(int color, int sq) {
  return (color == WHITE ? sq : (7 - sq / 8) * 8 + sq % 8);
}

inline int mirrorVert(int sq) {
  return (sq % 8 >= 4 ? 7 - sq % 8 : sq % 8) + 8 * (sq / 8);
}

inline int Sq(uint64_t bb) {
  return 63 - __builtin_clzll(bb);
}

int getFirstBit(int color, uint64_t bb) {
  if(!bb)
    return 0;

  return (color == WHITE ? Sq(lsb(bb)) : Sq(bb));
}

inline int oppositeColor(int sq1, int sq2) {
  return (((sq1 >> 3) + (sq1 & 7)) & 1) != (((sq2 >> 3) + (sq2 & 7)) & 1);
}

inline int sqDir(int color, int dir, int sq) {
  if(color == BLACK) {
    if(dir < 4)
      dir = (dir + 2) % 4;
    else
      dir = 11 - dir;
  }
  return sq + deltaPos[dir];

}

inline uint64_t shift(int color, int dir, uint64_t mask) {
  if(color == BLACK) {
    if(dir < 4)
      dir = (dir + 2) % 4;
    else
      dir = 11 - dir;
  }
  if(deltaPos[dir] > 0)
    return (mask << deltaPos[dir]);

  return (mask >> (-deltaPos[dir]));
}

inline int getType(int type, int color) {
  return 6 * color + type;
}

inline bool inTable(int rank, int file) {
  return (rank >= 0 && file >= 0 && rank <= 7 && file <= 7);
}

inline int getMove(int from, int to, int prom, int type) {
  return from | (to << 6) | (prom << 12) | (type << 14);
}

inline int sqFrom(uint16_t move) {
  return move & 63;
}

inline int sqTo(uint16_t move) {
  return (move & 4095) >> 6;
}

inline int promoted(uint16_t move) {
  return (move & 16383) >> 12;
}

inline int type(uint16_t move) {
  return move >> 14;
}

inline uint16_t inv(uint16_t move) { ///
  if(move >= (1 << 12))
    return NULLMOVE;
  return ((move << 6) & 4095) | (move >> 6);
}

inline std::string toString(uint16_t move) {
  int sq1 = sqFrom(move), sq2 = sqTo(move);
  std::string ans = "";
  ans += char((sq1 & 7) + 'a');
  ans += char((sq1 >> 3) + '1');
  ans += char((sq2 & 7) + 'a');
  ans += char((sq2 >> 3) + '1');
  if(type(move) == PROMOTION)
    ans += piece[((move & 16383) >> 12) + BN];
  return ans;
}

inline void printBB(uint64_t mask) {
  while(mask) {
    uint64_t b = lsb(mask);
    std::cout << Sq(b) << " ";
    mask ^= b;
  }
  std::cout << " mask\n";
}

inline void init_defs() {

  deltaPos[NORTH] = 8, deltaPos[SOUTH] = -8;
  deltaPos[WEST] = -1, deltaPos[EAST] = 1;
  deltaPos[NORTHWEST] = 7, deltaPos[NORTHEAST] = 9;
  deltaPos[SOUTHWEST] = -9, deltaPos[SOUTHEAST] = -7;

  for(int i = 0; i < 256; i++)
    cod[i] = 0;

  cod['p'] = 1, cod['n'] = 2, cod['b'] = 3,  cod['r'] = 4,  cod['q'] = 5,  cod['k'] = 6,
  cod['P'] = 7, cod['N'] = 8, cod['B'] = 9, cod['R'] = 10, cod['Q'] = 11, cod['K'] = 12;

  piece[0] = '.';
  piece[1] = 'p', piece[2] = 'n', piece[3] = 'b',  piece[4] = 'r',  piece[5] = 'q',  piece[6] = 'k';
  piece[7] = 'P', piece[8] = 'N', piece[9] = 'B', piece[10] = 'R', piece[11] = 'Q', piece[12] = 'K';

  /// zobrist keys

  for(int i = 0; i <= 12; i++) {
    for(int j = 0; j < 64; j++)
      hashKey[i][j] = rng(gen);
  }

  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 2; j++)
      castleKey[i][j] = rng(gen);
  }

  for(int i = 0; i < (1 << 4); i++) {
    for(int j = 0; j < 4; j++) {
      castleKeyModifier[i] ^= castleKey[j / 2][j % 2] * ((i >> j) & 1);
    }
  }

  for(int i = 0; i < 64; i++)
    enPasKey[i] = rng(gen);

  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 64; j++)
      mirrorSq[i][j] = mirror(i, j);
  }

  for(int i = 0; i < 64; i++) {
    for(int j = 0; j < 64; j++) {
      distance[i][j] = std::max(abs(i % 8 - j % 8), abs(i / 8 - j / 8));
    }
  }

  for(int i = 0; i < 8; i++)
    fileMask[i] = rankMask[i] = 0;

  /// importance of a capture

  for(int i = PAWN; i <= KING; i++) {
    for(int j = PAWN; j <= KING; j++)
      captureValue[i][j] = j * 100 - i;
  }

  /// mask for every file and rank
  /// mask squares between 2 squares

  for(int file = 0; file < 8; file++) {
    for(int rank = 0; rank < 8; rank++) {
      fileMask[file] |= (1ULL << getSq(rank, file)), rankMask[rank] |= (1ULL << getSq(rank, file));
      for(int i = 0; i < 8; i++) {
        int r = rank, f = file;
        uint64_t mask = 0;
        while(true) {
          r += kingDir[i].first, f += kingDir[i].second;
          if(!inTable(r, f))
            break;
          between[getSq(rank, file)][getSq(r, f)] = mask;
          int x = r, y = f, d = (i < 4 ? (i + 2) % 4 : 11 - i);
          uint64_t mask2 = 0;
          while(inTable(x, y)) {
            mask2 |= (1ULL << getSq(x, y));
            x += kingDir[i].first, y += kingDir[i].second;
          }
          x = rank, y = file;
          while(inTable(x, y)) {
            mask2 |= (1ULL << getSq(x, y));
            x += kingDir[d].first, y += kingDir[d].second;
          }
          Line[getSq(rank, file)][getSq(r, f)] = mask | mask2;
          //cout << "Line for " << getSq(rank, file) << " and " << getSq(r, f) << "\n";
          //printBB(mask | mask2);
          mask |= (1ULL << getSq(r, f));
        }
      }
    }
  }

  /// flank mask if we are on a file
  uint64_t queenSide = fileMask[0] | fileMask[1] | fileMask[2] | fileMask[3], kingSide = fileMask[4] | fileMask[5] | fileMask[6] | fileMask[7];

  flankMask[0] = queenSide ^ fileMask[3];
  flankMask[1] = flankMask[2] = queenSide;
  flankMask[3] = flankMask[4] = fileMask[3] | fileMask[4] | fileMask[5] | fileMask[6];
  flankMask[5] = flankMask[6] = kingSide;
  flankMask[7] = kingSide ^ fileMask[7];

  /// generate all squares that are above and below a square

  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int sq = getSq(rank, file);
      neighFileDownMask[sq] = neighFilesMask[sq] = neighFileUpMask[sq] = 0;
      fileUpMask[sq] = fileDownMask[sq] = 0;
      for(int r = 0; r < 8; r++) {
        for(int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
          int sq2 = getSq(r, f);
          if(r > rank)
            neighFileUpMask[sq] |= (1ULL << sq2);
          if(r < rank)
            neighFileDownMask[sq] |= (1ULL << sq2);
          if(file == f) {
            if(r > rank)
              fileUpMask[sq] |= (1ULL << sq2);
            if(r < rank)
              fileDownMask[sq] |= (1ULL << sq2);
          }
          if(f != file)
            neighFilesMask[sq] |= (1ULL << sq2);
        }
      }
    }
  }

}
