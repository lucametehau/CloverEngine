#pragma once
#include <cstdint>
#include <array>
#include "piece.h"
#include "square.h"

typedef uint8_t MoveType;

enum MoveTypes : MoveType {
    NO_TYPE = 0, CASTLE, ENPASSANT,
    KNIGHT_PROMO = 4, BISHOP_PROMO, ROOK_PROMO, QUEEN_PROMO
};

class Move {
private:
    uint16_t move;

public:
    constexpr Move() = default;
    constexpr Move(Square from, Square to, MoveType type) : move(from | (to << 6) | (type << 12)) {}

    constexpr operator bool() const { return move != 0; }

    Square get_from() const { return Square(move & 63); }
    Square get_to() const { return Square((move >> 6) & 63); }
    int get_from_to() const { return move & 4095; }
    bool is_promo() const { return (move >> 12) & 4; }
    Piece get_prom() const { return (move >> 12) & 3; }
    MoveType get_type() const { return (move >> 12) & 7; }
    Square get_special_to() const { return get_type() != MoveTypes::CASTLE ? get_to() : 
                                    static_cast<Square>(8 * (get_from() / 8) + (get_from() < get_to() ? Squares::G1 : Squares::C1)); }

    bool operator == (const Move other) const { return move == other.move; } 

    std::string to_string(const bool chess960 = false) {
        const Square sq_to = !chess960 ? get_special_to() : get_to();
        std::string ans;
        ans += char((get_from() & 7) + 'a');
        ans += char((get_from() >> 3) + '1');
        ans += char((sq_to & 7) + 'a');
        ans += char((sq_to >> 3) + '1');
        if (is_promo())
            ans += piece_char[get_prom() + Pieces::BlackKnight];
        return ans;
    }
};

constexpr Move NULLMOVE = Move();

constexpr int MAX_MOVES = 256;
typedef std::array<Move, MAX_MOVES> MoveList;