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
    bool turn, chess960;

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
    Key &pawn_key()
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
    Bitboard &checkers()
    {
        return state->checkers;
    }
    Bitboard &pinned_pieces()
    {
        return state->pinnedPieces;
    }
    Square &enpas()
    {
        return state->enPas;
    }
    uint16_t &half_moves()
    {
        return state->halfMoves;
    }
    uint16_t &move_index()
    {
        return state->moveIndex;
    }
    Piece &captured()
    {
        return state->captured;
    }
    Threats &threats()
    {
        return state->threats;
    }
    Square &rook_sq(bool color, bool side)
    {
        return state->rook_sq[color][side];
    }

    const Bitboard get_bb_color(const bool color) const
    {
        return pieces[color];
    }
    const Bitboard get_bb_piece(const Piece piece_type, const bool color) const
    {
        return bb[6 * color + piece_type];
    }
    const Bitboard get_bb_piece_type(const Piece piece_type) const
    {
        return get_bb_piece(piece_type, WHITE) | get_bb_piece(piece_type, BLACK);
    }

    const Bitboard diagonal_sliders(const bool color) const
    {
        return get_bb_piece(PieceTypes::BISHOP, color) | get_bb_piece(PieceTypes::QUEEN, color);
    }
    const Bitboard orthogonal_sliders(const bool color) const
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

    const int get_output_bucket() const
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

    Bitboard get_pawn_attacks(const bool color)
    {
        const Bitboard b = get_bb_piece(PieceTypes::PAWN, color);
        return shift_mask<NORTHWEST>(color, b & not_edge_mask[!color]) |
               shift_mask<NORTHEAST>(color, b & not_edge_mask[color]);
    }

    const Piece piece_at(const Square sq) const
    {
        return board[sq];
    }
    const Piece piece_type_at(const Square sq) const
    {
        return piece_at(sq).type();
    }
    const Piece get_captured_type(const Move move) const
    {
        return move.get_type() == MoveTypes::ENPASSANT ? PieceTypes::PAWN : piece_type_at(move.get_to());
    }

    const Square get_king(const bool color) const
    {
        return get_bb_piece(PieceTypes::KING, color).get_lsb_square();
    }

    const bool is_capture(const Move move) const
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

    template <int movegen_type> int gen_legal_moves(MoveList &moves);

    const bool has_non_pawn_material(const bool color) const
    {
        return (get_bb_piece(PieceTypes::KING, color) ^ get_bb_piece(PieceTypes::PAWN, color)) != get_bb_color(color);
    }

    NetInput to_netinput()
    {
        NetInput ans;
        for (auto color : {WHITE, BLACK})
        {
            for (auto c : {WHITE, BLACK})
            {
                for (Piece pt = PieceTypes::PAWN; pt <= PieceTypes::KING; pt++)
                {
                    Bitboard b = get_bb_piece(pt, c);
                    while (b)
                    {
                        ans.ind[color].push_back(net_index(Piece(pt, c), b.get_lsb_square(), get_king(color), color));
                        b ^= b.lsb();
                    }
                }
            }
        }

        return ans;
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

    Key speculative_next_key(const Move move)
    {
        const int from = move.get_from(), to = move.get_to();
        const Piece piece = piece_at(from);
        return key() ^ hashKey[piece][from] ^ hashKey[piece][to] ^
               (piece_at(to) != NO_PIECE ? hashKey[piece_at(to)][to] : 0) ^ 1;
    }

    const bool is_material_draw() const
    {
        /// KvK, KBvK, KNvK, KNNvK
        const int num = (get_bb_color(WHITE) | get_bb_color(BLACK)).count();
        return num == 2 ||
               (num == 3 && (get_bb_piece_type(PieceTypes::BISHOP) || get_bb_piece_type(PieceTypes::KNIGHT))) ||
               (num == 4 && (get_bb_piece(PieceTypes::KNIGHT, WHITE).count() == 2 ||
                             get_bb_piece(PieceTypes::KNIGHT, BLACK).count() == 2));
    }

    bool is_repetition(const int ply)
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

    bool is_draw(const int ply)
    {
        if (half_moves() < 100 || !checkers())
            return is_material_draw() || is_repetition(ply) || half_moves() >= 100;
        MoveList moves;
        return gen_legal_moves<MOVEGEN_ALL>(moves) > 0;
    }

    bool has_upcoming_repetition(const int ply)
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