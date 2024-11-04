#pragma once
#include "square.h"
#include <iostream>
#include <cassert>
#include <array>


class Bitboard {
private:
    unsigned long long bb;

public:
    constexpr Bitboard(unsigned long long _bb = 0) : bb(_bb) {}
    Bitboard(Square sq) : bb(1ULL << sq) { assert(sq < NO_SQUARE); }

    bool has_square(Square sq) const { return (bb >> sq) & 1; }
    Square get_msb_square() const { return 63 - __builtin_clzll(bb); }
    Square get_lsb_square() const { return __builtin_ctzll(bb); }
    Bitboard lsb() const { return bb & -bb; }
    operator unsigned long long() const { return bb; } 

    Square get_square_pop() {
        const Square sq = get_lsb_square();
        bb &= bb - 1;
        return sq;
    }

    int count() const { return __builtin_popcountll(bb); }

    Bitboard operator | (const Bitboard &other) const { return bb | other.bb; }
    Bitboard operator & (const Bitboard &other) const { return bb & other.bb; }
    Bitboard operator ^ (const Bitboard &other) const { return bb ^ other.bb; }
    Bitboard operator << (const int8_t shift) const { return bb << shift; }
    Bitboard operator >> (const int8_t shift) const { return bb >> shift; }
    Bitboard operator ~ () const { return ~bb; }

    Bitboard& operator |= (const Bitboard &other) { bb |= other.bb; return *this; }
    Bitboard& operator &= (const Bitboard &other) { bb &= other.bb; return *this; }
    Bitboard& operator ^= (const Bitboard &other) { bb ^= other.bb; return *this; }

    void print() {
        Bitboard copy = bb;
        while (copy) {
            std::cout << int(copy.get_square_pop()) << " ";
        }
        std::cout << " mask\n";
    }
};

inline constexpr Bitboard ALL = 18446744073709551615ULL;

inline constexpr std::array<Bitboard, 8> file_mask = {
    72340172838076673ull, 144680345676153346ull, 289360691352306692ull, 578721382704613384ull, 1157442765409226768ull, 2314885530818453536ull, 4629771061636907072ull, 9259542123273814144ull
};
inline constexpr std::array<Bitboard, 8> rank_mask = {
    255ull, 65280ull, 16711680ull, 4278190080ull, 1095216660480ull, 280375465082880ull, 71776119061217280ull, 18374686479671623680ull
};