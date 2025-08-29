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
#include "board.h"
#include "defs.h"
#include "search-info.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>

void Board::make_move(const Move move, HistoricalState &next_state)
{
    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(from), piece_cap = piece_at(to);

    memcpy(&next_state, state, sizeof(HistoricalState));
    next_state.prev = state;
    state->next = &next_state;
    state = &next_state;
    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    half_moves()++;

    if (piece.type() == PieceTypes::PAWN)
        half_moves() = 0;

    captured() = NO_PIECE;
    enpas() = NO_SQUARE;

    switch (move.get_type())
    {
    case NO_TYPE:
        if (piece_cap != NO_PIECE)
        {
            half_moves() = 0;
            erase_square(to);
        }

        move_from_to(from, to, piece);
        /// moved a castle rook
        if (piece.type() == PieceTypes::ROOK && (from == rook_sq(turn, 0) || from == rook_sq(turn, 1)))
        {
            rook_sq(turn, get_king(turn) < from) = NO_SQUARE;
        }
        else if (piece.type() == PieceTypes::KING)
            rook_sq(turn, 0) = rook_sq(turn, 1) = NO_SQUARE;

        captured() = piece_cap;

        /// double push
        if (piece.type() == PieceTypes::PAWN && (from ^ to) == 16)
        {
            if ((to % 8 && board[to - 1] == Piece(PieceTypes::PAWN, turn ^ 1)) ||
                (to % 8 < 7 && board[to + 1] == Piece(PieceTypes::PAWN, turn ^ 1)))
                enpas() = shift_square<NORTH>(turn, from), key() ^= enPasKey[enpas()];
        }

        break;
    case MoveTypes::ENPASSANT: {
        const Square pos = shift_square<SOUTH>(turn, to);
        half_moves() = 0;
        move_from_to(from, to, piece);
        erase_square(pos);
    }

    break;
    case MoveTypes::CASTLE: {
        Square rFrom = to, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);

        if (to > from)
        {
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else
        {
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        move_from_to(rFrom, rTo, rPiece);
        move_from_to(from, to, piece);

        board[rFrom] = board[from] = NO_PIECE;
        board[to] = piece;
        board[rTo] = rPiece;

        captured() = NO_PIECE;
        rook_sq(turn, 0) = rook_sq(turn, 1) = NO_SQUARE;
    }

    break;
    default: /// promotion
    {
        Piece prom_piece(move.get_prom() + PieceTypes::KNIGHT, turn);

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from);
        bb[prom_piece] ^= Bitboard(to);

        if (piece_cap != NO_PIECE)
            erase_square(to);

        board[from] = NO_PIECE;
        board[to] = prom_piece;
        captured() = piece_cap;

        key() ^= hashKey[piece][from] ^ hashKey[prom_piece][to];
        pawn_key() ^= hashKey[piece][from];
    }

    break;
    }

    if (state->rook_sq != state->prev->rook_sq)
        key() ^= castle_rights_key(state->rook_sq) ^ castle_rights_key(state->prev->rook_sq);

    turn ^= 1;
    ply++;
    game_ply++;
    key() ^= 1;
    move_index() += turn == WHITE;
    get_pinned_pieces_and_checkers();
    get_threats(turn);
}

void Board::undo_move(const Move move)
{
    turn ^= 1;
    ply--;
    game_ply--;
    Piece piece_cap = captured();

    state = state->prev;

    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(to);

    switch (move.get_type())
    {
    case NO_TYPE:
        pieces[turn] ^= (1ull << from) ^ (1ull << to);
        bb[piece] ^= (1ull << from) ^ (1ull << to);

        board[from] = piece;
        board[to] = piece_cap;

        if (piece_cap != NO_PIECE)
        {
            pieces[1 ^ turn] ^= (1ull << to);
            bb[piece_cap] ^= (1ull << to);
        }
        break;
    case MoveTypes::CASTLE: {
        Square rFrom = to, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);
        piece = Piece(PieceTypes::KING, turn);

        if (to > from)
        {
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else
        {
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        pieces[turn] ^= (1ull << from) ^ (1ull << to) ^ (1ull << rFrom) ^ (1ull << rTo);
        bb[piece] ^= (1ull << from) ^ (1ull << to);
        bb[rPiece] ^= (1ull << rFrom) ^ (1ull << rTo);

        board[to] = board[rTo] = NO_PIECE;
        board[from] = piece;
        board[rFrom] = rPiece;
    }
    break;
    case MoveTypes::ENPASSANT: {
        Square pos = shift_square<SOUTH>(turn, to);

        piece_cap = Piece(PieceTypes::PAWN, 1 ^ turn);

        pieces[turn] ^= (1ull << from) ^ (1ull << to);
        bb[piece] ^= (1ull << from) ^ (1ull << to);
        bb[piece_cap] ^= (1ull << pos);

        pieces[1 ^ turn] ^= (1ull << pos);

        board[to] = NO_PIECE;
        board[from] = piece;
        board[pos] = piece_cap;
    }
    break;
    default: /// promotion
    {
        piece = Piece(PieceTypes::PAWN, turn);

        pieces[turn] ^= (1ull << from) ^ (1ull << to);
        bb[piece] ^= (1ull << from);
        bb[Piece(move.get_prom() + PieceTypes::KNIGHT, turn)] ^= (1ull << to);

        board[to] = piece_cap;
        board[from] = piece;

        if (piece_cap != NO_PIECE)
        {
            pieces[1 ^ turn] ^= (1ull << to);
            bb[piece_cap] ^= (1ull << to);
        }
    }
    break;
    }
}

void Board::make_null_move(HistoricalState &next_state)
{
    memcpy(&next_state, state, sizeof(HistoricalState));
    next_state.prev = state;
    state->next = &next_state;
    state = &next_state;

    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    captured() = NO_PIECE;
    enpas() = NO_SQUARE;
    turn ^= 1;
    key() ^= 1;
    get_pinned_pieces_and_checkers();
    get_threats(turn);
    ply++;
    game_ply++;
    half_moves()++;
    move_index()++;
}

void Board::undo_null_move()
{
    turn ^= 1;
    ply--;
    game_ply--;
    state = state->prev;
}

inline void add_moves(MoveList &moves, int &nr_moves, Square pos, Bitboard att)
{
    while (att)
        moves[nr_moves++] = Move(pos, att.get_square_pop(), NO_TYPE);
}

inline void add_promotions(MoveList &moves, int &nr_moves, Square from, Square to)
{
    moves[nr_moves++] = Move(from, to, MoveTypes::KNIGHT_PROMO);
    moves[nr_moves++] = Move(from, to, MoveTypes::BISHOP_PROMO);
    moves[nr_moves++] = Move(from, to, MoveTypes::ROOK_PROMO);
    moves[nr_moves++] = Move(from, to, MoveTypes::QUEEN_PROMO);
}

template <int movegen_type> constexpr int Board::gen_legal_moves(MoveList &moves) const
{
    constexpr bool noisy_movegen = movegen_type & MOVEGEN_NOISY;
    constexpr bool quiet_movegen = movegen_type & MOVEGEN_QUIET;

    int nrMoves = 0;
    const bool color = turn, enemy = color ^ 1;
    const int rank7 = color == WHITE ? 6 : 1, rank3 = color == WHITE ? 2 : 5;
    const Square king = get_king(color);
    Bitboard our_pawns = get_bb_piece(PieceTypes::PAWN, color);
    Bitboard mask, us = get_bb_color(color), them = get_bb_color(enemy);
    Bitboard b, b1, b2, b3;
    Bitboard attacked = threats().all_threats, pinned = pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard all = us | them, empty = ~all;

    b1 = attacks::kingBBAttacks[king] & ~(us | attacked);

    if constexpr (noisy_movegen)
        add_moves(moves, nrMoves, king, b1 & them);
    if constexpr (quiet_movegen)
        add_moves(moves, nrMoves, king, b1 & empty);

    Bitboard capMask(0ull), quietMask(0ull);
    int cnt = checkers().count();

    // double check, only king moves are legal
    if (cnt == 2)
    {
        return nrMoves;
    }
    // only once check
    else if (cnt == 1)
    {
        Square sq = checkers().get_lsb_square();
        const Piece pt = piece_type_at(sq);

        if constexpr (noisy_movegen)
        {
            /// make en passant to cancel the check
            if (pt == PieceTypes::PAWN && enpas() != NO_SQUARE &&
                checkers() == Bitboard(shift_square<NORTH>(enemy, enpas())))
            {
                mask = attacks::pawnAttacksMask[enemy][enpas()] & ~pinned & our_pawns;
                while (mask)
                    moves[nrMoves++] = Move(mask.get_square_pop(), enpas(), MoveTypes::ENPASSANT);
            }
        }
        capMask = checkers();
        if (pt != PieceTypes::KNIGHT)
            quietMask = between_mask[king][sq];
    }
    else
    {
        capMask = them;
        quietMask = empty;

        if constexpr (noisy_movegen)
        {
            if (enpas() != NO_SQUARE)
            {
                Square ep = enpas(), sq2 = shift_square<SOUTH>(color, ep);
                b2 = attacks::pawnAttacksMask[enemy][ep] & our_pawns;
                b1 = b2 & ~pinned;
                while (b1)
                {
                    b = b1.lsb();
                    Square sq = b.get_lsb_square();
                    if (!(attacks::genAttacksRook(all ^ b ^ Bitboard(sq2) ^ Bitboard(ep), king) &
                          orthogonal_sliders(enemy)))
                    {
                        moves[nrMoves++] = Move(sq, ep, MoveTypes::ENPASSANT);
                    }
                    b1 ^= b;
                }
                b1 = b2 & pinned & line_mask[ep][king];
                if (b1)
                    moves[nrMoves++] = Move(b1.get_lsb_square(), ep, MoveTypes::ENPASSANT);
            }
        }

        if constexpr (quiet_movegen)
        {
            // castle king side
            if (rook_sq(color, 0) != NO_SQUARE)
            {
                Square kingTo = Squares::C1.mirror(color), rook = rook_sq(color, 0), rookTo = Squares::D1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !(pinned & Bitboard(rook)))
                {
                    moves[nrMoves++] = Move(king, rook, MoveTypes::CASTLE);
                }
            }
            // castle king side
            if (rook_sq(color, 1) != NO_SQUARE)
            {
                Square kingTo = Squares::G1.mirror(color), rook = rook_sq(color, 1), rookTo = Squares::F1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !(pinned & Bitboard(rook)))
                {
                    moves[nrMoves++] = Move(king, rook, MoveTypes::CASTLE);
                }
            }
        }

        // pinned pawns
        b1 = pinned & our_pawns & ~rank_mask[rank7];
        while (b1)
        {
            Square sq = b1.get_square_pop();
            if constexpr (noisy_movegen)
            {
                b2 = attacks::pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                add_moves(moves, nrMoves, sq, b2);
            }

            if constexpr (quiet_movegen)
            {
                /// single pawn push
                const Square sq_push = shift_square<NORTH>(color, sq);
                if ((quietMask & line_mask[king][sq]).has_square(sq_push))
                {
                    moves[nrMoves++] = Move(sq, sq_push, NO_TYPE);

                    /// double pawn push
                    const Square sq_double_push = shift_square<NORTH>(color, sq_push);
                    if (quietMask.has_square(sq_double_push) && sq_push / 8 == rank3)
                        moves[nrMoves++] = Move(sq, sq_double_push, NO_TYPE);
                }
            }
        }
        if constexpr (noisy_movegen)
        {
            b1 = pinned & our_pawns & rank_mask[rank7];
            while (b1)
            {
                Square sq = b1.get_square_pop();
                b2 = attacks::pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                while (b2)
                {
                    Square sq2 = b2.get_square_pop();
                    add_promotions(moves, nrMoves, sq, sq2);
                }
            }
        }
    }

    Bitboard mobMask(0ull);
    if constexpr (noisy_movegen)
        mobMask |= capMask;
    if constexpr (quiet_movegen)
        mobMask |= quietMask;

    mask = get_bb_piece(PieceTypes::KNIGHT, color) & ~pinned;
    while (mask)
    {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksKnight(sq) & mobMask);
    }

    mask = diagonal_sliders(color);
    while (mask)
    {
        Square sq = mask.get_square_pop();
        Bitboard attacks = attacks::genAttacksBishop(all, sq) & mobMask;
        if (pinned.has_square(sq))
            attacks &= line_mask[king][sq];
        add_moves(moves, nrMoves, sq, attacks);
    }

    mask = orthogonal_sliders(color);
    while (mask)
    {
        Square sq = mask.get_square_pop();
        Bitboard attacks = attacks::genAttacksRook(all, sq) & mobMask;
        if (pinned.has_square(sq))
            attacks &= line_mask[king][sq];
        add_moves(moves, nrMoves, sq, attacks);
    }

    our_pawns &= ~pinned; /// remove pinned pawns from our pawns
    b1 = our_pawns & ~rank_mask[rank7];

    if constexpr (quiet_movegen)
    {
        b2 = shift_mask<NORTH>(color, b1) & ~all;                         /// single push
        b3 = shift_mask<NORTH>(color, b2 & rank_mask[rank3]) & quietMask; /// double push
        b2 &= quietMask;

        while (b2)
        {
            Square sq = b2.get_square_pop();
            moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq), sq, NO_TYPE);
        }
        while (b3)
        {
            Square sq = b3.get_square_pop();
            moves[nrMoves++] = Move(shift_square<SOUTHSOUTH>(color, sq), sq, NO_TYPE);
        }
    }

    if constexpr (noisy_movegen)
    {
        b2 = shift_mask<NORTHWEST>(color, b1 & not_edge_mask[enemy]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & not_edge_mask[turn]) & capMask;
        /// captures

        while (b2)
        {
            Square sq = b2.get_square_pop();
            moves[nrMoves++] = Move(shift_square<SOUTHEAST>(color, sq), sq, NO_TYPE);
        }
        while (b3)
        {
            Square sq = b3.get_square_pop();
            moves[nrMoves++] = Move(shift_square<SOUTHWEST>(color, sq), sq, NO_TYPE);
        }

        b1 = our_pawns & rank_mask[rank7];
        b2 = shift_mask<NORTH>(color, b1) & quietMask;
        while (b2)
        {
            Square sq = b2.get_square_pop();
            add_promotions(moves, nrMoves, shift_square<SOUTH>(color, sq), sq);
        }

        b2 = shift_mask<NORTHWEST>(color, b1 & not_edge_mask[enemy]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & not_edge_mask[turn]) & capMask;
        while (b2)
        {
            Square sq = b2.get_square_pop();
            add_promotions(moves, nrMoves, shift_square<SOUTHEAST>(color, sq), sq);
        }
        while (b3)
        {
            Square sq = b3.get_square_pop();
            add_promotions(moves, nrMoves, shift_square<SOUTHWEST>(color, sq), sq);
        }
    }

    return nrMoves;
}

constexpr bool is_legal_slow(Board &board, Move move)
{
    MoveList moves;
    int nrMoves = 0;

    nrMoves = board.gen_legal_moves<MOVEGEN_ALL>(moves);

    for (int i = 0; i < nrMoves; i++)
    {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

static Move parse_move_string(Board &board, std::string moveStr, Info &info)
{
    if (moveStr[1] > '8' || moveStr[1] < '1' || moveStr[3] > '8' || moveStr[3] < '1' || moveStr[0] > 'h' ||
        moveStr[0] < 'a' || moveStr[2] > 'h' || moveStr[2] < 'a')
        return NULLMOVE;

    int from = Square(moveStr[1] - '1', moveStr[0] - 'a');
    if (!info.is_chess960() && board.piece_type_at(from) == PieceTypes::KING)
    {
        if (moveStr == "e1c1")
            moveStr = "e1a1";
        else if (moveStr == "e1g1")
            moveStr = "e1h1";
        else if (moveStr == "e8c8")
            moveStr = "e8a8";
        else if (moveStr == "e8g8")
            moveStr = "e8h8";
    }

    Square to = Square(moveStr[3] - '1', moveStr[2] - 'a');

    MoveList moves;
    int nrMoves = board.gen_legal_moves<MOVEGEN_ALL>(moves);

    for (int i = 0; i < nrMoves; i++)
    {
        Move move = moves[i];
        if (move.get_from() == from && move.get_to() == to)
        {
            if (!move.is_promo() || piece_char[move.get_prom() + PieceTypes::KNIGHT] == moveStr[4])
                return move;
        }
    }

    return NULLMOVE;
}
