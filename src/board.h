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
#include "attacks.h"
#include "cuckoo.h"
#include "defs.h"

struct Threats
{
    std::array<Bitboard, 4> threats_pieces;
    Bitboard all_threats;
    Bitboard threatened_pieces;
};

struct HistoricalState
{
    Square enPas;
    MultiArray<Square, 2, 2> rook_sq;
    Piece captured;
    uint16_t halfMoves, moveIndex;
    Bitboard checkers, pinnedPieces;
    Key key, pawn_key, mat_key[2];
    Threats threats;

    HistoricalState *prev;
    HistoricalState *next;
};

class Board
{
  public:
    bool turn;

    HistoricalState *state;

    std::array<Piece, 64> board;

    uint16_t ply, game_ply;

    std::array<Bitboard, 12> bb;
    std::array<Bitboard, 2> pieces;

    constexpr Board() = default;

    void clear()
    {
        ply = 0;
    }

    void print()
    {
        for (int i = 7; i >= 0; i--)
        {
            for (Square sq = Square(i, 0); sq < Square(i + 1, 0); sq++)
                std::cerr << piece_char[board[sq]] << " ";
            std::cerr << "\n";
        }
    }

    Key &key()
    {
        return state->key;
    }
    constexpr Key key() const
    {
        return state->key;
    }
    Key &pawn_key()
    {
        return state->pawn_key;
    }
    constexpr Key pawn_key() const
    {
        return state->pawn_key;
    }
    const Key king_pawn_key() const
    {
        return state->pawn_key ^ hashKey[Pieces::WhiteKing][get_king(WHITE)] ^
               hashKey[Pieces::BlackKing][get_king(BLACK)];
    }
    Key &mat_key(const bool color)
    {
        return state->mat_key[color];
    }
    constexpr Key mat_key(const bool color) const
    {
        return state->mat_key[color];
    }
    Bitboard &checkers()
    {
        return state->checkers;
    }
    constexpr Bitboard checkers() const
    {
        return state->checkers;
    }
    Bitboard &pinned_pieces()
    {
        return state->pinnedPieces;
    }
    constexpr Bitboard pinned_pieces() const
    {
        return state->pinnedPieces;
    }
    Square &enpas()
    {
        return state->enPas;
    }
    constexpr Square enpas() const
    {
        return state->enPas;
    }
    uint16_t &half_moves()
    {
        return state->halfMoves;
    }
    constexpr uint16_t half_moves() const
    {
        return state->halfMoves;
    }
    uint16_t &move_index()
    {
        return state->moveIndex;
    }
    constexpr uint16_t move_index() const
    {
        return state->moveIndex;
    }
    Piece &captured()
    {
        return state->captured;
    }
    constexpr Piece captured() const
    {
        return state->captured;
    }
    Threats &threats()
    {
        return state->threats;
    }
    constexpr Threats threats() const
    {
        return state->threats;
    }
    Square &rook_sq(bool color, bool side)
    {
        return state->rook_sq[color][side];
    }
    constexpr Square rook_sq(bool color, bool side) const
    {
        return state->rook_sq[color][side];
    }

    constexpr Bitboard get_bb_color(const bool color) const
    {
        return pieces[color];
    }
    constexpr Bitboard get_bb_piece(const Piece piece_type, const bool color) const
    {
        return bb[6 * color + piece_type];
    }
    constexpr Bitboard get_bb_piece_type(const Piece piece_type) const
    {
        return get_bb_piece(piece_type, WHITE) | get_bb_piece(piece_type, BLACK);
    }

    constexpr Bitboard diagonal_sliders(const bool color) const
    {
        return get_bb_piece(PieceTypes::BISHOP, color) | get_bb_piece(PieceTypes::QUEEN, color);
    }
    constexpr Bitboard orthogonal_sliders(const bool color) const
    {
        return get_bb_piece(PieceTypes::ROOK, color) | get_bb_piece(PieceTypes::QUEEN, color);
    }

    const Bitboard get_attackers(const bool color, const Bitboard blockers, const Square sq) const
    {
        return (attacks::genAttacksPawn(1 ^ color, sq) & get_bb_piece(PieceTypes::PAWN, color)) |
               (attacks::genAttacksKnight(sq) & get_bb_piece(PieceTypes::KNIGHT, color)) |
               (attacks::genAttacksBishop(blockers, sq) & diagonal_sliders(color)) |
               (attacks::genAttacksRook(blockers, sq) & orthogonal_sliders(color)) |
               (attacks::genAttacksKing(sq) & get_bb_piece(PieceTypes::KING, color));
    }

    constexpr int get_output_bucket() const
    {
        const int count = (get_bb_color(WHITE) | get_bb_color(BLACK)).count();
        return (count - 2) / 4;
    }

    void get_pinned_pieces_and_checkers()
    {
        const bool enemy = turn ^ 1;
        const Square king = get_king(turn);
        Bitboard us = pieces[turn], them = pieces[enemy];
        pinned_pieces() = checkers() = Bitboard(0ull);
        Bitboard mask = (attacks::genAttacksRook(them, king) & orthogonal_sliders(enemy)) |
                        (attacks::genAttacksBishop(them, king) & diagonal_sliders(enemy));

        while (mask)
        {
            Square sq = mask.get_square_pop();
            Bitboard b2 = us & between_mask[sq][king];
            if (b2.count() == 1)
                pinned_pieces() |= b2;
            else if (!b2)
                checkers() |= (1ull << sq);
        }

        checkers() |= (attacks::genAttacksKnight(king) & get_bb_piece(PieceTypes::KNIGHT, enemy)) |
                      (attacks::genAttacksPawn(turn, king) & get_bb_piece(PieceTypes::PAWN, enemy));
    }

    void get_threats(const bool color)
    {
        const bool enemy = 1 ^ color;
        Bitboard our_pieces = get_bb_color(color) ^ get_bb_piece(PieceTypes::PAWN, color);
        Bitboard att = get_pawn_attacks(enemy);
        Bitboard threatened_pieces = att & our_pieces;
        Bitboard pieces, att_mask;
        Bitboard all = get_bb_color(WHITE) | get_bb_color(BLACK);

        threats().threats_pieces[PieceTypes::PAWN] = att;
        threats().threats_pieces[PieceTypes::KNIGHT] = 0;
        threats().threats_pieces[PieceTypes::BISHOP] = 0;
        threats().threats_pieces[PieceTypes::ROOK] = 0;
        our_pieces ^= get_bb_piece(PieceTypes::KNIGHT, color) | get_bb_piece(PieceTypes::BISHOP, color);

        pieces = get_bb_piece(PieceTypes::KNIGHT, enemy);
        while (pieces)
        {
            att_mask = attacks::genAttacksKnight(pieces.get_square_pop());
            att |= att_mask;
            threats().threats_pieces[PieceTypes::KNIGHT] |= att_mask;
        }
        threatened_pieces |= att & our_pieces;

        all.toggle_bit(get_king(color));

        pieces = get_bb_piece(PieceTypes::BISHOP, enemy);
        while (pieces)
        {
            att_mask = attacks::genAttacksBishop(all, pieces.get_square_pop());
            att |= att_mask;
            threats().threats_pieces[PieceTypes::BISHOP] |= att_mask;
        }
        threatened_pieces |= att & our_pieces;

        our_pieces ^= get_bb_piece(PieceTypes::ROOK, color);

        pieces = get_bb_piece(PieceTypes::ROOK, enemy);
        while (pieces)
        {
            att_mask = attacks::genAttacksRook(all, pieces.get_square_pop());
            att |= att_mask;
            threats().threats_pieces[PieceTypes::ROOK] |= att_mask;
        }
        threatened_pieces |= att & our_pieces;

        pieces = get_bb_piece(PieceTypes::QUEEN, enemy);
        while (pieces)
            att |= attacks::genAttacksQueen(all, pieces.get_square_pop());

        att |= attacks::genAttacksKing(get_king(enemy));
        threats().all_threats = att;
        threats().threatened_pieces = threatened_pieces;
    }

    constexpr Bitboard get_pawn_attacks(const bool color) const
    {
        const Bitboard b = get_bb_piece(PieceTypes::PAWN, color);
        return shift_mask<NORTHWEST>(color, b & not_edge_mask[!color]) |
               shift_mask<NORTHEAST>(color, b & not_edge_mask[color]);
    }

    constexpr Piece piece_at(const Square sq) const
    {
        return board[sq];
    }
    constexpr Piece piece_type_at(const Square sq) const
    {
        return piece_at(sq).type();
    }
    constexpr Piece get_captured_type(const Move move) const
    {
        return move.get_type() == MoveTypes::ENPASSANT ? PieceTypes::PAWN : piece_type_at(move.get_to());
    }

    constexpr Square get_king(const bool color) const
    {
        return get_bb_piece(PieceTypes::KING, color).get_lsb_square();
    }

    constexpr bool is_capture(const Move move) const
    {
        return move.get_type() != MoveTypes::CASTLE && piece_at(move.get_to()) != NO_PIECE;
    }
    const bool is_noisy_move(const Move move) const
    {
        return (move.get_type() && move.get_type() != MoveTypes::CASTLE) || is_capture(move);
    }

    void make_move(const Move move, HistoricalState &state);
    void undo_move(const Move move);
    void make_null_move(HistoricalState &state);
    void undo_null_move();

    template <int movegen_type> constexpr int gen_legal_moves(MoveList &moves) const;

    constexpr bool is_pseudo_legal(Move move) const
    {
        if (!move)
            return false;

        const Square from = move.get_from(), to = move.get_to();
        const int t = move.get_type();
        const Piece pt = piece_type_at(from);
        const bool color = turn;
        const Bitboard own = get_bb_color(color), enemy = get_bb_color(1 ^ color);
        const Bitboard occ = own | enemy;

        if (!own.has_square(from))
            return false;

        if (t == MoveTypes::CASTLE)
            return true;

        if (own.has_square(to))
            return false;

        if (pt == PieceTypes::PAWN)
        {
            Bitboard att = attacks::pawnAttacksMask[color][from];

            if (t == MoveTypes::ENPASSANT)
                return to == enpas() && att.has_square(to);

            Bitboard push = shift_mask<NORTH>(color, Bitboard(from)) & ~occ;
            const int rank_to = to / 8;

            if (move.is_promo())
                return (rank_to == 0 || rank_to == 7) && ((att & enemy) | push).has_square(to);

            // add double push to mask
            if (from / 8 == 1 || from / 8 == 6)
                push |= shift_mask<NORTH>(color, push) & ~occ;

            return (rank_to != 0 && rank_to != 7) && t == MoveTypes::NO_TYPE && ((att & enemy) | push).has_square(to);
        }

        // check for normal moves
        return t == MoveTypes::NO_TYPE && attacks::genAttacksSq(occ, from, pt).has_square(to);
    }

    constexpr bool is_legal(Move move) const
    {
        if (!is_pseudo_legal(move))
            return false;

        const bool us = turn, enemy = 1 ^ us;
        const Square king = get_king(us);
        Square from = move.get_from(), to = move.get_to();
        const Bitboard all = get_bb_color(WHITE) | get_bb_color(BLACK);

        if (move.get_type() == MoveTypes::CASTLE)
        {
            bool side = to > from;
            if (from != king || to != rook_sq(us, side) || checkers())
                return false;

            const Square rFrom = to, rTo = (side ? Squares::F1 : Squares::D1).mirror(us);
            to = (side ? Squares::G1 : Squares::C1).mirror(us);

            if (threats().all_threats & (between_mask[from][to] | Bitboard(to)))
                return false;

            if ((!((all ^ Bitboard(rFrom)) & (between_mask[from][to] | Bitboard(to))) || from == to) &&
                (!((all ^ Bitboard(from)) & (between_mask[rFrom][rTo] | Bitboard(rTo))) || rFrom == rTo))
            {
                return !pinned_pieces().has_square(rFrom);
            }

            return false;
        }

        if (move.get_type() == MoveTypes::ENPASSANT)
        {
            const Square cap = shift_square<SOUTH>(us, to);
            const Bitboard all_no_move = all ^ Bitboard(from) ^ Bitboard(to) ^ Bitboard(cap);

            return !(attacks::genAttacksBishop(all_no_move, king) & diagonal_sliders(enemy)) &&
                   !(attacks::genAttacksRook(all_no_move, king) & orthogonal_sliders(enemy));
        }

        if (piece_type_at(from) == PieceTypes::KING)
            return !get_attackers(enemy, all ^ Bitboard(from), to);

        bool notInCheck = !pinned_pieces().has_square(from) || between_mask[king][to].has_square(from) ||
                          between_mask[king][from].has_square(to);
        if (!notInCheck)
            return 0;

        if (!checkers())
            return 1;

        return checkers().count() == 2 ? false
                                       : (checkers() | between_mask[king][checkers().get_lsb_square()]).has_square(to);
    }

    constexpr bool has_non_pawn_material(const bool color) const
    {
        return (get_bb_piece(PieceTypes::KING, color) ^ get_bb_piece(PieceTypes::PAWN, color)) != get_bb_color(color);
    }

    void move_from_to(Square from, Square to, const Piece piece)
    {
        const Piece pt = piece.type();
        board[from] = NO_PIECE;
        board[to] = piece;

        const Key hash_delta = hashKey[piece][from] ^ hashKey[piece][to];
        key() ^= hash_delta;
        if (pt == PieceTypes::PAWN)
            pawn_key() ^= hash_delta;
        else
            mat_key(piece.color()) ^= hash_delta;

        pieces[piece.color()] ^= (1ull << from) ^ (1ull << to);
        bb[piece] ^= (1ULL << from) ^ (1ull << to);
    }

    void place_piece_at_sq(Piece piece, Square sq)
    {
        board[sq] = piece;
        key() ^= hashKey[piece][sq];
        if (piece.type() == PieceTypes::PAWN)
            pawn_key() ^= hashKey[piece][sq];
        else
            mat_key(piece.color()) ^= hashKey[piece][sq];

        pieces[piece.color()].set_bit(sq);
        bb[piece].set_bit(sq);
    }

    void erase_square(Square sq)
    {
        const Piece piece = piece_at(sq), pt = piece.type();
        const bool color = piece.color();
        board[sq] = NO_PIECE;

        key() ^= hashKey[piece][sq];
        if (pt == PieceTypes::PAWN)
            pawn_key() ^= hashKey[piece][sq];
        else
        {
            mat_key(color) ^= hashKey[piece][sq];
            if (pt == PieceTypes::ROOK && (sq == rook_sq(color, 0) || sq == rook_sq(color, 1)))
                rook_sq(color, get_king(color) < sq) = NO_SQUARE;
        }

        pieces[color].toggle_bit(sq);
        bb[piece].toggle_bit(sq);
    }

    void set_fen(const std::string fen, HistoricalState &state);
    void set_frc_side(bool color, int idx);
    void set_dfrc(int idx, HistoricalState &state);

    std::string fen();

    constexpr Key speculative_next_key(const Move move) const
    {
        const int from = move.get_from(), to = move.get_to();
        const Piece piece = piece_at(from);
        return key() ^ hashKey[piece][from] ^ hashKey[piece][to] ^
               (piece_at(to) != NO_PIECE ? hashKey[piece_at(to)][to] : 0) ^ 1;
    }

    constexpr bool is_material_draw() const
    {
        /// KvK, KBvK, KNvK, KNNvK
        const int num = (get_bb_color(WHITE) | get_bb_color(BLACK)).count();
        return num == 2 ||
               (num == 3 && (get_bb_piece_type(PieceTypes::BISHOP) || get_bb_piece_type(PieceTypes::KNIGHT))) ||
               (num == 4 && (get_bb_piece(PieceTypes::KNIGHT, WHITE).count() == 2 ||
                             get_bb_piece(PieceTypes::KNIGHT, BLACK).count() == 2));
    }

    constexpr bool is_repetition(const int ply) const
    {
        int cnt = 1;
        HistoricalState *temp_state = state;
        for (int i = 2; i <= game_ply && i <= half_moves(); i += 2)
        {
            temp_state = temp_state->prev->prev;
            if (temp_state->key == key())
            {
                cnt++;
                if (ply > i || cnt == 3)
                    return true;
            }
        }
        return false;
    }

    constexpr bool is_draw(const int ply) const
    {
        if (half_moves() < 100 || !checkers())
            return is_material_draw() || is_repetition(ply) || half_moves() >= 100;
        MoveList moves;
        return gen_legal_moves<MOVEGEN_ALL>(moves) > 0;
    }

    constexpr bool has_upcoming_repetition(const int ply) const
    {
        const Bitboard all_pieces = get_bb_color(WHITE) | get_bb_color(BLACK);
        HistoricalState *temp_state = state->prev;
        Key b = ~(key() ^ temp_state->key);
        for (int i = 3; i <= half_moves() && i <= game_ply; i += 2)
        {
            assert(temp_state->prev->prev);
            temp_state = temp_state->prev->prev;
            b ^= ~(temp_state->next->key ^ temp_state->key);
            if (b)
                continue;
            const Key key_delta = key() ^ temp_state->key;
            int cuckoo_ind = cuckoo::hash1(key_delta);

            if (cuckoo::cuckoo[cuckoo_ind] != key_delta)
                cuckoo_ind = cuckoo::hash2(key_delta);
            if (cuckoo::cuckoo[cuckoo_ind] != key_delta)
                continue;

            const Move move = cuckoo::cuckoo_move[cuckoo_ind];
            const Square from = move.get_from(), to = move.get_to();
            if ((between_mask[from][to] ^ Bitboard(to)) & all_pieces)
                continue;
            if (ply > i)
                return true;
            const Piece piece = piece_at(from) != NO_PIECE ? piece_at(from) : piece_at(to);
            return piece != NO_PIECE && piece.color() == turn;
        }
        return false;
    }
};