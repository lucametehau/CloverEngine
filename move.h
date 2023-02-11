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

inline uint16_t* addQuiets(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

inline uint16_t* addCaps(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

inline uint16_t* addMoves(uint16_t* moves, int& nrMoves, int pos, uint64_t att) {
    while (att) {
        uint64_t b = lsb(att);
        int pos2 = Sq(b);
        *(moves++) = getMove(pos, pos2, 0, NEUT);
        nrMoves++;
        att ^= b;
    }
    return moves;
}

inline void makeMove(Board& board, uint16_t mv) { /// assuming move is at least pseudo-legal
    int posFrom = sqFrom(mv), posTo = sqTo(mv);
    int pieceFrom = board.board[posFrom], pieceTo = board.board[posTo];
    int kw = board.king(WHITE), kb = board.king(BLACK);
    bool turn = board.turn;

    int ply = board.gamePly;
    board.history[ply].enPas = board.enPas;
    board.history[ply].castleRights = board.castleRights;
    board.history[ply].captured = board.captured;
    board.history[ply].halfMoves = board.halfMoves;
    board.history[ply].moveIndex = board.moveIndex;
    board.history[ply].checkers = board.checkers;
    board.history[ply].pinnedPieces = board.pinnedPieces;
    board.history[ply].key = board.key;

    board.key ^= (board.enPas >= 0 ? enPasKey[board.enPas] : 0);

    board.halfMoves++;

    if (piece_type(pieceFrom) == PAWN)
        board.halfMoves = 0;

    board.captured = 0;
    board.enPas = -1;

    switch (type(mv)) {
    case NEUT:
        board.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo];

        board.NN.removeInput(pieceFrom, posFrom, kw, kb);
        board.NN.addInput(pieceFrom, posTo, kw, kb);

        /// moved a castle rook
        if (pieceFrom == WR)
            board.castleRights &= castleRightsDelta[WHITE][posFrom];
        else if (pieceFrom == BR)
            board.castleRights &= castleRightsDelta[BLACK][posFrom];

        if (pieceTo) {
            board.halfMoves = 0;

            board.pieces[1 ^ turn] ^= (1ULL << posTo);
            board.bb[pieceTo] ^= (1ULL << posTo);
            board.key ^= hashKey[pieceTo][posTo];

            board.NN.removeInput(pieceTo, posTo, kw, kb);

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                board.castleRights &= castleRightsDelta[WHITE][posTo];
            else if (pieceTo == BR)
                board.castleRights &= castleRightsDelta[BLACK][posTo];
        }

        board.board[posFrom] = 0;
        board.board[posTo] = pieceFrom;
        board.captured = pieceTo;

        /// double push
        if (piece_type(pieceFrom) == PAWN && (posFrom ^ posTo) == 16) {
            board.enPas = sqDir(board.turn, NORTH, posFrom);
            board.key ^= enPasKey[board.enPas];
        }

        /// moved the king
        if (pieceFrom == WK) {
            board.castleRights &= castleRightsDelta[WHITE][posFrom];
        }
        else if (pieceFrom == BK) {
            board.castleRights &= castleRightsDelta[BLACK][posFrom];
        }

        break;
    case ENPASSANT:
    {
        int pos = sqDir(turn, SOUTH, posTo), pieceCap = getType(PAWN, 1 ^ turn);

        board.halfMoves = 0;

        board.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^ hashKey[pieceCap][pos];

        board.NN.removeInput(pieceFrom, posFrom, kw, kb);
        board.NN.addInput(pieceFrom, posTo, kw, kb);
        board.NN.removeInput(pieceCap, pos, kw, kb);

        board.pieces[1 ^ turn] ^= (1ULL << pos);
        board.bb[pieceCap] ^= (1ULL << pos);

        board.board[posFrom] = 0;
        board.board[posTo] = pieceFrom;
        board.board[pos] = 0;
    }

    break;
    case CASTLE:
    {
        int rFrom, rTo, rPiece = getType(ROOK, turn);

        if (posTo == mirror(turn, C1)) {
            rFrom = mirror(turn, A1);
            rTo = mirror(turn, D1);
        }
        else {
            rFrom = mirror(turn, H1);
            rTo = mirror(turn, F1);
        }

        board.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^
            (1ULL << rFrom) ^ (1ULL << rTo);
        board.bb[pieceFrom] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board.NN.removeInput(pieceFrom, posFrom, kw, kb);
        board.NN.addInput(pieceFrom, posTo, kw, kb);
        board.NN.removeInput(rPiece, rFrom, kw, kb);
        board.NN.addInput(rPiece, rTo, kw, kb);

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[pieceFrom][posTo] ^
            hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        board.board[posFrom] = board.board[rFrom] = 0;
        board.board[posTo] = pieceFrom;
        board.board[rTo] = rPiece;
        board.captured = 0;

        if (pieceFrom == WK)
            board.castleRights &= castleRightsDelta[WHITE][posFrom];
        else if (pieceFrom == BK)
            board.castleRights &= castleRightsDelta[BLACK][posFrom];
    }

    break;
    default: /// promotion
    {
        int promPiece = getType(promoted(mv) + KNIGHT, turn);

        board.pieces[turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[pieceFrom] ^= (1ULL << posFrom);
        board.bb[promPiece] ^= (1ULL << posTo);

        board.NN.removeInput(pieceFrom, posFrom, kw, kb);
        board.NN.addInput(promPiece, posTo, kw, kb);

        if (pieceTo) {
            board.bb[pieceTo] ^= (1ULL << posTo);
            board.pieces[1 ^ turn] ^= (1ULL << posTo);
            board.key ^= hashKey[pieceTo][posTo];

            board.NN.removeInput(pieceTo, posTo, kw, kb);

            /// special case: captured rook might have been a castle rook
            if (pieceTo == WR)
                board.castleRights &= castleRightsDelta[WHITE][posTo];
            else if (pieceTo == BR)
                board.castleRights &= castleRightsDelta[BLACK][posTo];
        }

        board.board[posFrom] = 0;
        board.board[posTo] = promPiece;
        board.captured = pieceTo;

        board.key ^= hashKey[pieceFrom][posFrom] ^ hashKey[promPiece][posTo];
    }

    break;
    }

    // recalculate if king changes sides
    if (piece_type(pieceFrom) == KING && recalc(posFrom, posTo)) {
        uint64_t b = board.pieces[WHITE] | board.pieces[BLACK];
        kw = board.king(WHITE), kb = board.king(BLACK);

        if (turn == WHITE) {
            board.NN.applyUpdates(BLACK);
            board.NN.updateSz[WHITE] = 0;
            while (b) {
                uint64_t b2 = lsb(b);
                int sq = Sq(b2);
                //board.NN.removeInput(WHITE, netInd(board.piece_at(sq), sq, kw, WHITE));
                board.NN.addInput(WHITE, netInd(board.piece_at(sq), sq, kw, WHITE));
                b ^= b2;
            }
            
            board.NN.applyInitUpdates(WHITE);
        }
        else {
            board.NN.applyUpdates(WHITE);
            board.NN.updateSz[BLACK] = 0;
            while (b) {
                uint64_t b2 = lsb(b);
                int sq = Sq(b2);
                //board.NN.removeInput(BLACK, netInd(board.piece_at(sq), sq, kb, BLACK));
                board.NN.addInput(BLACK, netInd(board.piece_at(sq), sq, kb, BLACK));
                b ^= b2;
            }
            board.NN.applyInitUpdates(BLACK);
        }
    }
    else {
        board.NN.applyUpdates(BLACK);
        board.NN.applyUpdates(WHITE);
    }

    board.NN.histSz++;

    /// dirty trick

    int temp = board.castleRights ^ board.history[ply].castleRights;

    board.key ^= castleKeyModifier[temp];
    board.checkers = getAttackers(board, turn, board.pieces[WHITE] | board.pieces[BLACK], board.king(1 ^ turn));

    board.turn ^= 1;
    board.ply++;
    board.gamePly++;
    board.key ^= 1;
    if (board.turn == WHITE)
        board.moveIndex++;
}

inline void undoMove(Board& board, uint16_t move) {

    board.turn ^= 1;
    board.ply--;
    board.gamePly--;

    int ply = board.gamePly;

    board.enPas = board.history[ply].enPas;
    board.castleRights = board.history[ply].castleRights;
    board.halfMoves = board.history[ply].halfMoves;
    board.moveIndex = board.history[ply].moveIndex;
    board.checkers = board.history[ply].checkers;
    board.pinnedPieces = board.history[ply].pinnedPieces;
    board.key = board.history[ply].key;

    int posFrom = sqFrom(move), posTo = sqTo(move), piece = board.board[posTo], pieceCap = board.captured;

    switch (type(move)) {
    case NEUT:
        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.board[posFrom] = piece;
        board.board[posTo] = pieceCap;

        if (pieceCap) {
            board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
            board.bb[pieceCap] ^= (1ULL << posTo);
        }
        break;
    case CASTLE:
    {
        int rFrom, rTo, rPiece = getType(ROOK, board.turn);

        if (posTo == mirror(board.turn, C1)) {
            rFrom = mirror(board.turn, A1);
            rTo = mirror(board.turn, D1);
        }
        else {
            rFrom = mirror(board.turn, H1);
            rTo = mirror(board.turn, F1);
        }

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        board.bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        board.board[posTo] = board.board[rTo] = 0;
        board.board[posFrom] = piece;
        board.board[rFrom] = rPiece;
    }
    break;
    case ENPASSANT:
    {
        int pos = sqDir(board.turn, SOUTH, posTo);

        pieceCap = getType(PAWN, 1 ^ board.turn);

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[piece] ^= (1ULL << posFrom) ^ (1ULL << posTo);

        board.pieces[1 ^ board.turn] ^= (1ULL << pos);
        board.bb[pieceCap] ^= (1ULL << pos);

        board.board[posTo] = 0;
        board.board[posFrom] = piece;
        board.board[pos] = pieceCap;
    }
    break;
    default: /// promotion
    {
        int promPiece = getType(promoted(move) + KNIGHT, board.turn);

        piece = getType(PAWN, board.turn);

        board.pieces[board.turn] ^= (1ULL << posFrom) ^ (1ULL << posTo);
        board.bb[piece] ^= (1ULL << posFrom);
        board.bb[promPiece] ^= (1ULL << posTo);

        board.board[posTo] = pieceCap;
        board.board[posFrom] = piece;

        if (pieceCap) {
            board.pieces[1 ^ board.turn] ^= (1ULL << posTo);
            board.bb[pieceCap] ^= (1ULL << posTo);
        }
    }
    break;
    }

    board.NN.revertUpdates();

    board.captured = board.history[ply].captured;
}

inline void makeNullMove(Board& board) {
    int ply = board.gamePly;

    board.history[ply].enPas = board.enPas;
    board.history[ply].castleRights = board.castleRights;
    board.history[ply].captured = board.captured;
    board.history[ply].halfMoves = board.halfMoves;
    board.history[ply].moveIndex = board.moveIndex;
    board.history[ply].checkers = board.checkers;
    board.history[ply].pinnedPieces = board.pinnedPieces;
    board.history[ply].key = board.key;

    board.key ^= (board.enPas >= 0 ? enPasKey[board.enPas] : 0);

    board.captured = 0;
    board.checkers = board.pinnedPieces = 0;
    board.enPas = -1;
    board.turn ^= 1;
    board.key ^= 1;
    board.ply++;
    board.gamePly++;
    board.halfMoves++;
    board.moveIndex++;
}

inline void undoNullMove(Board& board) {
    board.turn ^= 1;
    board.ply--;
    board.gamePly--;
    int ply = board.gamePly;

    board.enPas = board.history[ply].enPas;
    board.castleRights = board.history[ply].castleRights;
    board.captured = board.history[ply].captured;
    board.halfMoves = board.history[ply].halfMoves;
    board.moveIndex = board.history[ply].moveIndex;
    board.checkers = board.history[ply].checkers;
    board.pinnedPieces = board.history[ply].pinnedPieces;
    board.key = board.history[ply].key;
}

inline int genLegal(Board& board, uint16_t* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = 0; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = board.bb[getType(KNIGHT, enemy)];
    while (pieces) {
        b = lsb(pieces);
        attacked |= knightBBAttacks[Sq(b)];
        pieces ^= b;
    }

    pieces = enemyDiagSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    pieces = enemyOrthSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    attacked |= kingBBAttacks[enemyKing];

    b1 = kingBBAttacks[king] & ~(us | attacked);

    moves = addMoves(moves, nrMoves, king, b1);

    mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

    while (mask) {
        b = lsb(mask);
        int pos = Sq(b);
        b2 = us & between[pos][king];
        if (!(b2 & (b2 - 1)))
            pinned ^= b2;
        mask ^= b;
    }

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
            if (board.enPas != -1 && board.checkers == (1ULL << (sqDir(enemy, NORTH, board.enPas)))) {
                mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[getType(PAWN, color)];
                while (mask) {
                    b = lsb(mask);
                    *(moves++) = (getMove(Sq(b), board.enPas, 0, ENPASSANT));
                    nrMoves++;
                    mask ^= b;
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
            int ep = board.enPas, sq2 = sqDir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.bb[getType(PAWN, color)];
            b1 = b2 & notPinned;
            while (b1) {
                b = lsb(b1);
                if (!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
                    *(moves++) = (getMove(Sq(b), ep, 0, ENPASSANT));
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

        if ((board.castleRights >> (2 * color)) & 1) {
            if (!(attacked & (7ULL << (king - 2))) && !(all & (7ULL << (king - 3)))) {
                *(moves++) = (getMove(king, king - 2, 0, CASTLE));
                nrMoves++;
            }
        }

        /// castle king side

        if ((board.castleRights >> (2 * color + 1)) & 1) {
            if (!(attacked & (7ULL << king)) && !(all & (3ULL << (king + 1)))) {
                *(moves++) = (getMove(king, king + 2, 0, CASTLE));
                nrMoves++;
            }
        }

        /// for pinned pieces they move on the same line with the king

        b1 = ~notPinned & board.diagSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksBishop(all, sq) & Line[king][sq];
            moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
            moves = addCaps(moves, nrMoves, sq, b2 & capMask);
            b1 ^= b;
        }
        b1 = ~notPinned & board.orthSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksRook(all, sq) & Line[king][sq];
            moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
            moves = addCaps(moves, nrMoves, sq, b2 & capMask);
            b1 ^= b;
        }

        /// pinned pawns

        b1 = ~notPinned & board.bb[getType(PAWN, color)];
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                while (b2) {
                    b3 = lsb(b2);
                    for (int j = 0; j < 4; j++) {
                        *(moves++) = (getMove(sq, Sq(b3), j, PROMOTION));
                        nrMoves++;
                    }
                    b2 ^= b3;
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & them & Line[king][sq];
                moves = addMoves(moves, nrMoves, sq, b2);

                /// single pawn push
                b2 = (1ULL << (sqDir(color, NORTH, sq))) & emptySq & Line[king][sq];
                if (b2) {
                    *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
                    nrMoves++;

                    /// double pawn push
                    b3 = (1ULL << (sqDir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
                    if (b3 && Sq(b2) / 8 == rank3) {
                        *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
                        nrMoves++;
                    }
                }
            }
            b1 ^= b;
        }
    }

    /// not pinned pieces (excluding pawns)
    uint64_t mobMask = capMask | quietMask;

    mask = board.bb[getType(KNIGHT, color)] & notPinned;
    while (mask) {
        uint64_t b = lsb(mask);
        int sq = Sq(b);
        moves = addMoves(moves, nrMoves, sq, knightBBAttacks[sq] & mobMask);
        mask ^= b;
    }

    mask = board.diagSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksBishop(all, sq);
        moves = addMoves(moves, nrMoves, sq, b2 & mobMask);
        mask ^= b;
    }

    mask = board.orthSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksRook(all, sq);
        moves = addMoves(moves, nrMoves, sq, b2 & mobMask);
        mask ^= b;
    }

    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        b = lsb(b2);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, 0, NEUT));
        nrMoves++;
        b2 ^= b;
    }
    while (b3) {
        b = lsb(b3);
        int sq = Sq(b), sq2 = sqDir(color, SOUTH, sq);
        *(moves++) = (getMove(sqDir(color, SOUTH, sq2), sq, 0, NEUT));
        nrMoves++;
        b3 ^= b;
    }

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    /// captures

    while (b2) {
        b = lsb(b2);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, 0, NEUT));
        nrMoves++;
        b2 ^= b;
    }
    while (b3) {
        b = lsb(b3);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, 0, NEUT));
        nrMoves++;
        b3 ^= b;
    }

    b1 = board.bb[getType(PAWN, color)] & notPinned & rankMask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            b = lsb(b2);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b2 ^= b;
        }

        /// capture promotions

        b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
        while (b2) {
            b = lsb(b2);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b2 ^= b;
        }
        while (b3) {
            b = lsb(b3);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b3 ^= b;
        }
    }

    return nrMoves;
}

/// noisy moves generator

inline int genLegalNoisy(Board& board, uint16_t* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = 0; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];

    attacked |= pawnAttacks(board, enemy);

    pieces = board.bb[getType(KNIGHT, enemy)];
    while (pieces) {
        b = lsb(pieces);
        attacked |= knightBBAttacks[Sq(b)];
        pieces ^= b;
    }

    pieces = enemyDiagSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    pieces = enemyOrthSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    attacked |= kingBBAttacks[enemyKing];

    moves = addCaps(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & them);

    mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

    while (mask) {
        b = lsb(mask);
        int pos = Sq(b);
        uint64_t b2 = us & between[pos][king];
        if (!(b2 & (b2 - 1)))
            pinned ^= b2;
        mask ^= b;
    }

    uint64_t notPinned = ~pinned, capMask = 0, quietMask = 0;

    board.pinnedPieces = pinned;

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
            if (board.enPas != -1 && board.checkers == (1ULL << (sqDir(enemy, NORTH, board.enPas)))) {
                mask = pawnAttacksMask[enemy][board.enPas] & notPinned & board.bb[getType(PAWN, color)];
                while (mask) {
                    b = lsb(mask);
                    *(moves++) = (getMove(Sq(b), board.enPas, 0, ENPASSANT));
                    nrMoves++;
                    mask ^= b;
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
            int ep = board.enPas, sq2 = sqDir(color, SOUTH, ep);
            b2 = pawnAttacksMask[enemy][ep] & board.bb[getType(PAWN, color)];
            b1 = b2 & notPinned;
            while (b1) {
                b = lsb(b1);
                if (!(genAttacksRook(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyOrthSliders) &&
                    !(genAttacksBishop(all ^ b ^ (1ULL << sq2) ^ (1ULL << ep), king) & enemyDiagSliders)) {
                    *(moves++) = (getMove(Sq(b), ep, 0, ENPASSANT));
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
        b1 = pinned & board.diagSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksBishop(all, sq) & Line[king][sq];
            moves = addCaps(moves, nrMoves, sq, b2 & capMask);
            b1 ^= b;
        }
        b1 = pinned & board.orthSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksRook(all, sq) & Line[king][sq];
            moves = addCaps(moves, nrMoves, sq, b2 & capMask);
            b1 ^= b;
        }

        /// pinned pawns

        b1 = pinned & board.bb[getType(PAWN, color)];
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1);
            if (sq / 8 == rank7) { /// promotion captures
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                while (b2) {
                    b3 = lsb(b2);
                    for (int j = 0; j < 4; j++) {
                        *(moves++) = (getMove(sq, Sq(b3), j, PROMOTION));
                        nrMoves++;
                    }
                    b2 ^= b3;
                }
            }
            else {
                b2 = pawnAttacksMask[color][sq] & capMask & Line[king][sq];
                moves = addCaps(moves, nrMoves, sq, b2);
            }
            b1 ^= b;
        }
    }

    /// not pinned pieces (excluding pawns)

    mask = board.bb[getType(KNIGHT, color)] & notPinned;
    while (mask) {
        uint64_t b = lsb(mask);
        int sq = Sq(b);
        moves = addCaps(moves, nrMoves, sq, knightBBAttacks[sq] & capMask);
        mask ^= b;
    }

    mask = board.diagSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksBishop(all, sq);
        moves = addCaps(moves, nrMoves, sq, b2 & capMask);
        mask ^= b;
    }

    mask = board.orthSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksRook(all, sq);
        moves = addCaps(moves, nrMoves, sq, b2 & capMask);
        mask ^= b;
    }

    int rank7 = (color == WHITE ? 6 : 1);
    int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
    b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
    b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
    /// captures

    while (b2) {
        b = lsb(b2);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, 0, NEUT));
        nrMoves++;
        b2 ^= b;
    }
    while (b3) {
        b = lsb(b3);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, 0, NEUT));
        nrMoves++;
        b3 ^= b;
    }

    b1 = board.bb[getType(PAWN, color)] & notPinned & rankMask[rank7];
    if (b1) {
        /// quiet promotions
        b2 = shift(color, NORTH, b1) & quietMask;
        while (b2) {
            b = lsb(b2);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b2 ^= b;
        }

        /// capture promotions

        b2 = shift(color, NORTHWEST, b1 & ~fileMask[fileA]) & capMask;
        b3 = shift(color, NORTHEAST, b1 & ~fileMask[fileH]) & capMask;
        while (b2) {
            b = lsb(b2);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTHEAST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b2 ^= b;
        }
        while (b3) {
            b = lsb(b3);
            int sq = Sq(b);
            for (int i = 0; i < 4; i++) {
                *(moves++) = (getMove(sqDir(color, SOUTHWEST, sq), sq, i, PROMOTION));
                nrMoves++;
            }
            b3 ^= b;
        }
    }

    return nrMoves;
}

/// generate quiet moves

inline int genLegalQuiets(Board& board, uint16_t* moves) {
    int nrMoves = 0;
    int color = board.turn, enemy = color ^ 1;
    int king = board.king(color), enemyKing = board.king(enemy), pos = board.king(board.turn);
    uint64_t pieces, mask, us = board.pieces[color], them = board.pieces[enemy];
    uint64_t b, b1, b2, b3;
    uint64_t attacked = 0, pinned = 0; /// squares attacked by enemy / pinned pieces
    uint64_t enemyOrthSliders = board.orthSliders(enemy), enemyDiagSliders = board.diagSliders(enemy);
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK], emptySq = ~all;

    attacked |= pawnAttacks(board, enemy);

    pieces = board.bb[getType(KNIGHT, enemy)];
    while (pieces) {
        b = lsb(pieces);
        attacked |= knightBBAttacks[Sq(b)];
        pieces ^= b;
    }

    pieces = enemyDiagSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksBishop(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    pieces = enemyOrthSliders;
    while (pieces) {
        b = lsb(pieces);
        attacked |= genAttacksRook(all ^ (1ULL << king), Sq(b));
        pieces ^= b;
    }

    attacked |= kingBBAttacks[enemyKing];

    moves = addQuiets(moves, nrMoves, king, kingBBAttacks[king] & ~(us | attacked) & ~them);
    mask = (genAttacksRook(them, king) & enemyOrthSliders) | (genAttacksBishop(them, king) & enemyDiagSliders);

    while (mask) {
        b = lsb(mask);
        int pos = Sq(b);
        uint64_t b2 = us & between[pos][king];
        if (!(b2 & (b2 - 1)))
            pinned ^= b2;
        mask ^= b;
    }

    uint64_t notPinned = ~pinned, quietMask = 0;

    board.pinnedPieces = pinned;

    int cnt = count(board.checkers);

    if (cnt == 2) { /// double check, only king moves are legal
        return nrMoves;
    }
    else if (cnt == 1) { /// one check
        int sq = Sq(lsb(board.checkers));
        quietMask = between[king][sq];
        if (board.piece_type_at(sq) == KNIGHT || !quietMask)
            return nrMoves;
    }
    else {
        quietMask = ~all;
        // castles
        if (board.castleRights & (1 << (2 * color))) {
            if (!(attacked & (7ULL << (pos - 2))) && !(all & (7ULL << (pos - 3)))) {
                *(moves++) = (getMove(pos, pos - 2, 0, CASTLE));
                nrMoves++;
            }
        }

        if (board.castleRights & (1 << (2 * color + 1))) {
            if (!(attacked & (7ULL << pos)) && !(all & (3ULL << (pos + 1)))) {
                *(moves++) = (getMove(pos, pos + 2, 0, CASTLE));
                nrMoves++;
            }
        }

        /// for pinned pieces they move on the same line with the king
        b1 = pinned & board.diagSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksBishop(all, sq) & Line[king][sq];
            moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
            b1 ^= b;
        }
        b1 = pinned & board.orthSliders(color);
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b);
            b2 = genAttacksRook(all, sq) & Line[king][sq];
            moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
            b1 ^= b;
        }

        /// pinned pawns

        b1 = pinned & board.bb[getType(PAWN, color)];
        while (b1) {
            b = lsb(b1);
            int sq = Sq(b), rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);
            if (sq / 8 != rank7) {

                /// single pawn push
                b2 = (1ULL << (sqDir(color, NORTH, sq))) & emptySq & Line[king][sq];
                if (b2) {
                    *(moves++) = (getMove(sq, Sq(b2), 0, NEUT));
                    nrMoves++;

                    /// double pawn push
                    b3 = (1ULL << (sqDir(color, NORTH, Sq(b2)))) & emptySq & Line[king][sq];
                    if (b3 && Sq(b2) / 8 == rank3) {
                        *(moves++) = (getMove(sq, Sq(b3), 0, NEUT));
                        nrMoves++;
                    }
                }

            }
            b1 ^= b;
        }
    }

    /// not pinned pieces (excluding pawns)

    mask = board.bb[getType(KNIGHT, color)] & notPinned;
    while (mask) {
        uint64_t b = lsb(mask);
        int sq = Sq(b);
        moves = addQuiets(moves, nrMoves, sq, knightBBAttacks[sq] & quietMask);
        mask ^= b;
    }

    mask = board.diagSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksBishop(all, sq);
        moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
        mask ^= b;
    }

    mask = board.orthSliders(color) & notPinned;
    while (mask) {
        b = lsb(mask);
        int sq = Sq(b);
        b2 = genAttacksRook(all, sq);
        moves = addQuiets(moves, nrMoves, sq, b2 & quietMask);
        mask ^= b;
    }

    int rank7 = (color == WHITE ? 6 : 1), rank3 = (color == WHITE ? 2 : 5);

    b1 = board.bb[getType(PAWN, color)] & notPinned & ~rankMask[rank7];

    b2 = shift(color, NORTH, b1) & ~all; /// single push
    b3 = shift(color, NORTH, b2 & rankMask[rank3]) & quietMask; /// double push
    b2 &= quietMask;

    while (b2) {
        b = lsb(b2);
        int sq = Sq(b);
        *(moves++) = (getMove(sqDir(color, SOUTH, sq), sq, 0, NEUT));
        nrMoves++;
        b2 ^= b;
    }
    while (b3) {
        b = lsb(b3);
        int sq = Sq(b), sq2 = sqDir(color, SOUTH, sq);
        *(moves++) = (getMove(sqDir(color, SOUTH, sq2), sq, 0, NEUT));
        nrMoves++;
        b3 ^= b;
    }

    return nrMoves;
}

inline bool isNoisyMove(Board& board, uint16_t move) {
    if (type(move) && type(move) != CASTLE)
        return 1;

    return board.isCapture(move);
}


inline bool isRepetition(Board& board, int ply) {
    int cnt = 1;
    for (int i = board.gamePly - 2; i >= 0; i -= 2) {
        if (i < board.gamePly - board.halfMoves)
            break;
        if (board.history[i].key == board.key) {
            cnt++;
            if (i > board.gamePly - ply)
                return 1;
            if (cnt == 3)
                return 1;
        }
    }
    return 0;
}

bool isPseudoLegalMove(Board& board, uint16_t move) {
    if (!move)
        return 0;

    int from = sqFrom(move), to = sqTo(move), t = type(move), pt = board.piece_type_at(from), color = board.turn;
    uint64_t own = board.pieces[color], enemy = board.pieces[1 ^ color], occ = board.pieces[WHITE] | board.pieces[BLACK];

    if (color != board.board[from] / 7) /// different color
        return 0;

    if (!board.board[from]) /// there isn't a piece
        return 0;

    if (own & (1ULL << to)) /// can't move piece on the same square as one of our pieces
        return 0;

    if (t == CASTLE)
        return 1;

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

    if (pt == KNIGHT)
        return knightBBAttacks[from] & (1ULL << to);

    if (pt == BISHOP)
        return genAttacksBishop(occ, from) & (1ULL << to);

    if (pt == ROOK)
        return genAttacksRook(occ, from) & (1ULL << to);

    if (pt == QUEEN)
        return genAttacksQueen(occ, from) & (1ULL << to);

    return kingBBAttacks[from] & (1ULL << to);

}

bool isLegalMoveSlow(Board& board, int move) {
    uint16_t moves[256];
    int nrMoves = 0;

    nrMoves = genLegal(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        if (move == moves[i])
            return 1;
    }

    return 0;
}

bool isLegalMove(Board& board, int move) {
    if (!isPseudoLegalMove(board, move)) {
        return 0;
    }

    int from = sqFrom(move), to = sqTo(move);
    bool us = board.turn, enemy = 1 ^ us;
    uint64_t all = board.pieces[WHITE] | board.pieces[BLACK];
    int king = board.king(us);

    if (type(move) == CASTLE) {
        int side = (to % 8 == 6); /// queen side or king side

        if (board.castleRights & (1 << (2 * us + side))) { /// can i castle
            if (board.checkers || isSqAttacked(board, enemy, to) || isSqAttacked(board, enemy, Sq(between[from][to])))
                return 0;

            /// castle queen side

            if (!side) {
                return !(all & (7ULL << (from - 3)));
            }
            return !(all & (3ULL << (from + 1)));
        }

        return 0;
    }

    if (type(move) == ENPASSANT) {
        int cap = sqDir(us, SOUTH, to);

        all ^= (1ULL << from) ^ (1ULL << to) ^ (1ULL << cap);

        return !(genAttacksBishop(all, king) & board.diagSliders(enemy)) && !(genAttacksRook(all, king) & board.orthSliders(enemy));
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

inline uint16_t parseMove(Board& board, std::string moveStr) {

    if (moveStr[1] > '8' || moveStr[1] < '1')
        return NULLMOVE;
    if (moveStr[3] > '8' || moveStr[3] < '1')
        return NULLMOVE;
    if (moveStr[0] > 'h' || moveStr[0] < 'a')
        return NULLMOVE;
    if (moveStr[2] > 'h' || moveStr[2] < 'a')
        return NULLMOVE;

    int from = getSq(moveStr[1] - '1', moveStr[0] - 'a');
    int to = getSq(moveStr[3] - '1', moveStr[2] - 'a');

    uint16_t moves[256];
    int nrMoves = genLegal(board, moves);

    for (int i = 0; i < nrMoves; i++) {
        int move = moves[i];
        if (sqFrom(move) == from && sqTo(move) == to) {
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

