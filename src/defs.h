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
#include "params.h"

#if defined(__ARM_NEON)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

std::mt19937_64 gen(0xBEEF);
std::uniform_int_distribution <uint64_t> rng;

template<typename T, std::size_t size, std::size_t... sizes>
struct MultiArray_impl {
    using type = std::array<typename MultiArray_impl<T, sizes...>::type, size>;
};

template<typename T, std::size_t size>
struct MultiArray_impl<T, size> {
    using type = std::array<T, size>;
};

template<typename T, std::size_t... sizes>
using MultiArray = typename MultiArray_impl<T, sizes...>::type;


template<typename T, std::size_t size>
void fill_multiarray(MultiArray<T, size> &array, T value) {
    array.fill(value);
}

template<typename T, std::size_t size, std::size_t... sizes>
void fill_multiarray(MultiArray<typename MultiArray_impl<T, sizes...>::type, size> &array, T value) {
    for(std::size_t i = 0; i < size; i++)
        fill_multiarray<T, sizes...>(array[i], value);
}

typedef uint16_t Move;

class Threats {
public:
    uint64_t threats_pieces[4];
    uint64_t all_threats;
    uint64_t threatened_pieces;

    Threats() {
        for(int i = 0; i < 4; i++) threats_pieces[i] = 0;
    }
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
    PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE
};

enum {
    BP = 0, BN, BB, BR, BQ, BK,
    WP = 6, WN, WB, WR, WQ, WK,
    NO_PIECE
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

constexpr Move NULLMOVE = 0;
constexpr int HALFMOVES = 100;
constexpr int INF = 32000;
constexpr int VALUE_NONE = INF + 10;
constexpr int MATE = 31000;
constexpr int TB_WIN_SCORE = 22000;
constexpr int MAX_DEPTH = 200;
constexpr int MAX_MOVES = 256;
constexpr uint64_t ALL = 18446744073709551615ULL;
const std::string piece_char = "pnbrqkPNBRQK.";
const std::string START_POS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::map<char, int> cod;
MultiArray<uint64_t, 12, 64> hashKey;
MultiArray<uint64_t, 2, 2> castleKey;
std::array<uint64_t, 64> enPasKey;
std::array<uint64_t, 16> castleKeyModifier;
constexpr std::array<uint64_t, 8> file_mask = {
    72340172838076673ull, 144680345676153346ull, 289360691352306692ull, 578721382704613384ull, 1157442765409226768ull, 2314885530818453536ull, 4629771061636907072ull, 9259542123273814144ull
};
constexpr std::array<uint64_t, 8> rank_mask = {
    255ull, 65280ull, 16711680ull, 4278190080ull, 1095216660480ull, 280375465082880ull, 71776119061217280ull, 18374686479671623680ull
};
MultiArray<uint64_t, 64, 64> between_mask, line_mask;

MultiArray<int, 64, 64> lmr_red, lmr_red_noisy;

constexpr std::pair<int, int> knightDir[8] = { {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}, {1, 2}, {2, 1}, {2, -1}, {1, -2} };
constexpr std::pair<int, int> rookDir[4] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
constexpr std::pair<int, int> bishopDir[4] = { {-1, 1}, {-1, -1}, {1, -1}, {1, 1} };
constexpr std::pair<int, int> kingDir[8] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
constexpr std::pair<int, int> pawnCapDirWhite[2] = { {1, -1}, {1, 1} };
constexpr std::pair<int, int> pawnCapDirBlack[2] = { {-1, -1}, {-1, 1} };

std::array<int, 8> deltaPos; /// how does my position change when moving in direction D

constexpr std::array<int, 64> kingIndTable = {
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 4, 5, 5, 5, 5, 4, 4,
    4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
};

class MeanValue {
private:
    std::string name;
    double valuesSum;
    int valuesCount;

public:
    void init(std::string _name) {
        name = _name;
        valuesSum = 0.0;
        valuesCount = 0;
    }

    void upd(double value) {
        valuesSum += value;
        valuesCount++;
    }

    void print_mean() {
        std::cout << name << " has the mean value " << valuesSum / valuesCount << "\n";//(" << valuesCount << " calls)\n";
    }
};

const auto t_init = std::chrono::steady_clock::now();

inline int64_t getTime() {
    auto t = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t - t_init).count();
}

inline uint64_t mul_hi(const uint64_t a, const uint64_t b) {
    using uint128_t = unsigned __int128;
    return (static_cast<uint128_t>(a) * static_cast<uint128_t>(b)) >> 64;
}

inline uint64_t lsb(uint64_t nr) {
    return nr & -nr;
}

inline int Sq(const uint64_t b) {
    return 63 - __builtin_clzll(b);
}

inline int sq_single_bit(const uint64_t b) {
    return __builtin_ctzll(b);
}

inline int sq_lsb(uint64_t &b) {
    const int sq = sq_single_bit(b);
    b &= b - 1;
    return sq;
}

inline int piece_type(int piece) {
    return piece >= 6 ? piece - 6 : piece;
}

inline int16_t net_index(int piece, int sq, int kingSq, bool side) {
    return 64 * 12 * kingIndTable[kingSq ^ (56 * !side)] + 64 * (piece + side * (piece >= 6 ? -6 : +6)) + (sq ^ (56 * !side) ^ (7 * ((kingSq >> 2) & 1))); // kingSq should be ^7, if kingSq&7 >= 4
}

inline bool recalc(int from, int to, bool side) {
    return (from & 4) != (to & 4) || kingIndTable[from ^ (56 * !side)] != kingIndTable[to ^ (56 * !side)];
}

inline uint32_t count(uint64_t b) {
    return __builtin_popcountll(b);
}

inline int get_sq(int rank, int file) {
    return (rank << 3) | file;
}

inline int mirror(bool color, int sq) {
    return sq ^ (56 * !color);
}

inline int sq_dir(int color, int dir, int sq) {
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
        return mask << deltaPos[dir];

    return mask >> (-deltaPos[dir]);
}

inline int get_piece(const int piece_type, const bool color) {
    return 6 * color + piece_type;
}

inline bool color_of(int piece) {
    return piece >= 6;
}

inline bool inside_board(int rank, int file) {
    return rank >= 0 && file >= 0 && rank <= 7 && file <= 7;
}

inline Move getMove(int from, int to, int prom, int type) {
    return from | (to << 6) | (prom << 12) | (type << 14);
}

inline int sq_from(Move move) {
    return move & 63;
}

inline int sq_to(Move move) {
    return (move & 4095) >> 6;
}

inline int from_to(Move move) {
    return move & 4095;
}

inline int type(Move move) {
    return move >> 14;
}

inline int special_sqto(Move move) {
    return type(move) != CASTLE ? sq_to(move) : 8 * (sq_from(move) / 8) + (sq_from(move) < sq_to(move) ? 6 : 2);
}

inline int promoted(Move move) {
    return (move & 16383) >> 12;
}

inline std::string move_to_string(Move move, bool chess960) {
    int sq1 = sq_from(move), sq2 = (!chess960 ? special_sqto(move) : sq_to(move));
    std::string ans;
    ans += char((sq1 & 7) + 'a');
    ans += char((sq1 >> 3) + '1');
    ans += char((sq2 & 7) + 'a');
    ans += char((sq2 >> 3) + '1');
    if (type(move) == PROMOTION)
        ans += piece_char[((move & 16383) >> 12) + BN];
    return ans;
}

inline void printBB(uint64_t mask) {
    while (mask) {
        std::cout << sq_lsb(mask) << " ";
    }
    std::cout << " mask\n";
}

inline void init_defs() {
    deltaPos[NORTH] = 8, deltaPos[SOUTH] = -8;
    deltaPos[WEST] = -1, deltaPos[EAST] = 1;
    deltaPos[NORTHWEST] = 7, deltaPos[NORTHEAST] = 9;
    deltaPos[SOUTHWEST] = -9, deltaPos[SOUTHEAST] = -7;

    cod['p'] = BP, cod['n'] = BN, cod['b'] = BB, cod['r'] = BR, cod['q'] = BQ, cod['k'] = BK;
    cod['P'] = WP, cod['N'] = WN, cod['B'] = WB, cod['R'] = WR, cod['Q'] = WQ, cod['K'] = WK;

    for(int j= 0; j < 64; j++) {
        int x = rng(gen);
    }
    /// zobrist keys
    for (int i = BP; i <= WQ; i++) {
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

    /// mask squares between 2 squares
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int i = 0; i < 8; i++) {
                int r = rank, f = file;
                uint64_t mask = 0;
                while (true) {
                    r += kingDir[i].first, f += kingDir[i].second;
                    if (!inside_board(r, f))
                        break;
                    between_mask[get_sq(rank, file)][get_sq(r, f)] = mask;
                    int x = r, y = f, d = (i < 4 ? (i + 2) % 4 : 11 - i);
                    uint64_t mask2 = 0;
                    while (inside_board(x, y)) {
                        mask2 |= (1ULL << get_sq(x, y));
                        x += kingDir[i].first, y += kingDir[i].second;
                    }
                    x = rank, y = file;
                    while (inside_board(x, y)) {
                        mask2 |= (1ULL << get_sq(x, y));
                        x += kingDir[d].first, y += kingDir[d].second;
                    }
                    line_mask[get_sq(rank, file)][get_sq(r, f)] = mask | mask2;

                    mask |= (1ULL << get_sq(r, f));
                }
            }
        }
    }

    
    for (int i = 1; i < 64; i++) { /// depth
        for (int j = 1; j < 64; j++) { /// moves played 
            lmr_red[i][j] = LMRQuietBias / 100.0 + log(i) * log(j) / (LMRQuietDiv / 100.0);
            lmr_red_noisy[i][j] = LMRNoisyBias / 100.0 + lmr_red[i][j] / (LMRNoisyDiv / 100.0);
        }
    }
}
