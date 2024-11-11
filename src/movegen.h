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
#include <cassert>
#include <iomanip>
#include "board.h"
#include "defs.h"
#include "search-info.h"
#include "attacks.h"

void Board::make_move(const Move move) { /// assuming move is at least pseudo-legal
    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(from), piece_cap = piece_at(to);

    history[game_ply] = state;
    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    half_moves()++;

    if (piece.type() == PieceTypes::PAWN)
        half_moves() = 0;

    captured() = NO_PIECE;
    enpas() = NO_SQUARE;

    switch (move.get_type()) {
    case NO_TYPE:
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        if (piece.type() == PieceTypes::PAWN) pawn_key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        else mat_key(turn) ^= hashKey[piece][from] ^ hashKey[piece][to];
        /// moved a castle rook
        if (piece == Pieces::WhiteRook)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == Pieces::BlackRook)
            castle_rights() &= castleRightsDelta[BLACK][from];

        if (piece_cap != NO_PIECE) {
            half_moves() = 0;

            pieces[1 ^ turn] ^= (1ULL << to);
            bb[piece_cap] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            if (piece_cap.type() == PieceTypes::PAWN) pawn_key() ^= hashKey[piece_cap][to];
            else mat_key(1 ^ turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == Pieces::WhiteRook)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == Pieces::BlackRook)
                castle_rights() &= castleRightsDelta[BLACK][to];
        }

        board[from] = NO_PIECE;
        board[to] = piece;
        captured() = piece_cap;

        /// double push
        if (piece.type() == PieceTypes::PAWN && (from ^ to) == 16) {
            if ((to % 8 && board[to - 1] == Piece(PieceTypes::PAWN, turn ^ 1)) || 
                (to % 8 < 7 && board[to + 1] == Piece(PieceTypes::PAWN, turn ^ 1))) 
                enpas() = shift_square<NORTH>(turn, from), key() ^= enPasKey[enpas()];
        }

        /// moved the king
        if (piece == Pieces::WhiteKing) {
            castle_rights() &= castleRightsDelta[WHITE][from];
        }
        else if (piece == Pieces::BlackKing) {
            castle_rights() &= castleRightsDelta[BLACK][from];
        }

        break;
    case MoveTypes::ENPASSANT:
    {
        const Square pos = shift_square<SOUTH>(turn, to);
        piece_cap = Piece(PieceTypes::PAWN, 1 ^ turn);
        half_moves() = 0;
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^ hashKey[piece_cap][pos];
        pawn_key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^ hashKey[piece_cap][pos];

        pieces[1 ^ turn] ^= (1ULL << pos);
        bb[piece_cap] ^= (1ULL << pos);

        board[from] = board[pos] = NO_PIECE;
        board[to] = piece;
    }

    break;
    case MoveTypes::CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else { // queen side castle
            rFrom = to;
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);
        bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^
                 hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];
        mat_key(turn) ^= hashKey[piece][from] ^ hashKey[piece][to] ^
                         hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        board[from] = board[rFrom] = NO_PIECE;
        board[to] = piece;
        board[rTo] = rPiece;
        captured() = NO_PIECE;

        if (piece == Pieces::WhiteKing)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == Pieces::BlackKing)
            castle_rights() &= castleRightsDelta[BLACK][from];
    }

    break;
    default: /// promotion
    {
        Piece prom_piece(move.get_prom() + PieceTypes::KNIGHT, turn);

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from);
        bb[prom_piece] ^= (1ULL << to);

        if (piece_cap != NO_PIECE) {
            bb[piece_cap] ^= (1ULL << to);
            pieces[1 ^ turn] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            mat_key(1 ^ turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == Pieces::WhiteRook)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == Pieces::BlackRook)
                castle_rights() &= castleRightsDelta[BLACK][to];
        }

        board[from] = NO_PIECE;
        board[to] = prom_piece;
        captured() = piece_cap;

        key() ^= hashKey[piece][from] ^ hashKey[prom_piece][to];
        pawn_key() ^= hashKey[piece][from];
    }

    break;
    }

    key() ^= castleKeyModifier[castle_rights() ^ history[game_ply].castleRights];
    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], get_king(1 ^ turn));

    turn ^= 1;
    ply++;
    game_ply++;
    key() ^= 1;
    if (turn == WHITE) move_index()++;

    pinned_pieces() = get_pinned_pieces();
}

void Board::undo_move(const Move move) {
    turn ^= 1;
    ply--;
    game_ply--;
    Piece piece_cap = captured();
    
    state = history[game_ply];

    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(to);

    switch (move.get_type()) {
    case NO_TYPE:
        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);

        board[from] = piece;
        board[to] = piece_cap;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= Bitboard(to);
            bb[piece_cap] ^= Bitboard(to);
        }
        break;
    case MoveTypes::CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece(PieceTypes::ROOK, turn);

        piece = Piece(PieceTypes::KING, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else { // queen side castle
            rFrom = to;
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to) ^ Bitboard(rFrom) ^ Bitboard(rTo);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);
        bb[rPiece] ^= Bitboard(rFrom) ^ Bitboard(rTo);

        board[to] = board[rTo] = NO_PIECE;
        board[from] = piece;
        board[rFrom] = rPiece;
    }
    break;
    case MoveTypes::ENPASSANT:
    {
        Square pos = shift_square<SOUTH>(turn, to);

        piece_cap = Piece(PieceTypes::PAWN, 1 ^ turn);

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);

        pieces[1 ^ turn] ^= Bitboard(pos);
        bb[piece_cap] ^= Bitboard(pos);

        board[to] = NO_PIECE;
        board[from] = piece;
        board[pos] = piece_cap;
    }
    break;
    default: /// promotion
    {
        Piece prom_piece(move.get_prom() + PieceTypes::KNIGHT, turn);

        piece = Piece(PieceTypes::PAWN, turn);

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from);
        bb[prom_piece] ^= Bitboard(to);

        board[to] = piece_cap;
        board[from] = piece;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= Bitboard(to);
            bb[piece_cap] ^= Bitboard(to);
        }
    }
    break;
    }

    captured() = history[game_ply].captured;
}

void Board::make_null_move() {
    history[game_ply] = state;
    
    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], get_king(1 ^ turn));
    captured() = NO_PIECE;
    enpas() = NO_SQUARE;
    turn ^= 1;
    key() ^= 1;
    pinned_pieces() = get_pinned_pieces();
    ply++;
    game_ply++;
    half_moves()++;
    move_index()++;
}

void Board::undo_null_move() {
    turn ^= 1;
    ply--;
    game_ply--;

    state = history[game_ply];
}

inline void add_moves(MoveList &moves, int& nrMoves, Square pos, Bitboard att) {
    while (att) moves[nrMoves++] = Move(pos, att.get_square_pop(), 0, NO_TYPE);
}

int Board::gen_legal_moves(MoveList &moves) {
    int nrMoves = 0;
    const bool color = turn, enemy = color ^ 1;
    const Square king = get_king(color), enemyKing = get_king(enemy);
    Bitboard pieces, mask, us = get_bb_color(color), them = get_bb_color(enemy);
    Bitboard b, b1, b2, b3;
    Bitboard attacked(0ull), pinned = pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = orthogonal_sliders(enemy), enemyDiagSliders = diagonal_sliders(enemy);
    const Bitboard all = us | them, emptySq = ~all;

    attacked |= get_pawn_attacks(enemy);

    pieces = get_bb_piece(PieceTypes::KNIGHT, enemy);
    while (pieces) attacked |= attacks::genAttacksKnight(pieces.get_square_pop());

    pieces = enemyDiagSliders;
    while (pieces) attacked |= attacks::genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

    pieces = enemyOrthSliders;
    while (pieces) attacked |= attacks::genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

    attacked |= attacks::kingBBAttacks[enemyKing];

    b1 = attacks::kingBBAttacks[king] & ~(us | attacked);
    add_moves(moves, nrMoves, king, b1);

    Bitboard notPinned = ~pinned, capMask(0ull), quietMask(0ull);
    int cnt = checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        Square sq = checkers().get_lsb_square();
        const Piece pt = piece_type_at(sq);
        
        if (pt == PieceTypes::PAWN) {
            /// make en passant to cancel the check
            if (enpas() != NO_SQUARE && checkers() == Bitboard(shift_square<NORTH>(enemy, enpas()))) {
                mask = attacks::pawnAttacksMask[enemy][enpas()] & notPinned & get_bb_piece(PieceTypes::PAWN, color);
                while (mask) moves[nrMoves++] = Move(mask.get_square_pop(), enpas(), 0, MoveTypes::ENPASSANT);
            }
        }
        if (pt == PieceTypes::KNIGHT) {
            capMask = checkers();
        }
        else {
            capMask = checkers();
            quietMask = between_mask[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (enpas() != NO_SQUARE) {
            Square ep = enpas(), sq2 = shift_square<SOUTH>(color, ep);
            b2 = attacks::pawnAttacksMask[enemy][ep] & get_bb_piece(PieceTypes::PAWN, color);
            b1 = b2 & notPinned;
            while (b1) {
                b = b1.lsb();
                Square sq = b.get_lsb_square();
                if (!(attacks::genAttacksRook(all ^ b ^ Bitboard(sq2) ^ Bitboard(ep), king) & enemyOrthSliders)) {
                    moves[nrMoves++] =  Move(sq, ep, 0, MoveTypes::ENPASSANT);
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & line_mask[ep][king];
            if (b1) moves[nrMoves++] = Move(b1.get_lsb_square(), ep, 0, MoveTypes::ENPASSANT);
        }

        if (!chess960) {
            /// castle queen side
            if (castle_rights() & (1 << (2 * color))) {
                if (!(attacked & Bitboard(7ULL << (king - 2))) && !(all & Bitboard(7ULL << (king - 3)))) {
                    moves[nrMoves++] = Move(king, king - 4, 0, MoveTypes::CASTLE);
                }
            }
            /// castle king side
            if (castle_rights() & (1 << (2 * color + 1))) {
                if (!(attacked & Bitboard(7ULL << king)) && !(all & Bitboard(3ULL << (king + 1)))) {
                    moves[nrMoves++] = Move(king, king + 3, 0, MoveTypes::CASTLE);
                }
            }
        }
        else {
            if ((castle_rights() >> (2 * color)) & 1) {
                Square kingTo = Squares::C1.mirror(color), rook = rookSq[color][0], rookTo = Squares::D1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = Move(king, rook, 0, MoveTypes::CASTLE);
                }
            }
            /// castle king side
            if ((castle_rights() >> (2 * color + 1)) & 1) {
                Square kingTo = Squares::G1.mirror(color), rook = rookSq[color][1], rookTo = Squares::F1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = Move(king, rook, 0, MoveTypes::CASTLE);
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = ~notPinned & diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = attacks::genAttacksBishop(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & quietMask);
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = ~notPinned & orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = attacks::genAttacksRook(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & quietMask);
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns

        b1 = ~notPinned & get_bb_piece(PieceTypes::PAWN, color);
        while (b1) {
            Square sq = b1.get_square_pop();
            int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = attacks::pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                while (b2) {
                    Square sq2 = b2.get_square_pop();
                    for (int j = 0; j < 4; j++) moves[nrMoves++] = Move(sq, sq2, j, MoveTypes::PROMOTION);
                }
            }
            else {
                b2 = attacks::pawnAttacksMask[color][sq] & them & line_mask[king][sq];
                add_moves(moves, nrMoves, sq, b2);

                /// single pawn push
                b2 = Bitboard(shift_square<NORTH>(color, sq)) & emptySq & line_mask[king][sq];
                if (b2) {
                    const Square sq2 = b2.get_lsb_square();
                    moves[nrMoves++] = Move(sq, sq2, 0, NO_TYPE);

                    /// double pawn push
                    b3 = Bitboard(shift_square<NORTH>(color, sq2)) & emptySq & line_mask[king][sq];
                    if (b3 && sq2 / 8 == rank3) {
                        moves[nrMoves++] = Move(sq, b3.get_lsb_square(), 0, NO_TYPE);
                    }
                }
            }
        }
    }

    /// not pinned pieces (excluding pawns)
    Bitboard mobMask = capMask | quietMask;

    mask = get_bb_piece(PieceTypes::KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksKnight(sq) & mobMask);
    }

    mask = diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksBishop(all, sq) & mobMask);
    }

    mask = orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksRook(all, sq) & mobMask);
    }

    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = get_bb_piece(PieceTypes::PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift_mask<NORTH>(color, b1) & ~all; /// single push
    b3 = shift_mask<NORTH>(color, b2 & rank_mask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq), sq, 0, NO_TYPE);
    }
    while (b3) {
        Square sq = b3.get_square_pop(), sq2 = shift_square<SOUTH>(color, sq);
        moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq2), sq, 0, NO_TYPE);
    }

    b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
    b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
    /// captures

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTHEAST>(color, sq), sq, 0, NO_TYPE);
    }
    while (b3) {
        Square sq = b3.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTHWEST>(color, sq), sq, 0, NO_TYPE);
    }

    b1 = get_bb_piece(PieceTypes::PAWN, color) & notPinned & rank_mask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift_mask<NORTH>(color, b1) & quietMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq), sq, i, MoveTypes::PROMOTION);
        }

        /// capture promotions

        b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTHEAST>(color, sq), sq, i, MoveTypes::PROMOTION);
        }
        while (b3) {
            Square sq = b3.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTHWEST>(color, sq), sq, i, MoveTypes::PROMOTION);
        }
    }

    return nrMoves;
}

/// noisy moves generator

int Board::gen_legal_noisy_moves(MoveList &moves) {
    int nrMoves = 0;
    const bool color = turn, enemy = color ^ 1;
    const Square king = get_king(color), enemyKing = get_king(enemy);
    Bitboard pieces, mask, us = get_bb_color(color), them = get_bb_color(enemy);
    Bitboard b, b1, b2, b3;
    Bitboard attacked(0ull), pinned = pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = orthogonal_sliders(enemy), enemyDiagSliders = diagonal_sliders(enemy);
    const Bitboard all = us | them;

    if (attacks::kingBBAttacks[king] & them) {
        attacked |= get_pawn_attacks(enemy);

        pieces = bb[Piece(PieceTypes::KNIGHT, enemy)];
        while (pieces) attacked |= attacks::genAttacksKnight(pieces.get_square_pop());

        pieces = enemyDiagSliders;
        while (pieces) attacked |= attacks::genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

        pieces = enemyOrthSliders;
        while (pieces) attacked |= attacks::genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

        attacked |= attacks::kingBBAttacks[enemyKing];

        add_moves(moves, nrMoves, king, attacks::kingBBAttacks[king] & ~(us | attacked) & them);
    }

    Bitboard notPinned = ~pinned, capMask(0ull), quietMask(0ull);

    int cnt = checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        Square sq = checkers().get_lsb_square();
        const Piece pt = piece_type_at(sq);
        if (pt == PieceTypes::PAWN) {
            /// make en passant to cancel the check
            if (enpas() != NO_SQUARE && checkers() == Bitboard(shift_square<NORTH>(enemy, enpas()))) {
                mask = attacks::pawnAttacksMask[enemy][enpas()] & notPinned & get_bb_piece(PieceTypes::PAWN, color);
                while (mask) moves[nrMoves++] = Move(mask.get_square_pop(), enpas(), 0, MoveTypes::ENPASSANT);
            }
        }
        if (pt == PieceTypes::KNIGHT) {
            capMask = checkers();
        }
        else {
            capMask = checkers();
            quietMask = between_mask[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (enpas() != NO_SQUARE) {
            Square ep = enpas(), sq2 = shift_square<SOUTH>(color, ep);
            b2 = attacks::pawnAttacksMask[enemy][ep] & get_bb_piece(PieceTypes::PAWN, color);
            b1 = b2 & notPinned;
            while (b1) {
                b = b1.lsb();
                Square sq = b.get_lsb_square();
                if (!(attacks::genAttacksRook(all ^ b ^ Bitboard(sq2) ^ Bitboard(ep), king) & enemyOrthSliders)) {
                    moves[nrMoves++] = Move(sq, ep, 0, MoveTypes::ENPASSANT);
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & line_mask[ep][king];
            if (b1) moves[nrMoves++] = Move(b1.get_lsb_square(), ep, 0, MoveTypes::ENPASSANT);
        }


        /// for pinned pieces they move on the same line with the king
        b1 = pinned & diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = attacks::genAttacksBishop(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = pinned & orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = attacks::genAttacksRook(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns
        b1 = pinned & get_bb_piece(PieceTypes::PAWN, color);
        while (b1) {
            const Square sq = b1.get_square_pop();
            int rank7 = (color == WHITE ? 6 : 1);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = attacks::pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                while (b2) {
                    const int sq2 = b2.get_square_pop();
                    for (int j = 0; j < 4; j++) moves[nrMoves++] = Move(sq, sq2, j, MoveTypes::PROMOTION);
                }
            }
            else {
                b2 = attacks::pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                add_moves(moves, nrMoves, sq, b2);
            }
        }
    }

    /// not pinned pieces (excluding pawns)

    mask = get_bb_piece(PieceTypes::KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksKnight(sq) & capMask);
    }

    mask = diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksBishop(all, sq) & capMask);
    }

    mask = orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksRook(all, sq) & capMask);
    }

    int rank7 = (color == WHITE ? 6 : 1);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = get_bb_piece(PieceTypes::PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
    b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
    /// captures

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTHEAST>(color, sq), sq, 0, NO_TYPE);
    }
    while (b3) {
        Square sq = b3.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTHWEST>(color, sq), sq, 0, NO_TYPE);
    }

    b1 = get_bb_piece(PieceTypes::PAWN, color) & notPinned & rank_mask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift_mask<NORTH>(color, b1) & quietMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq), sq, i, MoveTypes::PROMOTION);
        }

        /// capture promotions
        b2 = shift_mask<NORTHWEST>(color, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift_mask<NORTHEAST>(color, b1 & ~file_mask[fileH]) & capMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTHEAST>(color, sq), sq, i, MoveTypes::PROMOTION);
        }
        while (b3) {
            Square sq = b3.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = Move(shift_square<SOUTHWEST>(color, sq), sq, i, MoveTypes::PROMOTION);
        }
    }

    return nrMoves;
}

/// generate quiet moves
int Board::gen_legal_quiet_moves(MoveList &moves) {
    int nrMoves = 0;
    const bool color = turn, enemy = color ^ 1;
    const Square king = get_king(color), enemyKing = get_king(enemy);
    const int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    Bitboard pieces, mask, us = get_bb_color(color), them = get_bb_color(enemy);
    Bitboard b1, b2, b3;
    Bitboard attacked(0ull), pinned = pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = orthogonal_sliders(enemy), enemyDiagSliders = diagonal_sliders(enemy);
    const Bitboard all = us | them, emptySq = ~all;

    attacked |= get_pawn_attacks(enemy);

    pieces = get_bb_piece(PieceTypes::KNIGHT, enemy);
    while (pieces) attacked |= attacks::genAttacksKnight(pieces.get_square_pop());

    pieces = enemyDiagSliders;
    while (pieces) attacked |= attacks::genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

    pieces = enemyOrthSliders;
    while (pieces) attacked |= attacks::genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

    attacked |= attacks::kingBBAttacks[enemyKing];

    add_moves(moves, nrMoves, king, attacks::kingBBAttacks[king] & ~(us | attacked) & ~them);

    Bitboard notPinned = ~pinned, quietMask(0ull);
    const int cnt = checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        const Square sq = checkers().get_lsb_square();
        quietMask = between_mask[king][sq];
        if (piece_type_at(sq) == PieceTypes::KNIGHT || !quietMask)
            return nrMoves;
    }
    else {
        quietMask = ~all;

        if (!chess960) {
            /// castle queen side
            if (castle_rights() & (1 << (2 * color))) {
                if (!(attacked & Bitboard(7ULL << (king - 2))) && !(all & Bitboard(7ULL << (king - 3)))) {
                    moves[nrMoves++] = Move(king, king - 4, 0, MoveTypes::CASTLE);
                }
            }
            /// castle king side
            if (castle_rights() & (1 << (2 * color + 1))) {
                if (!(attacked & Bitboard(7ULL << king)) && !(all & Bitboard(3ULL << (king + 1)))) {
                    moves[nrMoves++] = Move(king, king + 3, 0, MoveTypes::CASTLE);
                }
            }
        }
        else {
            /// castle queen side
            if ((castle_rights() >> (2 * color)) & 1) {
                Square kingTo = Squares::C1.mirror(color), rook = rookSq[color][0], rookTo = Squares::D1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = Move(king, rook, 0, MoveTypes::CASTLE);
                }
            }
            /// castle king side
            if ((castle_rights() >> (2 * color + 1)) & 1) {
                Square kingTo = Squares::G1.mirror(color), rook = rookSq[color][1], rookTo = Squares::F1.mirror(color);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = Move(king, rook, 0, MoveTypes::CASTLE);
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = pinned & diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            add_moves(moves, nrMoves, sq, attacks::genAttacksBishop(all, sq) & line_mask[king][sq] & quietMask);
        }
        b1 = pinned & orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            add_moves(moves, nrMoves, sq, attacks::genAttacksRook(all, sq) & line_mask[king][sq] & quietMask);
        }

        /// pinned pawns
        b1 = pinned & get_bb_piece(PieceTypes::PAWN, color);
        while (b1) {
            Square sq = b1.get_square_pop();
            if (sq / 8 != rank7) {
                /// single pawn push
                b2 = Bitboard(shift_square<NORTH>(color, sq)) & emptySq & line_mask[king][sq];
                if (b2) {
                    const Square sq2 = b2.get_lsb_square();
                    moves[nrMoves++] = Move(sq, sq2, 0, NO_TYPE);

                    /// double pawn push
                    b3 = Bitboard(shift_square<NORTH>(color, sq2)) & emptySq & line_mask[king][sq];
                    if (b3 && sq2 / 8 == rank3) moves[nrMoves++] = Move(sq, b3.get_lsb_square(), 0, NO_TYPE);
                }

            }
        }
    }

    /// not pinned pieces (excluding pawns)
    mask = get_bb_piece(PieceTypes::KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksKnight(sq) & quietMask);
    }

    mask = diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksBishop(all, sq) & quietMask);
    }

    mask = orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, attacks::genAttacksRook(all, sq) & quietMask);
    }

    b1 = get_bb_piece(PieceTypes::PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift_mask<NORTH>(color, b1) & ~all; /// single push
    b3 = shift_mask<NORTH>(color, b2 & rank_mask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq), sq, 0, NO_TYPE);
    }
    while (b3) {
        Square sq = b3.get_square_pop(), sq2 = shift_square<SOUTH>(color, sq);
        moves[nrMoves++] = Move(shift_square<SOUTH>(color, sq2), sq, 0, NO_TYPE);
    }

    return nrMoves;
}

bool isNoisyMove(Board& board, const Move move) {
    const int t = move.get_type();
    return t == MoveTypes::PROMOTION || t == MoveTypes::ENPASSANT || board.is_capture(move);
}

bool is_pseudo_legal(Board& board, Move move) {
    if (!move) return false;

    const Square from = move.get_from(), to = move.get_to();
    const int t = move.get_type();
    const Piece pt = board.piece_type_at(from);
    const bool color = board.turn;
    const Bitboard own = board.get_bb_color(color), enemy = board.get_bb_color(1 ^ color);
    const Bitboard occ = own | enemy;

    if (!own.has_square(from)) return false;

    if (t == MoveTypes::CASTLE) return true;

    if (own.has_square(to)) return false;

    if (pt == PieceTypes::PAWN) {
        Bitboard att = attacks::pawnAttacksMask[color][from];

        /// enpassant
        if (t == MoveTypes::ENPASSANT) return to == board.enpas() && att.has_square(to);

        Bitboard push = shift_mask<NORTH>(color, Bitboard(from)) & ~occ;
        const int rank_to = to / 8;

        /// promotion
        if (t == MoveTypes::PROMOTION) return (rank_to == 0 || rank_to == 7) && ((att & enemy) | push).has_square(to);

        /// add double push to mask
        if (from / 8 == 1 || from / 8 == 6)
            push |= shift_mask<NORTH>(color, push) & ~occ;

        return (rank_to != 0 && rank_to != 7) && t == NO_TYPE && ((att & enemy) | push).has_square(to);
    }

    if (t != NO_TYPE) return false;

    /// check for normal moves
    if (pt != PieceTypes::KING)
        return attacks::genAttacksSq(occ, from, pt).has_square(to);

    return attacks::kingBBAttacks[from].has_square(to);

}

bool is_legal_slow(Board& board, Move move) {
    MoveList moves;
    int nrMoves = 0;

    nrMoves = board.gen_legal_moves(moves);

    for (int i = 0; i < nrMoves; i++) {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

bool is_legal(Board& board, Move move) {
    if (!is_pseudo_legal(board, move)) return 0;

    const bool us = board.turn, enemy = 1 ^ us;
    const Square king = board.get_king(us);
    Square from = move.get_from(), to = move.get_to();
    const Bitboard all = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);

    if (move.get_type() == MoveTypes::CASTLE) {
        if (from != king || board.checkers()) return 0;
        bool side = (to > from); /// queen side or king side

        if (board.castle_rights() & (1 << (2 * us + side))) { /// can i castle
            const Square rFrom = to, rTo = (side ? Squares::F1 : Squares::D1).mirror(us);
            to = (side ? Squares::G1 : Squares::C1).mirror(us);
            Bitboard mask = between_mask[from][to] | Bitboard(to);
            
            while (mask) {
                if (board.is_attacked_by(enemy, mask.get_square_pop()))
                    return 0;
            }
            if (!board.chess960) {
                if (!side) {
                    return !(all & Bitboard(7ULL << (from - 3)));
                }
                return !(all & Bitboard(3ULL << (from + 1)));
            }
            if ((!((all ^ Bitboard(rFrom)) & (between_mask[from][to] | Bitboard(to))) || from == to) &&
                (!((all ^ Bitboard(from)) & (between_mask[rFrom][rTo] | Bitboard(rTo))) || rFrom == rTo)) {
                return !board.get_attackers(enemy, all ^ Bitboard(rFrom), from);
            }
            return 0;
        }

        return 0;
    }

    if (move.get_type() == MoveTypes::ENPASSANT) {
        const Square cap = shift_square<SOUTH>(us, to);
        const Bitboard all_no_move = all ^ Bitboard(from) ^ Bitboard(to) ^ Bitboard(cap);

        return !(attacks::genAttacksBishop(all_no_move, king) & board.diagonal_sliders(enemy)) && 
               !(attacks::genAttacksRook(all_no_move, king) & board.orthogonal_sliders(enemy));
    }

    if (board.piece_type_at(from) == PieceTypes::KING)
        return !board.get_attackers(enemy, all ^ Bitboard(from), to);

    bool notInCheck = !board.pinned_pieces().has_square(from) || 
                       between_mask[king][to].has_square(from) || 
                       between_mask[king][from].has_square(to);
    if (!notInCheck) return 0;

    if (!board.checkers()) return 1;
    
    return board.checkers().count() == 2 ? false 
           : (board.checkers() | between_mask[king][board.checkers().get_lsb_square()]).has_square(to);
}

bool is_legal_dummy(Board& board, Move move) {
    if (!is_pseudo_legal(board, move))
        return 0;
    if (move.get_type() == MoveTypes::CASTLE)
        return is_legal(board, move);
    bool legal = false;
    board.make_move(move);
    legal = !board.is_attacked_by(board.turn, board.get_king(board.turn ^ 1));
    board.undo_move(move);
    if (legal != is_legal(board, move)) {
        board.print();
        std::cout << move.to_string(board.chess960) << " " << legal << " " << is_legal_slow(board, move) << " " << is_legal(board, move) << "\n";
        exit(0);
    }
    return legal;
}

Move parse_move_string(Board& board, std::string moveStr, Info &info) {
    if (moveStr[1] > '8' || moveStr[1] < '1' || 
        moveStr[3] > '8' || moveStr[3] < '1' || 
        moveStr[0] > 'h' || moveStr[0] < 'a' || 
        moveStr[2] > 'h' || moveStr[2] < 'a')
        return NULLMOVE;

    int from = Square(moveStr[1] - '1', moveStr[0] - 'a');
    if (!info.is_chess960() && board.piece_type_at(from) == PieceTypes::KING) {
        if (moveStr == "e1c1") moveStr = "e1a1";
        else if (moveStr == "e1g1") moveStr = "e1h1";
        else if (moveStr == "e8c8") moveStr = "e8a8";
        else if (moveStr == "e8g8") moveStr = "e8h8";
    }

    Square to = Square(moveStr[3] - '1', moveStr[2] - 'a');

    MoveList moves;
    int nrMoves = board.gen_legal_moves(moves);

    for (int i = 0; i < nrMoves; i++) {
        Move move = moves[i];
        if (move.get_from() == from && move.get_to() == to) {
            int prom = move.get_prom() + PieceTypes::KNIGHT;
            if (move.get_type() == MoveTypes::PROMOTION) {
                if (prom == PieceTypes::ROOK && moveStr[4] == 'r') {
                    return move;
                }
                else if (prom == PieceTypes::BISHOP && moveStr[4] == 'b') {
                    return move;
                }
                else if (prom == PieceTypes::QUEEN && moveStr[4] == 'q') {
                    return move;
                }
                else if (prom == PieceTypes::KNIGHT && moveStr[4] == 'n') {
                    return move;
                }
                continue;
            }
            return move;
        }
    }

    return NULLMOVE;
}

