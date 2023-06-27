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
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <string>
#include <chrono>
#include <ctime>
#include <ratio>
#include <random>
#include <cassert>
#include <array>
#include <chrono>
#include <map>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

std::mt19937_64 gen(0xBEEF);
std::uniform_int_distribution <uint64_t> rng;

#define TablePieceTo std::array <std::array <int16_t, 64>, 13>

struct StackEntry { /// info to keep in the stack
    uint16_t move, piece;
    uint16_t killer, excluded;
    uint16_t quiets[256], captures[256];
    int eval;
    TablePieceTo* continuationHist;
};

enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};

enum {
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum {
    BP = 1, BN, BB, BR, BQ, BK,
    WP, WN, WB, WR, WQ, WK
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

const int NULLMOVE = 0;
const int HALFMOVES = 100;
const int INF = 32000;
const int MATE = 31000;
const int TB_WIN_SCORE = 22000;
const int DEPTH = 100;
const uint64_t ALL = 18446744073709551615ULL;
const std::string START_POS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::map <char, int> cod;
uint64_t hashKey[13][64], castleKey[2][2], enPasKey[64];
uint64_t castleKeyModifier[16];
char pieceChar[13];
uint64_t fileMask[8], rankMask[8];
uint64_t between[64][64], Line[64][64];

const std::pair <int, int> knightDir[] = { {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}, {1, 2}, {2, 1}, {2, -1}, {1, -2} };
const std::pair <int, int> rookDir[] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
const std::pair <int, int> bishopDir[] = { {-1, 1}, {-1, -1}, {1, -1}, {1, 1} };
const std::pair <int, int> kingDir[] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
const std::pair <int, int> pawnCapDirWhite[] = { {1, -1}, {1, 1} };
const std::pair <int, int> pawnCapDirBlack[] = { {-1, -1}, {-1, 1} };

int deltaPos[8]; /// how does my position change when moving in direction D

int castleRightsDelta[2][64] = {
    {
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        14, 15, 15, 15, 12, 15, 15, 13,
    },
    {
        11, 15, 15, 15,  3, 15, 15,  7,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
    }
};

const int kingIndTable2[64] = {
    3, 2, 1, 0, 0, 1, 2, 3,
    3, 2, 1, 0, 0, 1, 2, 3,
    5, 5, 4, 4, 4, 4, 5, 5,
    5, 5, 4, 4, 4, 4, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
};

const int kingIndTable[64] = {
    1, 1, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
};

struct MeanValue {
    std::string name;
    double valuesSum;
    int valuesCount;

    void init(std::string& _name) {
        name = _name;
        valuesSum = 0.0;
        valuesCount = 0;
    }

    void upd(double value) {
        valuesSum += value;
        valuesCount++;
    }

    void print_mean() {
        std::cout << name << " has the mean value " << valuesSum / valuesCount << "\n";
    }
};

const auto t_init = std::chrono::steady_clock::now();

inline int64_t getTime() {
    auto t = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t - t_init).count();
}

inline uint64_t lsb(uint64_t nr) {
    return nr & -nr;
}

inline int Sq(uint64_t bb) {
    return 63 - __builtin_clzll(bb);
}

int sq_lsb(uint64_t &b) {
    const int sq = Sq(lsb(b));
    b &= b - 1;
    return sq;
}

inline int piece_type(int piece) {
    return (piece > 6 ? piece - 6 : piece);
}

inline int16_t netInd(int piece, int sq, int kingSq, int side) {
    if (side == BLACK) {
        kingSq ^= 56;
        sq ^= 56;
        piece = (piece > 6 ? piece - 6 : piece + 6);
    }
    if ((kingSq & 7) >= 4)
        kingSq ^= 7, sq ^= 7;
    return 5 * 64 * (piece - 1) + 64 * kingIndTable[kingSq] + sq;
}

inline bool recalc(int from, int to, bool side) {
    return (from & 4) != (to & 4) || kingIndTable[from ^ (56 * !side)] != kingIndTable[to ^ (56 * !side)];
}

inline int hashVal(int value, int ply) {
    if (value >= MATE)
        return value - ply;
    else if (value <= -MATE)
        return value + ply;
    return value;
}

inline int count(uint64_t b) {
    return __builtin_popcountll(b);
}

inline int getSq(int rank, int file) {
    return (rank << 3) | file;
}

inline int mirror(bool color, int sq) {
    return sq ^ (56 * !color);
}

inline int mirrorVert(int sq) {
    return (sq % 8 >= 4 ? 7 - sq % 8 : sq % 8) + 8 * (sq / 8);
}

inline int getFrontBit(int color, uint64_t bb) {
    if (!bb)
        return 0;
    return (color == WHITE ? Sq(bb) : __builtin_ctzll(bb));
}

inline int sqDir(int color, int dir, int sq) {
    if (color == BLACK) {
        if (dir < 4)
            dir = (dir + 2) % 4;
        else
            dir = 11 - dir;
    }
    return sq + deltaPos[dir];

}

inline uint64_t shift(int color, int dir, uint64_t mask) {
    if (color == BLACK) {
        if (dir < 4)
            dir = (dir + 2) % 4;
        else
            dir = 11 - dir;
    }
    if (deltaPos[dir] > 0)
        return (mask << deltaPos[dir]);

    return (mask >> (-deltaPos[dir]));
}

inline int getType(int type, int color) {
    return 6 * color + type;
}

inline int color_of(int piece) {
    return piece > 6;
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

inline int fromTo(uint16_t move) {
    return move & 4095;
}

inline int type(uint16_t move) {
    return move >> 14;
}

inline int specialSqTo(uint16_t move) {
    return type(move) != CASTLE ? sqTo(move) : 8 * (sqFrom(move) / 8) + (sqFrom(move) < sqTo(move) ? 6 : 2);
}

inline int promoted(uint16_t move) {
    return (move & 16383) >> 12;
}

inline std::string toString(uint16_t move, bool chess960) {
    int sq1 = sqFrom(move), sq2 = (!chess960 ? specialSqTo(move) : sqTo(move));
    std::string ans;
    ans += char((sq1 & 7) + 'a');
    ans += char((sq1 >> 3) + '1');
    ans += char((sq2 & 7) + 'a');
    ans += char((sq2 >> 3) + '1');
    if (type(move) == PROMOTION)
        ans += pieceChar[((move & 16383) >> 12) + BN];
    return ans;
}

inline void printBB(uint64_t mask) {
    while (mask) {
        uint64_t b = lsb(mask);
        std::cout << Sq(b) << " ";
        mask ^= b;
    }
    std::cout << " mask\n";
}

const int NormalizeToPawnValue = 130;

int winRateModel(int score, int ply) {
    constexpr double as[] = { -0.66398391,   -0.49826081,   39.11399082,   92.97752809 };
    constexpr double bs[] = { -2.53637448,   19.01555550,  -41.44937435,   63.22725557 };
    assert(NormalizeToPawnValue == static_cast<int>(as[0] + as[1] + as[2] + as[3]));
    const double m = std::min<double>(240, ply) / 64.0;
    const double a = ((as[0] * m + as[1]) * m + as[2]) * m + as[3];
    const double b = ((bs[0] * m + bs[1]) * m + bs[2]) * m + bs[3];
    const double x = std::min<double>(std::max<double>(-4000, score), 4000);
    return static_cast<int>(0.5 + 1000.0 / (1.0 + std::exp((a - x) / b)));
}

inline void init_defs() {
    deltaPos[NORTH] = 8, deltaPos[SOUTH] = -8;
    deltaPos[WEST] = -1, deltaPos[EAST] = 1;
    deltaPos[NORTHWEST] = 7, deltaPos[NORTHEAST] = 9;
    deltaPos[SOUTHWEST] = -9, deltaPos[SOUTHEAST] = -7;

    for (int i = 0; i < 256; i++)
        cod[i] = 0;

    cod['p'] = 1, cod['n'] = 2, cod['b'] = 3, cod['r'] = 4, cod['q'] = 5, cod['k'] = 6;
    cod['P'] = 7, cod['N'] = 8, cod['B'] = 9, cod['R'] = 10, cod['Q'] = 11, cod['K'] = 12;

    pieceChar[0] = '.';
    pieceChar[1] = 'p', pieceChar[2] = 'n', pieceChar[3] = 'b', pieceChar[4] = 'r', pieceChar[5] = 'q', pieceChar[6] = 'k';
    pieceChar[7] = 'P', pieceChar[8] = 'N', pieceChar[9] = 'B', pieceChar[10] = 'R', pieceChar[11] = 'Q', pieceChar[12] = 'K';

    /// zobrist keys

    for (int i = 0; i <= 12; i++) {
        for (int j = 0; j < 64; j++)
            hashKey[i][j] = rng(gen);
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++)
            castleKey[i][j] = rng(gen);
    }

    for (int i = 0; i < (1 << 4); i++) {
        for (int j = 0; j < 4; j++) {
            castleKeyModifier[i] ^= castleKey[j / 2][j % 2] * ((i >> j) & 1);
        }
    }

    for (int i = 0; i < 64; i++)
        enPasKey[i] = rng(gen);

    for (int i = 0; i < 8; i++)
        fileMask[i] = rankMask[i] = 0;

    /// mask for every file and rank
    /// mask squares between 2 squares
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            fileMask[file] |= (1ULL << getSq(rank, file)), rankMask[rank] |= (1ULL << getSq(rank, file));
            for (int i = 0; i < 8; i++) {
                int r = rank, f = file;
                uint64_t mask = 0;
                while (true) {
                    r += kingDir[i].first, f += kingDir[i].second;
                    if (!inTable(r, f))
                        break;
                    between[getSq(rank, file)][getSq(r, f)] = mask;
                    int x = r, y = f, d = (i < 4 ? (i + 2) % 4 : 11 - i);
                    uint64_t mask2 = 0;
                    while (inTable(x, y)) {
                        mask2 |= (1ULL << getSq(x, y));
                        x += kingDir[i].first, y += kingDir[i].second;
                    }
                    x = rank, y = file;
                    while (inTable(x, y)) {
                        mask2 |= (1ULL << getSq(x, y));
                        x += kingDir[d].first, y += kingDir[d].second;
                    }
                    Line[getSq(rank, file)][getSq(r, f)] = mask | mask2;

                    mask |= (1ULL << getSq(r, f));
                }
            }
        }
    }
}
