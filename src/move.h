#pragma once
#include <cstdint>
#include <array>
#include "piece.h"
#include "square.h"

typedef uint8_t MoveType;

enum MoveTypes : MoveType {
    NO_TYPE = 0, PROMOTION, CASTLE, ENPASSANT
};

class Move {
private:
    uint16_t move;

public:
    constexpr Move() = default;
    constexpr Move(Square from, Square to, Piece prom, MoveType type) : move(from | (to << 6) | (prom << 12) | (type << 14)) {}

    constexpr operator bool() const { return move != 0; }

    inline Square get_from() const { return move & 63; }
    inline Square get_to() const { return (move >> 6) & 63; }
    inline int get_from_to() const { return move & 4095; }
    inline Piece get_prom() const { return (move >> 12) & 3; }
    inline MoveType get_type() const { return (move >> 14) & 3; }
    inline Square get_special_to() const { return get_type() != MoveTypes::CASTLE ? get_to() : 
                                                                             static_cast<Square>(8 * (get_from() / 8) + (get_from() < get_to() ? Squares::G1 : Squares::C1)); }

    inline bool operator == (const Move other) const { return move == other.move; } 

    std::string to_string(const bool chess960 = false) {
        const Square sq_to = !chess960 ? get_special_to() : get_to();
        std::string ans;
        ans += char((get_from() & 7) + 'a');
        ans += char((get_from() >> 3) + '1');
        ans += char((sq_to & 7) + 'a');
        ans += char((sq_to >> 3) + '1');
        if (get_type() == MoveTypes::PROMOTION)
            ans += piece_char[get_prom() + Pieces::BlackKnight];
        return ans;
    }
};

constexpr Move NULLMOVE = Move();

constexpr int MAX_MOVES = 256;
typedef std::array<Move, MAX_MOVES> MoveList;