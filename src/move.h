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

inline bool isSqAttacked(Board& board, int color, int sq) {
    return getAttackers(board, color, board.get_pieces(WHITE) | board.get_pieces(BLACK), sq);
}

uint64_t getPinnedPieces(Board& board, bool turn) {
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color);
    uint64_t mask, us = board.get_pieces(color), them = board.get_pieces(enemy);
    uint64_t b2;
    uint64_t pinned = 0; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);

    mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

    while (mask) {
        b2 = us & between[sq_lsb(mask)][king];
        if (!(b2 & (b2 - 1)))
            pinned ^= b2;
    }

    return pinned;
}

uint16_t* add_moves(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        *(moves++) = getMove(pos, sq_lsb(att), 0, NEUT);
        nrMoves++;
    }
    return moves;
}

void Board::make_move(uint16_t mv) { /// assuming move is at least pseudo-legal
    int posFrom = sq_from(mv), posTo = sq_to(mv);
    int pieceFrom = piece_at(posFrom), pieceTo = piece_at(posTo);

    BoardState& curr = history[game_ply];
    BoardState& next = history[game_ply + 1];
    game_ply++;

    next = curr;

    next.key ^= (next.enPas >= 0 ? enPasKey[next.enPas] : 0);
    next.halfMoves++;

    if (piece_type(pieceFrom) == PAWN)
        next.halfMoves = 0;

    next.captured = 0;
    next.enPas = -1;

    switch (type(mv)) {
    case NEUT:
        next.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        next.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        next.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo];

        /// moved a castle rook
        if (pieceFrom == WR)
            next.castleRights &= castle_rights_delta[WHITE][posFrom];
        else if (pieceFrom == BR)
            next.castleRights &= castle_rights_delta[BLACK][posFrom];

        if (pieceTo) {
            next.halfMoves = 0;

            next.pieces[1 ^ turn] ^= (1ULL << posTo);
            next.bb[pieceTo] ^= (1ULL << posTo);
            next.key ^= hashKey[pieceTo][posTo];

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                next.castleRights &= castle_rights_delta[WHITE][posTo];
            else if (pieceTo == BR)
                next.castleRights &= castle_rights_delta[BLACK][posTo];
        }

        next.board[posFrom] = 0;
        next.board[posTo] = pieceFrom;
        next.captured = pieceTo;

        /// double push
        if (piece_type(pieceFrom) == PAWN && (posFrom ^ posTo) == 16) {
            if ((posTo % 8 && next.board[posTo - 1] == get_piece(PAWN, turn ^ 1)) || (posTo % 8 < 7 && next.board[posTo + 1] == get_piece(PAWN, turn ^ 1))) next.enPas = sq_dir(turn, NORTH, posFrom), next.key ^= enPasKey[next.enPas];
        }

        /// moved the king
        if (pieceFrom == WK) {
            next.castleRights &= castle_rights_delta[WHITE][posFrom];
        }
        else if (pieceFrom == BK) {
            next.castleRights &= castle_rights_delta[BLACK][posFrom];
        }

        break;
    case ENPASSANT:
    {
        int pos = sq_dir(turn, SOUTH, posTo), pieceCap = get_piece(PAWN, 1 ^ turn);

        next.halfMoves = 0;

        next.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        next.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        next.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^ hashKey[pieceCap][pos];

        next.pieces[1 ^ turn] ^= (1ULL << pos);
        next.bb[pieceCap] ^= (1ULL << pos);

        next.board[posFrom] = 0;
        next.board[posTo] = pieceFrom;
        next.board[pos] = 0;
    }

    break;
    case CASTLE:
    {
        int rFrom, rTo, rPiece = get_piece(ROOK, turn);

        if (posTo > posFrom) { // king side castle
            rFrom = posTo;
            posTo = mirror(turn, G1);
            rTo = mirror(turn, F1);
        }
        else { // queen side castle
            rFrom = posTo;
            posTo = mirror(turn, C1);
            rTo = mirror(turn, D1);
        }

        next.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        next.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        next.bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        next.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^
            hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        next.board[posFrom] = next.board[rFrom] = 0;
        next.board[posTo] = pieceFrom;
        next.board[rTo] = rPiece;
        next.captured = 0;

        if (pieceFrom == WK)
            next.castleRights &= castle_rights_delta[WHITE][posFrom];
        else if (pieceFrom == BK)
            next.castleRights &= castle_rights_delta[BLACK][posFrom];
    }

    break;
    default: /// promotion
    {
        int promPiece = get_piece(promoted(mv) + KNIGHT, turn);

        next.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        next.bb[pieceFrom] ^= (1ULL << posFrom);
        next.bb[promPiece] ^= (1ULL << posTo);

        if (pieceTo) {
            next.bb[pieceTo] ^= (1ULL << posTo);
            next.pieces[1 ^ turn] ^= (1ULL << posTo);
            next.key ^= hashKey[pieceTo][posTo];

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                next.castleRights &= castle_rights_delta[WHITE][posTo];
            else if (pieceTo == BR)
                next.castleRights &= castle_rights_delta[BLACK][posTo];
        }

        next.board[posFrom] = 0;
        next.board[posTo] = promPiece;
        next.captured = pieceTo;

        next.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[promPiece][posTo];
    }

    break;
    }

    NN.addHistory(mv, pieceFrom, next.captured);

    /// dirty trick

    int temp = next.castleRights ^ curr.castleRights;

    next.key ^= castleKeyModifier[temp];
    next.checkers = getAttackers(*this, turn, next.pieces[WHITE] | next.pieces[BLACK], king(1 ^ turn));

    turn ^= 1;
    next.turn = turn;
    next.key ^= 1;
    ply++;
    if (turn == WHITE)
        next.moveIndex++;

    next.pinnedPieces = getPinnedPieces(*this, turn);
}

void Board::undo_move(uint16_t move) {
    game_ply--;
    ply--;
    turn ^= 1;
    NN.revertUpdates();
}

void Board::make_null_move() {
    BoardState& curr = history[game_ply];
    BoardState& next = history[game_ply + 1];
    game_ply++;

    next = curr;
    next.key ^= (next.enPas >= 0 ? enPasKey[next.enPas] : 0);
    next.checkers = getAttackers(*this, turn, next.pieces[WHITE] | next.pieces[BLACK], king(1 ^ turn));
    next.captured = 0;
    next.enPas = -1;
    turn ^= 1;
    next.turn = turn;
    next.key ^= 1;
    next.pinnedPieces = getPinnedPieces(*this, turn);
    ply++;
    next.halfMoves++;
    next.moveIndex++;
}

void Board::undo_null_move() {
    turn ^= 1;
    ply--;
    game_ply--;
}

int gen_legal_moves(Board& board, uint16_t* moves) {
    BoardState& curr = board.state();
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.get_pieces(color), them = board.get_pieces(enemy);
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = board.get_pinned_pieces(); /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = curr.pieces[WHITE] | curr.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = board.get_bb(get_piece(KNIGHT, enemy));
    while (pieces) {
        attacked |= knightBBAttacks[sq_lsb(pieces)];
    }

    pieces = enemyDiagSliders;
    while (pieces) {
        attacked |= genAttacksBishop(all ^ (1ULL << king), sq_lsb(pieces));
    }

    pieces = enemyOrthSliders;
    while (pieces) {
        attacked |= genAttacksRook(all ^ (1ULL << king), sq_lsb(pieces));
    }

    attacked |= kingBBAttacks[enemyKing];

    b1 = kingBBAttacks[king] & ~(us | attacked);

    moves = add_moves(moves, nrMoves, king, b1);

    uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

    int cnt = count(curr.checkers);

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        int sq = Sq(lsb(board.get_checkers()));
        switch (board.piece_type_at(sq)) {
        case PAWN:
        {
            const int enPas = board.state().enPas;
            /// make en passant to cancel the check
            if (enPas != -1 && curr.checkers == (1ULL << (sq_dir(enemy, NORTH, enPas)))) {
                mask = pawnAttacksMask[enemy][enPas] & notPinned & board.get_bb(get_piece(PAWN, color));
                while (mask) {
                    *(moves++) = (getMove(sq_lsb(mask), enPas, 0, ENPASSANT));
                    nrMoves++;
                }
            }
        }
        case KNIGHT:
            capMask = curr.checkers;
            quietMask = 0;
            break;
        default:
            capMask = curr.checkers;
            quietMask = between[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (curr.enPas >= 0) {
            int ep = curr.enPas, sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & curr.bb[get_piece(PAWN, color)];
            b1 = b2 & notPinned;
            while (b1) {
                b = lsb(b1);
                int sq = Sq(b);
                if (!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
                    *(moves++) = (getMove(sq, ep, 0, ENPASSANT));
                    nrMoves++;
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & Line[ep][king];
            if (b1) {
                *(moves++) = (getMove(Sq(b1), ep, 0, ENPASSANT));
                nrMoves++;
            }
        }



        if (!curr.chess960) {
            /// castle queen side
            if (curr.castleRights & (1 << (2 * color))) {
                if (!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
                    *(moves++) = (getMove(king, king - 4, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if (curr.castleRights & (1 << (2 * color + 1))) {
                if (!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
                    *(moves++) = (getMove(king, king + 3, 0, CASTLE));
                    nrMoves++;
                }
            }
        }
        else {
            if ((curr.castleRights >> (2 * color)) & 1) {
                int kingTo = mirror(color, C1), rook = curr.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if ((curr.castleRights >> (2 * color + 1)) & 1) {
                int kingTo = mirror(color, G1), rook = curr.rookSq[color][1], rookTo = mirror(color, F1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = ~notPinned & board.diagonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            b2 = genAttacksBishop(all, sq) & Line[king][sq];
            moves = add_moves(moves, nrMoves, sq, b2 & quietMask);
            moves = add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = ~notPinned & board.orthogonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            b2 = genAttacksRook(all, sq) & Line[king][sq];
            moves = add_moves(moves, nrMoves, sq, b2 & quietMask);
            moves = add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns

        b1 = ~notPinned & curr.bb[get_piece(PAWN, color)];
        while (b1) {
            int sq = sq_lsb(b1), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                while (b2) {
                    int sq2 = sq_lsb(b2);
                    for (int j = 0; j < 4; j++) {
                        *(moves++) = (getMove(sq, sq2, j, PROMOTION));
                        nrMoves++;
                    }
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & them & Line[king][sq];
                moves = add_moves(moves, nrMoves, sq, b2);

                /// single pawn push
                b2 = (1ULL << (sq_dir(color, NORTH, sq))) & emptySq & Line[king][sq];
                if (b2) {
                    *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
                    nrMoves++;

                    /// double pawn push
                    b3 = (1ULL << (sq_dir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
                    if (b3 && Sq(b2) / 8 == rank3) {
                        *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
                        nrMoves++;
                    }
                }
            }
        }
    }

    /// not pinned pieces (excluding pawns)
    uint64_t mobMask = capMask | quietMask;

    mask = curr.bb[get_piece(KNIGHT, color)] & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, knightBBAttacks[sq] & mobMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & mobMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & mobMask);
    }

    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = curr.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        int sq = sq_lsb(b2);
        *(moves++) = (getMove(sq_dir(color, SOUTH, sq), sq, 0, NEUT));
        nrMoves++;
    }
    while (b3) {
        int sq = sq_lsb(b3), sq2 = sq_dir(color, SOUTH, sq);
        *(moves++) = (getMove(sq_dir(color, SOUTH, sq2), sq, 0, NEUT));
        nrMoves++;
    }

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    /// captures

    while (b2) {
        int sq = sq_lsb(b2);
        *(moves++) = (getMove(sq_dir(color, SOUTHEAST, sq), sq, 0, NEUT));
        nrMoves++;
    }
    while (b3) {
        int sq = sq_lsb(b3);
        *(moves++) = (getMove(sq_dir(color, SOUTHWEST, sq), sq, 0, NEUT));
        nrMoves++;
    }

    b1 = curr.bb[get_piece(PAWN, color)] & notPinned & rankMask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            int sq = sq_lsb(b2);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTH, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }

        /// capture promotions

        b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
        while (b2) {
            int sq = sq_lsb(b2);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTHEAST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }
        while (b3) {
            int sq = sq_lsb(b3);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTHWEST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }
    }

    return nrMoves;
}

/// noisy moves generator

int gen_legal_noisy_moves(Board& board, uint16_t* moves) {
    BoardState& curr = board.state();
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = curr.pieces[color], them = curr.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = curr.pinnedPieces; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = curr.pieces[WHITE] | curr.pieces[BLACK];

    if (kingBBAttacks[king] & them) {
        attacked |= pawnAttacks(board, enemy);

        pieces = curr.bb[get_piece(KNIGHT, enemy)];
        while (pieces) {
            attacked |= knightBBAttacks[sq_lsb(pieces)];
        }

        pieces = enemyDiagSliders;
        while (pieces) {
            attacked |= genAttacksBishop(all ^ (1ULL << king), sq_lsb(pieces));
        }

        pieces = enemyOrthSliders;
        while (pieces) {
            attacked |= genAttacksRook(all ^ (1ULL << king), sq_lsb(pieces));
        }

        attacked |= kingBBAttacks[enemyKing];

        moves = add_moves(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & them);
    }

    uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

    int cnt = count(curr.checkers);

    if (cnt == 2) { /// double check
      /// only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        int sq = Sq(lsb(curr.checkers));
        switch (board.piece_type_at(sq)) {
        case PAWN:
            /// make en passant to cancel the check
            if (curr.enPas != -1 && curr.checkers == (1ULL << (sq_dir(enemy, NORTH, curr.enPas)))) {
                mask = pawnAttacksMask[enemy][curr.enPas] & notPinned & curr.bb[get_piece(PAWN, color)];
                while (mask) {
                    *(moves++) = (getMove(sq_lsb(mask), curr.enPas, 0, ENPASSANT));
                    nrMoves++;
                }
            }
        case KNIGHT:
            capMask = curr.checkers;
            quietMask = 0;
            break;
        default:
            capMask = curr.checkers;
            quietMask = between[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (curr.enPas != -1) {
            int ep = curr.enPas, sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & curr.bb[get_piece(PAWN, color)];
            b1 = b2 & notPinned;
            while (b1) {
                b = lsb(b1);
                int sq = Sq(b);
                if (!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
                    *(moves++) = (getMove(sq, ep, 0, ENPASSANT));
                    nrMoves++;
                }
                b1 ^= b;
            }
            b1 = b2 & pinned & Line[ep][king];
            if (b1) {
                *(moves++) = (getMove(Sq(b1), ep, 0, ENPASSANT));
                nrMoves++;
            }
        }


        /// for pinned pieces they move on the same line with the king
        b1 = pinned & board.diagonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            b2 = genAttacksBishop(all, sq) & Line[king][sq];
            moves = add_moves(moves, nrMoves, sq, b2 & capMask);
        }
        b1 = pinned & board.orthogonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            b2 = genAttacksRook(all, sq) & Line[king][sq];
            moves = add_moves(moves, nrMoves, sq, b2 & capMask);
        }

        /// pinned pawns

        b1 = pinned & curr.bb[get_piece(PAWN, color)];
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                while (b2) {
                    int sq2 = sq_lsb(b2);
                    for (int j = 0; j < 4; j++) {
                        *(moves++) = (getMove(sq, sq2, j, PROMOTION));
                        nrMoves++;
                    }
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                moves = add_moves(moves, nrMoves, sq, b2);
            }
            b1 ^= b;
        }
    }

    /// not pinned pieces (excluding pawns)

    mask = curr.bb[get_piece(KNIGHT, color)] & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, knightBBAttacks[sq] & capMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & capMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & capMask);
    }

    int rank7 = (color == WHITE ? 6 : 1);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = curr.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    /// captures

    while (b2) {
        int sq = sq_lsb(b2);
        *(moves++) = (getMove(sq_dir(color, SOUTHEAST, sq), sq, 0, NEUT));
        nrMoves++;
    }
    while (b3) {
        int sq = sq_lsb(b3);
        *(moves++) = (getMove(sq_dir(color, SOUTHWEST, sq), sq, 0, NEUT));
        nrMoves++;
    }

    b1 = curr.bb[get_piece(PAWN, color)] & notPinned & rankMask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            int sq = sq_lsb(b2);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTH, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }

        /// capture promotions

        b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
        while (b2) {
            int sq = sq_lsb(b2);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTHEAST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }
        while (b3) {
            int sq = sq_lsb(b3);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sq_dir(color, SOUTHWEST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
        }
    }

    return nrMoves;
}

/// generate quiet moves

int gen_legal_quiet_moves(Board& board, uint16_t* moves) {
    BoardState& curr = board.state();
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = curr.pieces[color], them = curr.pieces[enemy];
    uint64_t b1, b2, b3;
    uint64_t attacked = 0, pinned = curr.pinnedPieces; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = curr.pieces[WHITE] | curr.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = curr.bb[get_piece(KNIGHT, enemy)];
    while (pieces) {
        attacked |= knightBBAttacks[sq_lsb(pieces)];
    }

    pieces = enemyDiagSliders;
    while (pieces) {
        attacked |= genAttacksBishop(all ^ (1ULL << king), sq_lsb(pieces));
    }

    pieces = enemyOrthSliders;
    while (pieces) {
        attacked |= genAttacksRook(all ^ (1ULL << king), sq_lsb(pieces));
    }

    attacked |= kingBBAttacks[enemyKing];

    moves = add_moves(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & ~them);

    uint64_t notPinned = ~pinned, quietMask = 0;

    const int cnt = count(curr.checkers);

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        const int sq = Sq(lsb(curr.checkers));
        quietMask = between[king][sq];
        if (board.piece_type_at(sq) == KNIGHT || !quietMask)
            return nrMoves;
    }
    else {
        quietMask = ~all;

        if (!curr.chess960) {
            /// castle queen side
            if (curr.castleRights & (1 << (2 * color))) {
                if (!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
                    *(moves++) = (getMove(king, king - 4, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if (curr.castleRights & (1 << (2 * color + 1))) {
                if (!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
                    *(moves++) = (getMove(king, king + 3, 0, CASTLE));
                    nrMoves++;
                }
            }
        }
        else {
            /// castle queen side
            if ((curr.castleRights >> (2 * color)) & 1) {
                int kingTo = mirror(color, C1), rook = curr.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if ((curr.castleRights >> (2 * color + 1)) & 1) {
                int kingTo = mirror(color, G1), rook = curr.rookSq[color][1], rookTo = mirror(color, F1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = pinned & board.diagonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            moves = add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & Line[king][sq] & quietMask);
        }
        b1 = pinned & board.orthogonal_sliders(color);
        while (b1) {
            int sq = sq_lsb(b1);
            moves = add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & Line[king][sq] & quietMask);
        }

        /// pinned pawns
        b1 = pinned & curr.bb[get_piece(PAWN, color)];
        while (b1) {
            int sq = sq_lsb(b1), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 != rank7) {
                /// single pawn push
                b2 = (1ULL << (sq_dir(color, NORTH, sq))) & emptySq & Line[king][sq];
                if (b2) {
                    *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
                    nrMoves++;

                    /// double pawn push
                    b3 = (1ULL << (sq_dir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
                    if (b3 && Sq(b2) / 8 == rank3) {
                        *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
                        nrMoves++;
                    }
                }

            }
        }
    }

    /// not pinned pieces (excluding pawns)
    mask = curr.bb[get_piece(KNIGHT, color)] & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, knightBBAttacks[sq] & quietMask);
    }

    mask = board.diagonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksBishop(all, sq) & quietMask);
    }

    mask = board.orthogonal_sliders(color) & notPinned;
    while (mask) {
        int sq = sq_lsb(mask);
        moves = add_moves(moves, nrMoves, sq, genAttacksRook(all, sq) & quietMask);
    }

    const int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);

    b1 = curr.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        int sq = sq_lsb(b2);
        *(moves++) = (getMove(sq_dir(color, SOUTH, sq), sq, 0, NEUT));
        nrMoves++;
    }
    while (b3) {
        int sq = sq_lsb(b3), sq2 = sq_dir(color, SOUTH, sq);
        *(moves++) = (getMove(sq_dir(color, SOUTH, sq2), sq, 0, NEUT));
        nrMoves++;
    }

    return nrMoves;
}

inline bool isNoisyMove(Board& board, uint16_t move) {
    if (type(move) && type(move) != CASTLE)
        return 1;

    return board.isCapture(move);
}

bool is_pseudo_legal(Board& board, uint16_t move) {
    if (!move)
        return 0;

    BoardState& curr = board.state();
    int from = sq_from(move), to = sq_to(move), t = type(move), pt = board.piece_type_at(from), color = board.turn;
    uint64_t own = curr.pieces[color], enemy = curr.pieces[1 ^ color], occ = curr.pieces[WHITE] | curr.pieces[BLACK];

    if (color != board.piece_at(from) / 7) /// different color
        return 0;

    if (!board.piece_at(from)) /// there isn't a piece
        return 0;

    if (t == CASTLE)
        return 1;

    if (own & (1ULL << to)) /// can't move piece on the same square as one of our pieces
        return 0;

    if (pt == PAWN) {

        uint64_t att = pawnAttacksMask[color][from];

        /// enpassant
        if (t == ENPASSANT)
            return to == curr.enPas && (att & (1ULL << to));

        uint64_t push = shift(color, NORTH, (1ULL << from)) & ~occ;

        /// promotion
        if (t == PROMOTION)
            return (to / 8 == 0 || to / 8 == 7) && (((att & enemy) | push) & (1ULL << to));

        /// add double push to mask

        if (from / 8 == 1 || from / 8 == 6)
            push |= shift(color, NORTH, push) & ~occ;

        return (to / 8 && to / 8 != 7) && t == NEUT && (((att & enemy) | push) & (1ULL << to));
    }

    if (t != NEUT)
        return 0;

    /// check for normal moves
    if (pt != KING)
        return genAttacksSq(occ, from, pt) & (1ULL << to);

    return kingBBAttacks[from] & (1ULL << to);

}

bool is_legal_slow(Board& board, int move) {
    uint16_t moves[256];
    int nrMoves = 0;

    nrMoves = gen_legal_moves(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

bool is_legal(Board& board, int move) {
    if (!is_pseudo_legal(board, move)) {
        return 0;
    }

    BoardState& curr = board.state();
    int from = sq_from(move), to = sq_to(move);
    bool us = board.turn, enemy = 1 ^ us;
    uint64_t all = curr.pieces[WHITE] | curr.pieces[BLACK];
    int king = board.king(us);

    if (type(move) == CASTLE) {
        if (from != board.king(us) || curr.checkers)
            return 0;
        int side = (to > from); /// queen side or king side

        if (curr.castleRights & (1 << (2 * us + side))) { /// can i castle
            int rFrom = to, rTo = mirror(us, (side ? F1 : D1));
            to = mirror(us, (side ? G1 : C1));
            uint64_t mask = between[from][to] | (1ULL << to);
            //printBB(mask);
            while (mask) {
                if (isSqAttacked(board, enemy, sq_lsb(mask)))
                    return 0;
            }
            if (!curr.chess960) {
                if (!side) {
                    return !(all & (7ULL << (from - 3)));
                }
                return !(all & (3ULL << (from + 1)));
            }
            if ((!((all ^ (1ULL << rFrom)) & (between[from][to] | (1ULL << to))) || from == to) &&
                (!((all ^ (1ULL << from)) & (between[rFrom][rTo] | (1ULL << rTo))) || rFrom == rTo)) {
                return !getAttackers(board, enemy, curr.pieces[WHITE] ^ curr.pieces[BLACK] ^ (1ULL << rFrom), from);
            }
            return 0;
        }

        return 0;
    }

    if (type(move) == ENPASSANT) {
        int cap = sq_dir(us, SOUTH, to);

        all ^= (1ULL << from) ^ (1ULL << to) ^ (1ULL << cap);

        return !(genAttacksBishop(all, king) & board.diagonal_sliders(enemy)) && !(genAttacksRook(all, king) & board.orthogonal_sliders(enemy));
    }

    if (board.piece_type_at(from) == KING)
        return !(getAttackers(board, enemy, all ^ (1ULL << from), to));

    bool notInCheck = !((1ULL << from) & curr.pinnedPieces) || ((1ULL << from) & between[king][to]) || ((1ULL << to) & between[king][from]);

    if (!notInCheck)
        return 0;

    if(!curr.checkers)
        return 1;
    
    return ((curr.checkers & (curr.checkers - 1)) ? false : (1ULL << to) & (curr.checkers | between[king][Sq(curr.checkers)]));
}

bool is_legal_dummy(Board& board, uint16_t move) {
    if (!is_pseudo_legal(board, move))
        return 0;
    if (type(move) == CASTLE)
        return is_legal(board, move);
    bool legal = false;
    board.make_move(move);
    legal = !isSqAttacked(board, board.turn, board.king(board.turn ^ 1));
    board.undo_move(move);
    if (legal != is_legal(board, move)) {
        board.print();
        std::cout << move_to_string(move, board.state().chess960) << " " << legal << " " << is_legal_slow(board, move) << " " << is_legal(board, move) << "\n";
        exit(0);
    }
    return legal;
}

uint16_t parse_move_string(Board& board, std::string moveStr, Info *info) {
    if (moveStr[1] > '8' || moveStr[1] < '1' || 
        moveStr[3] > '8' || moveStr[3] < '1' || 
        moveStr[0] > 'h' || moveStr[0] < 'a' || 
        moveStr[2] > 'h' || moveStr[2] < 'a')
        return NULLMOVE;

    int from = get_sq(moveStr[1] - '1', moveStr[0] - 'a');
    if (!info->chess960 && board.piece_type_at(from) == KING) {
        if (moveStr == "e1c1") moveStr = "e1a1";
        else if (moveStr == "e1g1") moveStr = "e1h1";
        else if (moveStr == "e8c8") moveStr = "e8a8";
        else if (moveStr == "e8g8") moveStr = "e8h8";
    }

    int to = get_sq(moveStr[3] - '1', moveStr[2] - 'a');

    uint16_t moves[256];
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

