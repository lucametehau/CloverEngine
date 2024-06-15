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
    return getAttackers(board, color, board.pieces[WHITE] | board.pieces[BLACK], sq);
}

uint64_t getPinnedPieces(Board& board, bool turn) {
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color);
    uint64_t mask, us = board.pieces[color], them = board.pieces[enemy];
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

Move* add_moves(Move* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        *(moves++) = getMove(pos, sq_lsb(att), 0, NEUT);
        nrMoves++;
    }
    return moves;
}

void Board::make_move(Move move) { /// assuming move is at least pseudo-legal
    int posFrom = sq_from(move), posTo = sq_to(move);
    int pieceFrom = board[posFrom], pieceTo = board[posTo];

    history[game_ply].enPas = enPas;
    history[game_ply].castleRights = castleRights;
    history[game_ply].captured = captured;
    history[game_ply].halfMoves = halfMoves;
    history[game_ply].moveIndex = moveIndex;
    history[game_ply].checkers = checkers;
    history[game_ply].pinnedPieces = pinnedPieces;
    history[game_ply].key = key;

    key ^= (enPas >= 0 ? enPasKey[enPas] : 0);

    halfMoves++;

    if (piece_type(pieceFrom) == PAWN)
        halfMoves = 0;

    captured = 0;
    enPas = -1;

    switch (type(move)) {
    case NEUT:
        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo];

        /// moved a castle rook
        if (pieceFrom == WR)
            castleRights &= castleRightsDelta[WHITE][posFrom];
        else if (pieceFrom == BR)
            castleRights &= castleRightsDelta[BLACK][posFrom];

        if (pieceTo) {
            halfMoves = 0;

            pieces[1 ^ turn] ^= (1ULL << posTo);
            bb[pieceTo] ^= (1ULL << posTo);
            key ^= hashKey[pieceTo][posTo];

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                castleRights &= castleRightsDelta[WHITE][posTo];
            else if (pieceTo == BR)
                castleRights &= castleRightsDelta[BLACK][posTo];
        }

        board[posFrom] = 0;
        board[posTo] = pieceFrom;
        captured = pieceTo;

        /// double push
        if (piece_type(pieceFrom) == PAWN && (posFrom ^ posTo) == 16) {
            if ((posTo % 8 && board[posTo - 1] == get_piece(PAWN, turn ^ 1)) || (posTo % 8 < 7 && board[posTo + 1] == get_piece(PAWN, turn ^ 1))) enPas = sq_dir(turn, NORTH, posFrom), key ^= enPasKey[enPas];
        }

        /// moved the king
        if (pieceFrom == WK) {
            castleRights &= castleRightsDelta[WHITE][posFrom];
        }
        else if (pieceFrom == BK) {
            castleRights &= castleRightsDelta[BLACK][posFrom];
        }

        break;
    case ENPASSANT:
    {
        int pos = sq_dir(turn, SOUTH, posTo), pieceCap = get_piece(PAWN, 1 ^ turn);

        halfMoves = 0;

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^ hashKey[pieceCap][pos];

        pieces[1 ^ turn] ^= (1ULL << pos);
        bb[pieceCap] ^= (1ULL << pos);

        board[posFrom] = 0;
        board[posTo] = pieceFrom;
        board[pos] = 0;
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

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^
            hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        board[posFrom] = board[rFrom] = 0;
        board[posTo] = pieceFrom;
        board[rTo] = rPiece;
        captured = 0;

        if (pieceFrom == WK)
            castleRights &= castleRightsDelta[WHITE][posFrom];
        else if (pieceFrom == BK)
            castleRights &= castleRightsDelta[BLACK][posFrom];
    }

    break;
    default: /// promotion
    {
        int promPiece = get_piece(promoted(move) + KNIGHT, turn);

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[pieceFrom] ^= (1ULL << posFrom);
        bb[promPiece] ^= (1ULL << posTo);

        if (pieceTo) {
            bb[pieceTo] ^= (1ULL << posTo);
            pieces[1 ^ turn] ^= (1ULL << posTo);
            key ^= hashKey[pieceTo][posTo];

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                castleRights &= castleRightsDelta[WHITE][posTo];
            else if (pieceTo == BR)
                castleRights &= castleRightsDelta[BLACK][posTo];
        }

        board[posFrom] = 0;
        board[posTo] = promPiece;
        captured = pieceTo;

        key ^= hashKey[pieceFrom][posFrom] ^ hashKey[promPiece][posTo];
    }

    break;
    }

    NN.add_move_to_history(move, pieceFrom, captured);

    /// dirty trick

    int temp = castleRights ^ history[game_ply].castleRights;

    key ^= castleKeyModifier[temp];
    checkers = getAttackers(*this, turn, pieces[WHITE] | pieces[BLACK], king(1 ^ turn));

    turn ^= 1;
    ply++;
    game_ply++;
    key ^= 1;
    if (turn == WHITE)
        moveIndex++;

    pinnedPieces = getPinnedPieces(*this, turn);
}

void Board::undo_move(Move move) {
    turn ^= 1;
    ply--;
    game_ply--;

    enPas = history[game_ply].enPas;
    castleRights = history[game_ply].castleRights;
    halfMoves = history[game_ply].halfMoves;
    moveIndex = history[game_ply].moveIndex;
    checkers = history[game_ply].checkers;
    pinnedPieces = history[game_ply].pinnedPieces;
    key = history[game_ply].key;

    int posFrom = sq_from(move), posTo = sq_to(move), piece = board[posTo], pieceCap = captured;

    switch (type(move)) {
    case NEUT:
        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board[posFrom] = piece;
        board[posTo] = pieceCap;

        if (pieceCap) {
            pieces[1 ^ turn] ^= (1ULL << posTo);
            bb[pieceCap] ^= (1ULL << posTo);
        }
        break;
    case CASTLE:
    {
        int rFrom, rTo, rPiece = get_piece(ROOK, turn);

        piece = get_piece(KING, turn);

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

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board[posTo] = board[rTo] = 0;
        board[posFrom] = piece;
        board[rFrom] = rPiece;
    }
    break;
    case ENPASSANT:
    {
        int pos = sq_dir(turn, SOUTH, posTo);

        pieceCap = get_piece(PAWN, 1 ^ turn);

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        pieces[1 ^ turn] ^= (1ULL << pos);
        bb[pieceCap] ^= (1ULL << pos);

        board[posTo] = 0;
        board[posFrom] = piece;
        board[pos] = pieceCap;
    }
    break;
    default: /// promotion
    {
        int promPiece = get_piece(promoted(move) + KNIGHT, turn);

        piece = get_piece(PAWN, turn);

        pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        bb[piece] ^= (1ULL << posFrom);
        bb[promPiece] ^= (1ULL << posTo);

        board[posTo] = pieceCap;
        board[posFrom] = piece;

        if (pieceCap) {
            pieces[1 ^ turn] ^= (1ULL << posTo);
            bb[pieceCap] ^= (1ULL << posTo);
        }
    }
    break;
    }

    NN.revert_move();

    captured = history[game_ply].captured;
}

void Board::make_null_move() {
    history[game_ply].enPas = enPas;
    history[game_ply].castleRights = castleRights;
    history[game_ply].captured = captured;
    history[game_ply].halfMoves = halfMoves;
    history[game_ply].moveIndex = moveIndex;
    history[game_ply].checkers = checkers;
    history[game_ply].pinnedPieces = pinnedPieces;
    history[game_ply].key = key;

    key ^= (enPas >= 0 ? enPasKey[enPas] : 0);

    checkers = getAttackers(*this, turn, pieces[WHITE] | pieces[BLACK], king(1 ^ turn));
    captured = 0;
    enPas = -1;
    turn ^= 1;
    key ^= 1;
    pinnedPieces = getPinnedPieces(*this, turn);
    ply++;
    game_ply++;
    halfMoves++;
    moveIndex++;
}

void Board::undo_null_move() {
    turn ^= 1;
    ply--;
    game_ply--;

    enPas = history[game_ply].enPas;
    castleRights = history[game_ply].castleRights;
    captured = history[game_ply].captured;
    halfMoves = history[game_ply].halfMoves;
    moveIndex = history[game_ply].moveIndex;
    checkers = history[game_ply].checkers;
    pinnedPieces = history[game_ply].pinnedPieces;
    key = history[game_ply].key;
    //NN.revert_move();
}

int gen_legal_moves(Board& board, Move* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = board.pinnedPieces; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = board.bb[get_piece(KNIGHT, enemy)];
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

    int cnt = count(board.checkers);

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        int sq = Sq(lsb(board.checkers));
        switch (board.piece_type_at(sq)) {
        case PAWN:
            /// make en passant to cancel the check
            if (board.enPas != -1 && board.checkers == (1ULL << (sq_dir(enemy, NORTH, board.enPas)))) {
                mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[get_piece(PAWN, color)];
                while (mask) {
                    *(moves++) = (getMove(sq_lsb(mask), board.enPas, 0, ENPASSANT));
                    nrMoves++;
                }
            }
        case KNIGHT:
            capMask = board.checkers;
            quietMask = 0;
            break;
        default:
            capMask = board.checkers;
            quietMask = between[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (board.enPas >= 0) {
            int ep = board.enPas, sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.bb[get_piece(PAWN, color)];
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



        if (!board.chess960) {
            /// castle queen side
            if (board.castleRights & (1 << (2 * color))) {
                if (!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
                    *(moves++) = (getMove(king, king - 4, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if (board.castleRights & (1 << (2 * color + 1))) {
                if (!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
                    *(moves++) = (getMove(king, king + 3, 0, CASTLE));
                    nrMoves++;
                }
            }
        }
        else {
            if ((board.castleRights >> (2 * color)) & 1) {
                int kingTo = mirror(color, C1), rook = board.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if ((board.castleRights >> (2 * color + 1)) & 1) {
                int kingTo = mirror(color, G1), rook = board.rookSq[color][1], rookTo = mirror(color, F1);
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

        b1 = ~notPinned & board.bb[get_piece(PAWN, color)];
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

    mask = board.bb[get_piece(KNIGHT, color)] & notPinned;
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
    b1 = board.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

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

    b1 = board.bb[get_piece(PAWN, color)] & notPinned & rankMask[rank7];
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

int gen_legal_noisy_moves(Board& board, Move* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = board.pinnedPieces; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];

    if (kingBBAttacks[king] & them) {
        attacked |= pawnAttacks(board, enemy);

        pieces = board.bb[get_piece(KNIGHT, enemy)];
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

    int cnt = count(board.checkers);

    if (cnt == 2) { /// double check
      /// only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        int sq = Sq(lsb(board.checkers));
        switch (board.piece_type_at(sq)) {
        case PAWN:
            /// make en passant to cancel the check
            if (board.enPas != -1 && board.checkers == (1ULL << (sq_dir(enemy, NORTH, board.enPas)))) {
                mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[get_piece(PAWN, color)];
                while (mask) {
                    *(moves++) = (getMove(sq_lsb(mask), board.enPas, 0, ENPASSANT));
                    nrMoves++;
                }
            }
        case KNIGHT:
            capMask = board.checkers;
            quietMask = 0;
            break;
        default:
            capMask = board.checkers;
            quietMask = between[king][sq];
        }
    }
    else {
        capMask = them;
        quietMask = ~all;

        if (board.enPas != -1) {
            int ep = board.enPas, sq2 = sq_dir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.bb[get_piece(PAWN, color)];
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

        b1 = pinned & board.bb[get_piece(PAWN, color)];
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

    mask = board.bb[get_piece(KNIGHT, color)] & notPinned;
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
    b1 = board.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

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

    b1 = board.bb[get_piece(PAWN, color)] & notPinned & rankMask[rank7];
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

int gen_legal_quiet_moves(Board& board, Move* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b1, b2, b3;
    uint64_t attacked = 0, pinned = board.pinnedPieces; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthogonal_sliders(enemy), enemyDiagSliders = board.diagonal_sliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = board.bb[get_piece(KNIGHT, enemy)];
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

    const int cnt = count(board.checkers);

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        const int sq = Sq(lsb(board.checkers));
        quietMask = between[king][sq];
        if (board.piece_type_at(sq) == KNIGHT || !quietMask)
            return nrMoves;
    }
    else {
        quietMask = ~all;

        if (!board.chess960) {
            /// castle queen side
            if (board.castleRights & (1 << (2 * color))) {
                if (!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
                    *(moves++) = (getMove(king, king - 4, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if (board.castleRights & (1 << (2 * color + 1))) {
                if (!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
                    *(moves++) = (getMove(king, king + 3, 0, CASTLE));
                    nrMoves++;
                }
            }
        }
        else {
            /// castle queen side
            if ((board.castleRights >> (2 * color)) & 1) {
                int kingTo = mirror(color, C1), rook = board.rookSq[color][0], rookTo = mirror(color, D1);
                if (!(attacked & (between[king][kingTo] | (1ULL << kingTo))) &&
                    (!((all ^ (1ULL << rook)) & (between[king][kingTo] | (1ULL << kingTo))) || king == kingTo) &&
                    (!((all ^ (1ULL << king)) & (between[rook][rookTo] | (1ULL << rookTo))) || rook == rookTo) &&
                    !getAttackers(board, enemy, all ^ (1ULL << rook), king)) {
                    *(moves++) = (getMove(king, rook, 0, CASTLE));
                    nrMoves++;
                }
            }
            /// castle king side
            if ((board.castleRights >> (2 * color + 1)) & 1) {
                int kingTo = mirror(color, G1), rook = board.rookSq[color][1], rookTo = mirror(color, F1);
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
        b1 = pinned & board.bb[get_piece(PAWN, color)];
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
    mask = board.bb[get_piece(KNIGHT, color)] & notPinned;
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

    b1 = board.bb[get_piece(PAWN, color)] & notPinned & ~rankMask[rank7];

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

inline bool isNoisyMove(Board& board, Move move) {
    if (type(move) && type(move) != CASTLE)
        return 1;

    return board.isCapture(move);
}

bool is_pseudo_legal(Board& board, Move move) {
    if (!move)
        return 0;

    int from = sq_from(move), to = sq_to(move), t = type(move), pt = board.piece_type_at(from), color = board.turn;
    uint64_t own = board.pieces[color], enemy = board.pieces[1 ^ color], occ = board.pieces[WHITE] | board.pieces[BLACK];

    if (color != board.board[from] / 7) /// different color
        return 0;

    if (!board.board[from]) /// there isn't a piece
        return 0;

    if (t == CASTLE)
        return 1;

    if (own & (1ULL << to)) /// can't move piece on the same square as one of our pieces
        return 0;

    if (pt == PAWN) {

        uint64_t att = pawnAttacksMask[color][from];

        /// enpassant
        if (t == ENPASSANT)
            return to == board.enPas && (att & (1ULL << to));

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

bool is_legal_slow(Board& board, Move move) {
    Move moves[256];
    int nrMoves = 0;

    nrMoves = gen_legal_moves(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

bool is_legal(Board& board, Move move) {
    if (!is_pseudo_legal(board, move)) {
        return 0;
    }

    int from = sq_from(move), to = sq_to(move);
    bool us = board.turn, enemy = 1 ^ us;
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];
    int king = board.king(us);

    if (type(move) == CASTLE) {
        if (from != board.king(us) || board.checkers)
            return 0;
        int side = (to > from); /// queen side or king side

        if (board.castleRights & (1 << (2 * us + side))) { /// can i castle
            int rFrom = to, rTo = mirror(us, (side ? F1 : D1));
            to = mirror(us, (side ? G1 : C1));
            uint64_t mask = between[from][to] | (1ULL << to);
            //printBB(mask);
            while (mask) {
                if (isSqAttacked(board, enemy, sq_lsb(mask)))
                    return 0;
            }
            if (!board.chess960) {
                if (!side) {
                    return !(all & (7ULL << (from - 3)));
                }
                return !(all & (3ULL << (from + 1)));
            }
            if ((!((all ^ (1ULL << rFrom)) & (between[from][to] | (1ULL << to))) || from == to) &&
                (!((all ^ (1ULL << from)) & (between[rFrom][rTo] | (1ULL << rTo))) || rFrom == rTo)) {
                return !getAttackers(board, enemy, board.pieces[WHITE] ^ board.pieces[BLACK] ^ (1ULL << rFrom), from);
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

    bool notInCheck = !((1ULL << from) & board.pinnedPieces) || ((1ULL << from) & between[king][to]) || ((1ULL << to) & between[king][from]);

    if (!notInCheck)
        return 0;

    if(!board.checkers)
        return 1;
    
    return ((board.checkers & (board.checkers - 1)) ? false : (1ULL << to) & (board.checkers | between[king][Sq(board.checkers)]));
}

bool is_legal_dummy(Board& board, Move move) {
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
        std::cout << move_to_string(move, board.chess960) << " " << legal << " " << is_legal_slow(board, move) << " " << is_legal(board, move) << "\n";
        exit(0);
    }
    return legal;
}

Move parse_move_string(Board& board, std::string moveStr, Info *info) {
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

    Move moves[256];
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

