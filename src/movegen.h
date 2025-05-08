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
{ /// assuming move is at least pseudo-legal
    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(from), piece_cap = piece_at(to);

    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position before make move " << move.to_string(chess960) << "\n";
    //     exit(0);
    // }

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
        Square rFrom, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);

        if (to > from)
        { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else
        { // queen side castle
            rFrom = to;
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
    {
        key() ^= castle_rights_key(state->rook_sq) ^ castle_rights_key(state->prev->rook_sq);
    }

    turn ^= 1;
    ply++;
    game_ply++;
    key() ^= 1;
    if (turn == WHITE)
        move_index()++;
    get_pinned_pieces_and_checkers();
    get_threats(turn);

    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position after make move " << move.to_string(chess960) << "\n";
    //     // std::cerr << "Previous FEN: " << previous_fen << "\n";
    //     exit(0);
    // }
}

void Board::undo_move(const Move move)
{
    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position before undo move " << move.to_string(chess960) << "\n";
    //     exit(0);
    // }
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
        Square rFrom, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);
        piece = Piece(PieceTypes::KING, turn);

        if (to > from)
        { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else
        { // queen side castle
            rFrom = to;
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

    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position after undo move " << move.to_string(chess960) << "\n";
    //     exit(0);
    // }
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

    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position after make null move\n";
    //     exit(0);
    // }
}

void Board::undo_null_move()
{
    turn ^= 1;
    ply--;
    game_ply--;

    state = state->prev;

    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position after undo null move\n";
    //     exit(0);
    // }
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

template <int movegen_type> int Board::gen_legal_moves(MoveList &moves)
{
    // if (!sanity_check())
    // {
    //     print();
    //     std::cerr << "Illegal position\n";
    //     exit(0);
    // }
    constexpr bool noisy_movegen = movegen_type & MOVEGEN_NOISY;
    constexpr bool quiet_movegen = movegen_type & MOVEGEN_QUIET;

    int nrMoves = 0;
    const bool color = turn, enemy = color ^ 1;
    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
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

    Bitboard notPinned = ~pinned, capMask(0ull), quietMask(0ull);
    int cnt = checkers().count();

    if (cnt == 2)
    { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1)
    { /// one check
        Square sq = checkers().get_lsb_square();
        const Piece pt = piece_type_at(sq);

        if constexpr (noisy_movegen)
        {
            /// make en passant to cancel the check
            if (pt == PieceTypes::PAWN && enpas() != NO_SQUARE &&
                checkers() == Bitboard(shift_square<NORTH>(enemy, enpas())))
            {
                mask = attacks::pawnAttacksMask[enemy][enpas()] & notPinned & our_pawns;
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
                b2 = attacks::pawnAttacksMask[enemy][ep] & get_bb_piece(PieceTypes::PAWN, color);
                b1 = b2 & notPinned;
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
            if (!chess960)
            {
                /// castle queen side
                if (rook_sq(color, 0) != NO_SQUARE)
                {
                    if (!(attacked & Bitboard(7ULL << (king - 2))) && !(all & Bitboard(7ULL << (king - 3))))
                    {
                        moves[nrMoves++] = Move(king, king - 4, MoveTypes::CASTLE);
                    }
                }
                /// castle king side
                if (rook_sq(color, 1) != NO_SQUARE)
                {
                    if (!(attacked & Bitboard(7ULL << king)) && !(all & Bitboard(3ULL << (king + 1))))
                    {
                        moves[nrMoves++] = Move(king, king + 3, MoveTypes::CASTLE);
                    }
                }
            }
            else
            {
                if (rook_sq(color, 0) != NO_SQUARE)
                {
                    Square kingTo = Squares::C1.mirror(color), rook = rook_sq(color, 0),
                           rookTo = Squares::D1.mirror(color);
                    if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                        (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) ||
                         king == kingTo) &&
                        (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) ||
                         rook == rookTo) &&
                        !get_attackers(enemy, all ^ Bitboard(rook), king))
                    {
                        moves[nrMoves++] = Move(king, rook, MoveTypes::CASTLE);
                    }
                }
                /// castle king side
                if (rook_sq(color, 1) != NO_SQUARE)
                {
                    Square kingTo = Squares::G1.mirror(color), rook = rook_sq(color, 1),
                           rookTo = Squares::F1.mirror(color);
                    if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                        (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) ||
                         king == kingTo) &&
                        (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) ||
                         rook == rookTo) &&
                        !get_attackers(enemy, all ^ Bitboard(rook), king))
                    {
                        moves[nrMoves++] = Move(king, rook, MoveTypes::CASTLE);
                    }
                }
            }
        }

        /// pinned pawns

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
                b2 = Bitboard(shift_square<NORTH>(color, sq)) & quietMask & line_mask[king][sq];
                if (b2)
                {
                    const Square sq2 = b2.get_lsb_square();
                    moves[nrMoves++] = Move(sq, sq2, NO_TYPE);

                    /// double pawn push
                    b3 = Bitboard(shift_square<NORTH>(color, sq2)) & quietMask & line_mask[king][sq];
                    if (b3 && sq2 / 8 == rank3)
                    {
                        moves[nrMoves++] = Move(sq, b3.get_lsb_square(), NO_TYPE);
                    }
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

    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = our_pawns & notPinned & ~rank_mask[rank7];

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
            Square sq = b3.get_square_pop(), sq2 = shift_square<SOUTH>(color, sq);
            moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq2), sq, NO_TYPE);
        }
    }

    if constexpr (noisy_movegen)
    {
        b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
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

        b1 = our_pawns & notPinned & rank_mask[rank7];
        b2 = shift_mask<NORTH>(color, b1) & quietMask;
        while (b2)
        {
            Square sq = b2.get_square_pop();
            add_promotions(moves, nrMoves, shift_square<SOUTH>(color, sq), sq);
        }

        b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
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

bool isNoisyMove(Board &board, const Move move)
{
    return move.is_promo() || move.get_type() == MoveTypes::ENPASSANT || board.is_capture(move);
}

bool is_pseudo_legal(Board &board, Move move)
{
    if (!move)
        return false;

    const Square from = move.get_from(), to = move.get_to();
    const int t = move.get_type();
    const Piece pt = board.piece_type_at(from);
    const bool color = board.turn;
    const Bitboard own = board.get_bb_color(color), enemy = board.get_bb_color(1 ^ color);
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

        /// enpassant
        if (t == MoveTypes::ENPASSANT)
            return to == board.enpas() && att.has_square(to);

        Bitboard push = shift_mask<NORTH>(color, Bitboard(from)) & ~occ;
        const int rank_to = to / 8;

        /// promotion
        if (move.is_promo())
            return (rank_to == 0 || rank_to == 7) && ((att & enemy) | push).has_square(to);

        /// add double push to mask
        if (from / 8 == 1 || from / 8 == 6)
            push |= shift_mask<NORTH>(color, push) & ~occ;

        return (rank_to != 0 && rank_to != 7) && t == NO_TYPE && ((att & enemy) | push).has_square(to);
    }

    if (t != NO_TYPE)
        return false;

    /// check for normal moves
    if (pt != PieceTypes::KING)
        return attacks::genAttacksSq(occ, from, pt).has_square(to);

    return attacks::kingBBAttacks[from].has_square(to);
}

bool is_legal_slow(Board &board, Move move)
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

bool is_legal(Board &board, Move move)
{
    if (!is_pseudo_legal(board, move))
        return 0;

    const bool us = board.turn, enemy = 1 ^ us;
    const Square king = board.get_king(us);
    Square from = move.get_from(), to = move.get_to();
    const Bitboard all = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);

    if (move.get_type() == MoveTypes::CASTLE)
    {
        if (from != king || board.checkers())
            return 0;
        bool side = (to > from); /// queen side or king side

        if (board.rook_sq(us, side) != NO_SQUARE)
        { /// can i castle
            const Square rFrom = to, rTo = (side ? Squares::F1 : Squares::D1).mirror(us);
            to = (side ? Squares::G1 : Squares::C1).mirror(us);
            Bitboard mask = between_mask[from][to] | Bitboard(to);

            while (mask)
            {
                if (board.is_attacked_by(enemy, mask.get_square_pop()))
                    return 0;
            }
            if (!board.chess960)
            {
                if (!side)
                {
                    return !(all & Bitboard(7ULL << (from - 3)));
                }
                return !(all & Bitboard(3ULL << (from + 1)));
            }
            if ((!((all ^ Bitboard(rFrom)) & (between_mask[from][to] | Bitboard(to))) || from == to) &&
                (!((all ^ Bitboard(from)) & (between_mask[rFrom][rTo] | Bitboard(rTo))) || rFrom == rTo))
            {
                return !board.get_attackers(enemy, all ^ Bitboard(rFrom), from);
            }
            return 0;
        }

        return 0;
    }

    if (move.get_type() == MoveTypes::ENPASSANT)
    {
        const Square cap = shift_square<SOUTH>(us, to);
        const Bitboard all_no_move = all ^ Bitboard(from) ^ Bitboard(to) ^ Bitboard(cap);

        return !(attacks::genAttacksBishop(all_no_move, king) & board.diagonal_sliders(enemy)) &&
               !(attacks::genAttacksRook(all_no_move, king) & board.orthogonal_sliders(enemy));
    }

    if (board.piece_type_at(from) == PieceTypes::KING)
        return !board.get_attackers(enemy, all ^ Bitboard(from), to);

    bool notInCheck = !board.pinned_pieces().has_square(from) || between_mask[king][to].has_square(from) ||
                      between_mask[king][from].has_square(to);
    if (!notInCheck)
        return 0;

    if (!board.checkers())
        return 1;

    return board.checkers().count() == 2
               ? false
               : (board.checkers() | between_mask[king][board.checkers().get_lsb_square()]).has_square(to);
}

bool is_legal_dummy(Board &board, Move move)
{
    if (!is_pseudo_legal(board, move))
        return 0;
    if (move.get_type() == MoveTypes::CASTLE)
        return is_legal(board, move);
    bool legal = false;

    HistoricalState next_state;
    board.make_move(move, next_state);
    legal = !board.is_attacked_by(board.turn, board.get_king(board.turn ^ 1));
    board.undo_move(move);
    if (legal != is_legal(board, move))
    {
        board.print();
        std::cout << move.to_string(board.chess960) << " " << legal << " " << is_legal_slow(board, move) << " "
                  << is_legal(board, move) << "\n";
        exit(0);
    }
    return legal;
}

Move parse_move_string(Board &board, std::string moveStr, Info &info)
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
            if (move.is_promo())
            {
                int prom = move.get_prom() + PieceTypes::KNIGHT;
                if (prom == PieceTypes::ROOK && moveStr[4] == 'r')
                {
                    return move;
                }
                else if (prom == PieceTypes::BISHOP && moveStr[4] == 'b')
                {
                    return move;
                }
                else if (prom == PieceTypes::QUEEN && moveStr[4] == 'q')
                {
                    return move;
                }
                else if (prom == PieceTypes::KNIGHT && moveStr[4] == 'n')
                {
                    return move;
                }
                continue;
            }
            return move;
        }
    }

    return NULLMOVE;
}
