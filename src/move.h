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
#include "attacks.h"
#include "evaluate.h"

Bitboard getPinnedPieces(Board& board, bool turn) {
    const bool stm = board.turn, enemy = stm ^ 1;
    const Square king = board.king(stm);
    Bitboard mask, us = board.pieces[stm], them = board.pieces[enemy];
    Bitboard pinned; /// squares attacked by enemy / pinned pieces
    Bitboard enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);

    mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

    while (mask) {
        uint64_t b2 = us & between_mask[mask.get_square_pop()][king];
        if (!(b2 & (b2 - 1)))
            pinned ^= b2;
    }

    return pinned;
}

inline void add_moves(MoveList &moves, int& nrMoves, Square pos, Bitboard att) {
    while (att) moves[nrMoves++] = getMove(pos, att.get_square_pop(), 0, NEUT);
}

void Board::make_move(const Move move) { /// assuming move is at least pseudo-legal
    Square from = sq_from(move), to = sq_to(move);
    Piece piece = piece_at(from), piece_cap = piece_at(to);

    history[game_ply] = state;
    key() ^= (enpas() >= 0 ? enPasKey[enpas()] : 0);

    half_moves()++;

    if (piece_type(piece) == PAWN)
        half_moves() = 0;

    captured() = NO_PIECE;
    enpas() = -1;

    switch (type(move)) {
    case NEUT:
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        if (piece_type(piece) == PAWN) pawn_key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        else mat_key(turn) ^= hashKey[piece][from] ^ hashKey[piece][to];
        /// moved a castle rook
        if (piece == WR)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == BR)
            castle_rights() &= castleRightsDelta[BLACK][from];

        if (piece_cap != NO_PIECE) {
            half_moves() = 0;

            pieces[1 ^ turn] ^= (1ULL << to);
            bb[piece_cap] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            if (piece_type(piece_cap) == PAWN) pawn_key() ^= hashKey[piece_cap][to];
            else mat_key(turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == WR)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == BR)
                castle_rights() &= castleRightsDelta[BLACK][to];
        }

        board[from] = NO_PIECE;
        board[to] = piece;
        captured() = piece_cap;

        /// double push
        if (piece_type(piece) == PAWN && (from ^ to) == 16) {
            if ((to % 8 && board[to - 1] == get_piece(PAWN, turn ^ 1)) || 
                (to % 8 < 7 && board[to + 1] == get_piece(PAWN, turn ^ 1))) 
                enpas() = sq_dir(turn, NORTH, from), key() ^= enPasKey[enpas()];
        }

        /// moved the king
        if (piece == WK) {
            castle_rights() &= castleRightsDelta[WHITE][from];
        }
        else if (piece == BK) {
            castle_rights() &= castleRightsDelta[BLACK][from];
        }

        break;
    case ENPASSANT:
    {
        const Square pos = sq_dir(turn, SOUTH, to);
        piece_cap = get_piece(PAWN, 1 ^ turn);
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
    case CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece = get_piece(ROOK, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = mirror(turn, G1);
            rTo = mirror(turn, F1);
        }
        else { // queen side castle
            rFrom = to;
            to = mirror(turn, C1);
            rTo = mirror(turn, D1);
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

        if (piece == WK)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == BK)
            castle_rights() &= castleRightsDelta[BLACK][from];
    }

    break;
    default: /// promotion
    {
        Piece prom_piece = get_piece(promoted(move) + KNIGHT, turn);

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from);
        bb[prom_piece] ^= (1ULL << to);

        if (piece_cap != NO_PIECE) {
            bb[piece_cap] ^= (1ULL << to);
            pieces[1 ^ turn] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            mat_key(turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == WR)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == BR)
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

    NN.add_move_to_history(move, piece, captured());

    key() ^= castleKeyModifier[castle_rights() ^ history[game_ply].castleRights];
    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], king(1 ^ turn));

    turn ^= 1;
    ply++;
    game_ply++;
    key() ^= 1;
    if (turn == WHITE) move_index()++;

    pinned_pieces() = getPinnedPieces(*this, turn);
}

void Board::undo_move(const Move move) {
    turn ^= 1;
    ply--;
    game_ply--;
    Piece piece_cap = captured();
    
    state = history[game_ply];

    Square from = sq_from(move), to = sq_to(move);
    Piece piece = piece_at(to);

    switch (type(move)) {
    case NEUT:
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        board[from] = piece;
        board[to] = piece_cap;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= (1ULL << to);
            bb[piece_cap] ^= (1ULL << to);
        }
        break;
    case CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece = get_piece(ROOK, turn);

        piece = get_piece(KING, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = mirror(turn, G1);
            rTo = mirror(turn, F1);
        }
        else { // queen side castle
            rFrom = to;
            to = mirror(turn, C1);
            rTo = mirror(turn, D1);
        }

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);
        bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board[to] = board[rTo] = NO_PIECE;
        board[from] = piece;
        board[rFrom] = rPiece;
    }
    break;
    case ENPASSANT:
    {
        int pos = sq_dir(turn, SOUTH, to);

        piece_cap = get_piece(PAWN, 1 ^ turn);

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        pieces[1 ^ turn] ^= (1ULL << pos);
        bb[piece_cap] ^= (1ULL << pos);

        board[to] = NO_PIECE;
        board[from] = piece;
        board[pos] = piece_cap;
    }
    break;
    default: /// promotion
    {
        Piece prom_piece = get_piece(promoted(move) + KNIGHT, turn);

        piece = get_piece(PAWN, turn);

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from);
        bb[prom_piece] ^= (1ULL << to);

        board[to] = piece_cap;
        board[from] = piece;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= (1ULL << to);
            bb[piece_cap] ^= (1ULL << to);
        }
    }
    break;
    }

    NN.revert_move();

    captured() = history[game_ply].captured;
}

void Board::make_null_move() {
    history[game_ply] = state;
    
    key() ^= (enpas() >= 0 ? enPasKey[enpas()] : 0);

    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], king(1 ^ turn));
    captured() = NO_PIECE;
    enpas() = -1;
    turn ^= 1;
    key() ^= 1;
    pinned_pieces() = getPinnedPieces(*this, turn);
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

int gen_legal_moves(Board& board, MoveList &moves) {
    int nrMoves = 0;
    const bool color = board.turn, enemy = color ^ 1;
    const Square king = board.king(color), enemyKing = board.king(enemy);
    Bitboard pieces, mask, us = board.get_bb_color(color), them = board.get_bb_color(enemy);
    Bitboard b, b1, b2, b3;
    Bitboard attacked, pinned = board.pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    const Bitboard all = us | them, emptySq = ~all;

    attacked |= board.get_pawn_attacks(enemy);

    pieces = board.get_bb_piece(KNIGHT, enemy);
    while (pieces) attacked |= genAttacksKnight(pieces.get_square_pop());

    pieces = enemyDiagSliders;
    while (pieces) attacked |= genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

    pieces = enemyOrthSliders;
    while (pieces) attacked |= genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

    attacked |= kingBBAttacks[enemyKing];

    b1 = kingBBAttacks[king] & ~(us | attacked);
    add_moves(moves, nrMoves, king, b1);

    Bitboard notPinned = ~pinned, capMask, quietMask;
    int cnt = board.checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        Square sq = board.checkers().get_lsb_square();
        switch (board.piece_type_at(sq)) {
        case PAWN:
            /// make en passant to cancel the check
            if (board.enpas() != -1 && board.checkers() == (1ULL << (sq_dir(enemy, NORTH, board.enpas())))) {
                mask = pawnAttacksMask[enemy][board.enpas()] & notPinned & board.get_bb_piece(PAWN, color);
                while (mask) moves[nrMoves++] = getMove(mask.get_square_pop(), board.enpas(), 0, ENPASSANT);
            }
        case KNIGHT:
            capMask = board.checkers();
            break;
        default:
            capMask = board.checkers();
            quietMask = between_mask[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (board.enpas() >= 0) {
            int ep = board.enpas();
            Square sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.get_bb_piece(PAWN, color);
            b1 = b2 & notPinned;
            while (b1) {
                b = b1.lsb();
                Square sq = b.get_lsb_square();
                if (!(genAttacksRook(all ^ b ^ Bitboard(sq2) ^ Bitboard(1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ Bitboard(sq2) ^ Bitboard(1ULL << ep), king) & enemyDiagSliders)) {
                    moves[nrMoves++] =  getMove(sq, ep, 0, ENPASSANT);
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & line_mask[ep][king];
            if (b1) moves[nrMoves++] = getMove(b1.get_lsb_square(), ep, 0, ENPASSANT);
        }

        if (!board.chess960) {
            /// castle queen side
            if (board.castle_rights() & (1 << (2 * color))) {
                if (!(attacked & Bitboard(7ULL << (king - 2))) && !(all & Bitboard(7ULL << (king - 3)))) {
                    moves[nrMoves++] = getMove(king, king - 4, 0, CASTLE);
                }
            }
            /// castle king side
            if (board.castle_rights() & (1 << (2 * color + 1))) {
                if (!(attacked & Bitboard(7ULL << king)) && !(all & Bitboard(3ULL << (king + 1)))) {
                    moves[nrMoves++] = getMove(king, king + 3, 0, CASTLE);
                }
            }
        }
        else {
            if ((board.castle_rights() >> (2 * color)) & 1) {
                Square kingTo = mirror(color, C1), rook = board.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(kingTo))) || rook == rookTo) &&
                    !board.get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = getMove(king, rook, 0, CASTLE);
                }
            }
            /// castle king side
            if ((board.castle_rights() >> (2 * color + 1)) & 1) {
                Square kingTo = mirror(color, G1), rook = board.rookSq[color][1], rookTo = mirror(color, F1);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(kingTo))) || rook == rookTo) &&
                    !board.get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = getMove(king, rook, 0, CASTLE);
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = ~notPinned & board.diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = genAttacksBishop(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & quietMask);
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = ~notPinned & board.orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = genAttacksRook(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & quietMask);
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns

        b1 = ~notPinned & board.get_bb_piece(PAWN, color);
        while (b1) {
            Square sq = b1.get_square_pop();
            int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                while (b2) {
                    Square sq2 = b2.get_square_pop();
                    for (int j = 0; j < 4; j++) moves[nrMoves++] = getMove(sq, sq2, j, PROMOTION);
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & them & line_mask[king][sq];
                add_moves(moves, nrMoves, sq, b2);

                /// single pawn push
                b2 = Bitboard(sq_dir(color, NORTH, sq)) & emptySq & line_mask[king][sq];
                if (b2) {
                    const Square sq2 = b2.get_lsb_square();
                    moves[nrMoves++] = getMove(sq, sq2, 0, NEUT);

                    /// double pawn push
                    b3 = (1ULL << (sq_dir(color, NORTH, sq2))) & emptySq & line_mask[king][sq];
                    if (b3 && sq2 / 8 == rank3) {
                        moves[nrMoves++] = getMove(sq, b3.get_lsb_square(), 0, NEUT);
                    }
                }
            }
        }
    }

    /// not pinned pieces (excluding pawns)
    Bitboard mobMask = capMask | quietMask;

    mask = board.get_bb_piece(KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksKnight(sq) & mobMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & mobMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & mobMask);
    }

    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = board.get_bb_piece(PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rank_mask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq), sq, 0, NEUT);
    }
    while (b3) {
        Square sq = b3.get_square_pop(), sq2 = sq_dir(color, SOUTH, sq);
        moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq2), sq, 0, NEUT);
    }

    b2 = shift(color, NORTHWEST, b1 & ~file_mask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~file_mask[fileH]) & capMask;
    /// captures

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTHEAST, sq), sq, 0, NEUT);
    }
    while (b3) {
        Square sq = b3.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTHWEST, sq), sq, 0, NEUT);
    }

    b1 = board.get_bb_piece(PAWN, color) & notPinned & rank_mask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq), sq, i, PROMOTION);
        }

        /// capture promotions

        b2 = shift(color, NORTHWEST, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~file_mask[fileH]) & capMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTHEAST, sq), sq, i, PROMOTION);
        }
        while (b3) {
            Square sq = b3.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTHWEST, sq), sq, i, PROMOTION);
        }
    }

    return nrMoves;
}

/// noisy moves generator

int gen_legal_noisy_moves(Board& board, MoveList &moves) {
    int nrMoves = 0;
    const bool color = board.turn, enemy = color ^ 1;
    const Square king = board.king(color), enemyKing = board.king(enemy);
    Bitboard pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    Bitboard b, b1, b2, b3;
    Bitboard attacked, pinned = board.pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    const Bitboard all = us | them;

    if (kingBBAttacks[king] & them) {
        attacked |= board.get_pawn_attacks(enemy);

        pieces = board.bb[get_piece(KNIGHT, enemy)];
        while (pieces) attacked |= genAttacksKnight(pieces.get_square_pop());

        pieces = enemyDiagSliders;
        while (pieces) attacked |= genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

        pieces = enemyOrthSliders;
        while (pieces) attacked |= genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

        attacked |= kingBBAttacks[enemyKing];

        add_moves(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & them);
    }

    Bitboard notPinned = ~pinned, capMask, quietMask;

    int cnt = board.checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        Square sq = board.checkers().get_lsb_square();
        switch (board.piece_type_at(sq)) {
        case PAWN:
            /// make en passant to cancel the check
            if (board.enpas() != -1 && board.checkers() == (1ULL << (sq_dir(enemy, NORTH, board.enpas())))) {
                mask = pawnAttacksMask[enemy][board.enpas()] & notPinned & board.get_bb_piece(PAWN, color);
                while (mask) moves[nrMoves++] = getMove(mask.get_square_pop(), board.enpas(), 0, ENPASSANT);
            }
        case KNIGHT:
            capMask = board.checkers();
            break;
        default:
            capMask = board.checkers();
            quietMask = between_mask[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (board.enpas() != -1) {
            int ep = board.enpas();
            Square sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.get_bb_piece(PAWN, color);
            b1 = b2 & notPinned;
            while (b1) {
                b = b1.lsb();
                Square sq = b.get_lsb_square();
                if (!(genAttacksRook(all ^ b ^ Bitboard(sq2) ^ Bitboard(1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ Bitboard(sq2) ^ Bitboard(1ULL << ep), king) & enemyDiagSliders)) {
                    moves[nrMoves++] = getMove(sq, ep, 0, ENPASSANT);
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & line_mask[ep][king];
            if (b1) moves[nrMoves++] = getMove(b1.get_lsb_square(), ep, 0, ENPASSANT);
        }


        /// for pinned pieces they move on the same line with the king
        b1 = pinned & board.diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = genAttacksBishop(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = pinned & board.orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            b2 = genAttacksRook(all, sq) & line_mask[king][sq];
            add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns
        b1 = pinned & board.get_bb_piece(PAWN, color);
        while (b1) {
            const Square sq = b1.get_square_pop();
            int rank7 = (color == WHITE ? 6 : 1);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                while (b2) {
                    const int sq2 = b2.get_square_pop();
                    for (int j = 0; j < 4; j++) moves[nrMoves++] = getMove(sq, sq2, j, PROMOTION);
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & capMask & line_mask[king][sq];
                add_moves(moves, nrMoves, sq, b2);
            }
        }
    }

    /// not pinned pieces (excluding pawns)

    mask = board.get_bb_piece(KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksKnight(sq) & capMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & capMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & capMask);
    }

    int rank7 = (color == WHITE ? 6 : 1);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = board.get_bb_piece(PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift(color, NORTHWEST, b1 & ~file_mask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~file_mask[fileH]) & capMask;
    /// captures

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTHEAST, sq), sq, 0, NEUT);
    }
    while (b3) {
        Square sq = b3.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTHWEST, sq), sq, 0, NEUT);
    }

    b1 = board.get_bb_piece(PAWN, color) & notPinned & rank_mask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq), sq, i, PROMOTION);
        }

        /// capture promotions
        b2 = shift(color, NORTHWEST, b1 & ~file_mask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~file_mask[fileH]) & capMask;
        while (b2) {
            Square sq = b2.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTHEAST, sq), sq, i, PROMOTION);
        }
        while (b3) {
            Square sq = b3.get_square_pop();
            for (int i = 0; i < 4; i++) moves[nrMoves++] = getMove(sq_dir(color, SOUTHWEST, sq), sq, i, PROMOTION);
        }
    }

    return nrMoves;
}

/// generate quiet moves
int gen_legal_quiet_moves(Board& board, MoveList &moves) {
    int nrMoves = 0;
    const bool color = board.turn, enemy = color ^ 1;
    const Square king = board.king(color), enemyKing = board.king(enemy);
    const int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    Bitboard pieces, mask, us = board.get_bb_color(color), them = board.get_bb_color(enemy);
    Bitboard b1, b2, b3;
    Bitboard attacked, pinned = board.pinned_pieces(); /// squares attacked by enemy / pinned pieces
    const Bitboard enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    const Bitboard all = us | them, emptySq = ~all;

    attacked |= board.get_pawn_attacks(enemy);

    pieces = board.get_bb_piece(KNIGHT, enemy);
    while (pieces) attacked |= genAttacksKnight(pieces.get_square_pop());

    pieces = enemyDiagSliders;
    while (pieces) attacked |= genAttacksBishop(all ^ Bitboard(king), pieces.get_square_pop());

    pieces = enemyOrthSliders;
    while (pieces) attacked |= genAttacksRook(all ^ Bitboard(king), pieces.get_square_pop());

    attacked |= kingBBAttacks[enemyKing];

    add_moves(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & ~them);

    Bitboard notPinned = ~pinned, quietMask;
    const int cnt = board.checkers().count();

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        const Square sq = board.checkers().get_lsb_square();
        quietMask = between_mask[king][sq];
        if (board.piece_type_at(sq) == KNIGHT || !quietMask)
            return nrMoves;
    }
    else {
        quietMask = ~all;

        if (!board.chess960) {
            /// castle queen side
            if (board.castle_rights() & (1 << (2 * color))) {
                if (!(attacked & Bitboard(7ULL << (king - 2))) && !(all & Bitboard(7ULL << (king - 3)))) {
                    moves[nrMoves++] = getMove(king, king - 4, 0, CASTLE);
                }
            }
            /// castle king side
            if (board.castle_rights() & (1 << (2 * color + 1))) {
                if (!(attacked & Bitboard(7ULL << king)) && !(all & Bitboard(3ULL << (king + 1)))) {
                    moves[nrMoves++] = getMove(king, king + 3, 0, CASTLE);
                }
            }
        }
        else {
            /// castle queen side
            if ((board.castle_rights() >> (2 * color)) & 1) {
                Square kingTo = mirror(color, C1), rook = board.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !board.get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = getMove(king, rook, 0, CASTLE);
                }
            }
            /// castle king side
            if ((board.castle_rights() >> (2 * color + 1)) & 1) {
                Square kingTo = mirror(color, G1), rook = board.rookSq[color][1], rookTo = mirror(color, F1);
                if (!(attacked & (between_mask[king][kingTo] | Bitboard(kingTo))) &&
                    (!((all ^ Bitboard(rook)) & (between_mask[king][kingTo] | Bitboard(kingTo))) || king == kingTo) &&
                    (!((all ^ Bitboard(king)) & (between_mask[rook][rookTo] | Bitboard(rookTo))) || rook == rookTo) &&
                    !board.get_attackers(enemy, all ^ Bitboard(rook), king)) {
                    moves[nrMoves++] = getMove(king, rook, 0, CASTLE);
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = pinned & board.diagonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & line_mask[king][sq] & quietMask);
        }
        b1 = pinned & board.orthogonal_sliders(color);
        while (b1) {
            Square sq = b1.get_square_pop();
            add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & line_mask[king][sq] & quietMask);
        }

        /// pinned pawns
        b1 = pinned & board.get_bb_piece(PAWN, color);
        while (b1) {
            Square sq = b1.get_square_pop();
            if (sq / 8 != rank7) {
                /// single pawn push
                b2 = (1ULL << sq_dir(color, NORTH, sq)) & emptySq & line_mask[king][sq];
                if (b2) {
                    const Square sq2 = b2.get_lsb_square();
                    moves[nrMoves++] = getMove(sq, sq2, 0, NEUT);

                    /// double pawn push
                    b3 = Bitboard(sq_dir(color, NORTH, sq2)) & emptySq & line_mask[king][sq];
                    if (b3 && sq2 / 8 == rank3) moves[nrMoves++] = getMove(sq, b3.get_lsb_square(), 0, NEUT);
                }

            }
        }
    }

    /// not pinned pieces (excluding pawns)
    mask = board.get_bb_piece(KNIGHT, color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksKnight(sq) & quietMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & quietMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        Square sq = mask.get_square_pop();
        add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & quietMask);
    }

    b1 = board.get_bb_piece(PAWN, color) & notPinned & ~rank_mask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rank_mask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        Square sq = b2.get_square_pop();
        moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq), sq, 0, NEUT);
    }
    while (b3) {
        Square sq = b3.get_square_pop(), sq2 = sq_dir(color, SOUTH, sq);
        moves[nrMoves++] = getMove(sq_dir(color, SOUTH, sq2), sq, 0, NEUT);
    }

    return nrMoves;
}

inline bool isNoisyMove(Board& board, const Move move) {
    const int t = type(move);
    return t == PROMOTION || t == ENPASSANT || board.is_capture(move);
}

bool is_pseudo_legal(Board& board, Move move) {
    if (!move)
        return 0;

    const Square from = sq_from(move), to = sq_to(move);
    const int t = type(move);
    const Piece pt = board.piece_type_at(from);
    const bool color = board.turn;
    const Bitboard own = board.get_bb_color(color), enemy = board.get_bb_color(1 ^ color);
    const Bitboard occ = own | enemy;

    if (!(own & Bitboard(from))) return 0;

    if (t == CASTLE) return 1;

    if (own & Bitboard(to)) return 0;

    if (pt == PAWN) {
        Bitboard att = pawnAttacksMask[color][from];

        /// enpassant
        if (t == ENPASSANT) return to == board.enpas() && att.has_square(to);

        Bitboard push = shift(color, NORTH, Bitboard(from)) & ~occ;
        const int rank_to = to / 8;

        /// promotion
        if (t == PROMOTION) return (rank_to == 0 || rank_to == 7) && ((att & enemy) | push).has_square(to);

        /// add double push to mask
        if (from / 8 == 1 || from / 8 == 6)
            push |= shift(color, NORTH, push) & ~occ;

        return (rank_to != 0 && rank_to != 7) && t == NEUT && ((att & enemy) | push).has_square(to);
    }

    if (t != NEUT)
        return 0;

    /// check for normal moves
    if (pt != KING)
        return genAttacksSq(occ, from, pt).has_square(to);

    return kingBBAttacks[from].has_square(to);

}

bool is_legal_slow(Board& board, Move move) {
    MoveList moves;
    int nrMoves = 0;

    nrMoves = gen_legal_moves(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

bool is_legal(Board& board, Move move) {
    if (!is_pseudo_legal(board, move)) return 0;

    const bool us = board.turn, enemy = 1 ^ us;
    const Square king = board.king(us);
    Square from = sq_from(move), to = sq_to(move);
    const Bitboard all = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);

    if (type(move) == CASTLE) {
        if (from != king || board.checkers()) return 0;
        bool side = (to > from); /// queen side or king side

        if (board.castle_rights() & (1 << (2 * us + side))) { /// can i castle
            const Square rFrom = to, rTo = mirror(us, (side ? F1 : D1));
            to = mirror(us, (side ? G1 : C1));
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

    if (type(move) == ENPASSANT) {
        const Square cap = sq_dir(us, SOUTH, to);
        const uint64_t all_no_move = all ^ Bitboard(from) ^ Bitboard(to) ^ Bitboard(cap);

        return !(genAttacksBishop(all_no_move, king) & board.diagonal_sliders(enemy)) && 
               !(genAttacksRook(all_no_move, king) & board.orthogonal_sliders(enemy));
    }

    if (board.piece_type_at(from) == KING)
        return !board.get_attackers(enemy, all ^ Bitboard(from), to);

    bool notInCheck = !(board.pinned_pieces().has_square(from) || 
                       between_mask[king][to].has_square(from) || 
                       between_mask[king][from].has_square(to));

    if (!notInCheck) return 0;

    if (!board.checkers()) return 1;
    
    return board.checkers().count() == 2 ? false 
           : (board.checkers() | between_mask[king][board.checkers().get_lsb_square()]).has_square(to);
}

bool is_legal_dummy(Board& board, Move move) {
    if (!is_pseudo_legal(board, move))
        return 0;
    if (type(move) == CASTLE)
        return is_legal(board, move);
    bool legal = false;
    board.make_move(move);
    legal = !board.is_attacked_by(board.turn, board.king(board.turn ^ 1));
    board.undo_move(move);
    if (legal != is_legal(board, move)) {
        board.print();
        std::cout << move_to_string(move, board.chess960) << " " << legal << " " << is_legal_slow(board, move) << " " << is_legal(board, move) << "\n";
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

    int from = get_sq(moveStr[1] - '1', moveStr[0] - 'a');
    if (!info.chess960 && board.piece_type_at(from) == KING) {
        if (moveStr == "e1c1") moveStr = "e1a1";
        else if (moveStr == "e1g1") moveStr = "e1h1";
        else if (moveStr == "e8c8") moveStr = "e8a8";
        else if (moveStr == "e8g8") moveStr = "e8h8";
    }

    Square to = get_sq(moveStr[3] - '1', moveStr[2] - 'a');

    MoveList moves;
    int nrMoves = gen_legal_moves(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        int move = moves[i];
        if (sq_from(move) == from && sq_to(move) == to) {
            int prom = promoted(move) + KNIGHT;
            if (type(move) == PROMOTION) {
                if (prom == ROOK && moveStr[4] == 'r') {
                    return move;
                }
                else if (prom == BISHOP && moveStr[4] == 'b') {
                    return move;
                }
                else if (prom == QUEEN && moveStr[4] == 'q') {
                    return move;
                }
                else if (prom == KNIGHT && moveStr[4] == 'n') {
                    return move;
                }
                continue;
            }
            return move;
        }
    }

    return NULLMOVE;
}

