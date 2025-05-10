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
#include "bitboard.h"
#include "move.h"
#include "params.h"
#include "piece.h"
#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <ratio>
#include <string>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

inline std::mt19937_64 gen(0xBEEF);
inline std::uniform_int_distribution<uint64_t> rng;

template <typename T, std::size_t size, std::size_t... sizes> struct MultiArray_impl
{
    using type = std::array<typename MultiArray_impl<T, sizes...>::type, size>;
};

template <typename T, std::size_t size> struct MultiArray_impl<T, size>
{
    using type = std::array<T, size>;
};

template <typename T, std::size_t... sizes> using MultiArray = typename MultiArray_impl<T, sizes...>::type;

template <typename T, std::size_t size> void fill_multiarray(MultiArray<T, size> &array, T value)
{
    array.fill(value);
}

template <typename T, std::size_t size, std::size_t... sizes>
void fill_multiarray(MultiArray<typename MultiArray_impl<T, sizes...>::type, size> &array, T value)
{
    for (std::size_t i = 0; i < size; i++)
        fill_multiarray<T, sizes...>(array[i], value);
}

struct Threats
{
    Bitboard threats_pieces[4];
    Bitboard all_threats;
    Bitboard threatened_pieces;
};

struct NetInput
{
    std::vector<short> ind[2];
};

enum
{
    BLACK = 0,
    WHITE
};

enum
{
    NORTH = 8,
    SOUTH = -8,
    EAST = 1,
    WEST = -1,
    NORTHEAST = 9,
    NORTHWEST = 7,
    SOUTHEAST = -7,
    SOUTHWEST = -9
};

enum
{
    NORTH_ID = 0,
    SOUTH_ID,
    WEST_ID,
    EAST_ID,
    NORTHWEST_ID,
    NORTHEAST_ID,
    SOUTHWEST_ID,
    SOUTHEAST_ID
};

enum
{
    MOVEGEN_NOISY = 1,
    MOVEGEN_QUIET,
    MOVEGEN_ALL
};

constexpr int HALFMOVES = 100;
constexpr int INF = 32000;
constexpr int VALUE_NONE = INF + 10;
constexpr int MATE = 31000;
constexpr int TB_WIN_SCORE = 22000;
constexpr int MAX_DEPTH = 200;
constexpr int STACK_SIZE = MAX_DEPTH + 5;

inline const std::string START_POS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

inline std::map<char, Piece> cod;
inline MultiArray<uint64_t, 12, 64> hashKey;
inline MultiArray<uint64_t, 2, 2> castleKey;
inline std::array<uint64_t, 64> enPasKey;
inline std::array<uint64_t, 16> castleKeyModifier;

inline MultiArray<Bitboard, 64, 64> between_mask, line_mask;

inline MultiArray<int, 64, 64> lmr_red;

constexpr std::pair<int, int> knightDir[8] = {{-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}, {1, 2}, {2, 1}, {2, -1}, {1, -2}};
constexpr std::pair<int, int> rookDir[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
constexpr std::pair<int, int> bishopDir[4] = {{-1, 1}, {-1, -1}, {1, -1}, {1, 1}};
constexpr std::pair<int, int> kingDir[8] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr std::pair<int, int> pawnCapDirWhite[2] = {{1, -1}, {1, 1}};
constexpr std::pair<int, int> pawnCapDirBlack[2] = {{-1, -1}, {-1, 1}};

constexpr std::array<int, 64> kingIndTable = {
    0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
};

class MeanValue
{
  private:
    std::string name;
    double sum;
    int count;

  public:
    MeanValue(std::string _name = "") : name(_name), sum(0.0), count(0)
    {
    }

    void upd(double value)
    {
        sum += value;
        count++;
    }

    void print_mean()
    {
        std::cout << name << " has the mean value " << sum / count << " (" << count << " calls)\n";
    }
};

inline int16_t net_index(Piece piece, Square sq, Square kingSq, bool side)
{
    return 64 * (piece + side * (piece >= 6 ? -6 : +6)) +
           (sq.mirror(side) ^ (7 * ((kingSq >> 2) & 1))); // kingSq should be ^7, if kingSq&7 >= 4
}

inline uint64_t castle_rights_key(MultiArray<Square, 2, 2> &rook_sq)
{
    return (castleKey[BLACK][0] * (rook_sq[BLACK][0] != NO_SQUARE)) ^
           (castleKey[BLACK][1] * (rook_sq[BLACK][1] != NO_SQUARE)) ^
           (castleKey[WHITE][0] * (rook_sq[WHITE][0] != NO_SQUARE)) ^
           (castleKey[WHITE][1] * (rook_sq[WHITE][1] != NO_SQUARE));
}

inline bool recalc(Square from, Square to, bool side)
{
    return (from & 4) != (to & 4) || kingIndTable[from ^ (56 * !side)] != kingIndTable[to ^ (56 * !side)];
}

template <int direction> inline Square shift_square(bool color, Square sq)
{
    return color == BLACK ? sq - direction : sq + direction;
}

template <int8_t direction> inline Bitboard shift_mask(int color, Bitboard bb)
{
    if (color == BLACK)
        return direction > 0 ? bb >> direction : bb << static_cast<int8_t>(-direction);
    return direction > 0 ? bb << direction : bb >> static_cast<int8_t>(-direction);
}

inline bool inside_board(int rank, int file)
{
    return rank >= 0 && file >= 0 && rank <= 7 && file <= 7;
}

inline void init_defs()
{
    cod['p'] = Pieces::BlackPawn, cod['n'] = Pieces::BlackKnight, cod['b'] = Pieces::BlackBishop;
    cod['r'] = Pieces::BlackRook, cod['q'] = Pieces::BlackQueen, cod['k'] = Pieces::BlackKing;
    cod['P'] = Pieces::WhitePawn, cod['N'] = Pieces::WhiteKnight, cod['B'] = Pieces::WhiteBishop;
    cod['R'] = Pieces::WhiteRook, cod['Q'] = Pieces::WhiteQueen, cod['K'] = Pieces::WhiteKing;

    /// zobrist keys
    for (Piece i = Pieces::BlackPawn; i <= Pieces::WhiteKing; i++)
    {
        for (Square j = 0; j < 64; j++)
            hashKey[i][j] = rng(gen);
    }

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
            castleKey[i][j] = rng(gen);
    }

    for (int i = 0; i < (1 << 4); i++)
    {
        for (int j = 0; j < 4; j++)
        {
            castleKeyModifier[i] ^= castleKey[j / 2][j % 2] * ((i >> j) & 1);
        }
    }

    for (int i = 0; i < 64; i++)
        enPasKey[i] = rng(gen);

    /// mask squares between 2 squares
    for (int file = 0; file < 8; file++)
    {
        for (int rank = 0; rank < 8; rank++)
        {
            for (int i = 0; i < 8; i++)
            {
                int r = rank, f = file;
                Bitboard mask(0ull);
                while (true)
                {
                    r += kingDir[i].first, f += kingDir[i].second;
                    if (!inside_board(r, f))
                        break;
                    between_mask[Square(rank, file)][Square(r, f)] = mask;
                    int x = r, y = f, d = (i < 4 ? (i + 2) % 4 : 11 - i);
                    Bitboard mask2(0ull);
                    while (inside_board(x, y))
                    {
                        mask2 |= Bitboard(Square(x, y));
                        x += kingDir[i].first, y += kingDir[i].second;
                    }
                    x = rank, y = file;
                    while (inside_board(x, y))
                    {
                        mask2 |= Bitboard(Square(x, y));
                        x += kingDir[d].first, y += kingDir[d].second;
                    }
                    line_mask[Square(rank, file)][Square(r, f)] = mask | mask2;

                    mask |= Bitboard(Square(r, f));
                }
            }
        }
    }

#ifndef TUNE_FLAG
    for (int i = 1; i < 64; i++)
    { /// depth
        for (int j = 1; j < 64; j++)
        { /// moves played
            lmr_red[i][j] = LMRGrain * (LMRQuietBias + log(i) * log(j) / LMRQuietDiv);
        }
    }
#endif
}
