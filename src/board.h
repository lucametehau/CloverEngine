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
#include <vector>
#include <algorithm>
#include <iostream>
#include "defs.h"
#include "net.h"

class Undo {
public:
    int8_t enPas;
    uint8_t castleRights;
    uint8_t captured;
    uint16_t halfMoves, moveIndex;
    uint64_t checkers, pinnedPieces;
    uint64_t key;
};

class Board {
public:
    bool turn;

    uint8_t captured; /// keeping track of last captured piece so i reduce the size of move
    uint8_t castleRights; /// 1 - bq, 2 - bk, 4 - wq, 8 - wk
    int8_t enPas;
    uint8_t board[64], rookSq[2][2];

    uint16_t ply, gamePly;
    uint16_t halfMoves, moveIndex;
    
    uint64_t checkers, pinnedPieces;
    uint64_t bb[13];
    uint64_t pieces[2];
    uint64_t key;
    Undo history[2000]; /// fuck it

    Network NN;

    Board() {
        halfMoves = moveIndex = key = 0;
        turn = 0;
        ply = gamePly = 0;
        for (int i = 0; i <= 12; i++)
            bb[i] = 0;
        for (int i = 0; i < 64; i++)
            board[i] = 0;
        castleRights = 0;
        captured = 0;
        checkers = pinnedPieces = 0;

        setFen(START_POS_FEN);
    }

    Board(const Board& other) {
        halfMoves = other.halfMoves;
        moveIndex = other.moveIndex;
        turn = other.turn;
        key = other.key;
        gamePly = other.gamePly;
        ply = other.ply;
        for (int i = 0; i <= 12; i++)
            bb[i] = other.bb[i];
        for (int i = BLACK; i <= WHITE; i++)
            pieces[i] = other.pieces[i];
        for (int i = 0; i < 64; i++)
            board[i] = other.board[i];
        castleRights = other.castleRights;
        captured = other.captured;
        NN = other.NN;
        checkers = other.checkers;
        pinnedPieces = other.pinnedPieces;
    }

    uint64_t diagSliders(int color) {
        return bb[getType(BISHOP, color)] | bb[getType(QUEEN, color)];
    }

    uint64_t orthSliders(int color) {
        return bb[getType(ROOK, color)] | bb[getType(QUEEN, color)];
    }

    int piece_type_at(int sq) {
        return piece_type(board[sq]);
    }

    int piece_at(int sq) {
        return board[sq];
    }

    int king(bool color) {
        return Sq(bb[getType(KING, color)]);
    }

    bool isCapture(uint16_t move) {
        return type(move) != CASTLE && (board[sqTo(move)] > 0);
    }

    void makeMove(uint16_t move);

    void undoMove(uint16_t move);

    void makeNullMove();

    void undoNullMove();

    bool hasNonPawnMaterial(bool color) {
        return (pieces[color] ^ bb[getType(KING, color)] ^ bb[getType(PAWN, color)]) != 0;
    }

    void clear() {
        ply = 0;

        NetInput input = toNetInput();

        NN.calc(input, turn);
    }

    void print() {
        for (int i = 7; i >= 0; i--) {
            for (int j = 0; j <= 7; j++)
                std::cerr << pieceChar[board[8 * i + j]] << " ";
            std::cerr << "\n";
        }
    }

    NetInput toNetInput() {
        NetInput ans;

        int kingsSide[2] = {
            king(BLACK), king(WHITE)
        };

        for (int i = 1; i <= 12; i++) {
            uint64_t b = bb[i];
            while (b) {
                uint64_t b2 = lsb(b);
                ans.ind[WHITE].push_back(netInd(i, Sq(b2), kingsSide[WHITE], WHITE));
                ans.ind[BLACK].push_back(netInd(i, Sq(b2), kingsSide[BLACK], BLACK));
                b ^= b2;
            }
        }

        return ans;
    }

    void setFen(const std::string fen); // check init.h

    std::string fen() {
        std::string fen = "";
        for (int i = 7; i >= 0; i--) {
            int cnt = 0;
            for (int j = 0, sq = i * 8; j < 8; j++, sq++) {
                if (board[sq] == 0)
                    cnt++;
                else {
                    if (cnt)
                        fen += char(cnt + '0');
                    cnt = 0;
                    fen += pieceChar[board[sq]];
                }
            }
            if (cnt)
                fen += char(cnt + '0');
            if (i)
                fen += "/";
        }
        fen += " ";
        fen += (turn == WHITE ? "w" : "b");
        fen += " ";
        if (castleRights & 8)
            fen += "K";
        if (castleRights & 4)
            fen += "Q";
        if (castleRights & 2)
            fen += "k";
        if (castleRights & 1)
            fen += "q";
        if (!castleRights)
            fen += "-";
        fen += " ";
        if (enPas >= 0) {
            fen += char('a' + enPas % 8);
            fen += char('1' + enPas / 8);
        }
        else
            fen += "-";
        fen += " ";
        std::string s = "";
        int nr = halfMoves;
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        if (halfMoves)
            fen += s;
        else
            fen += "0";
        fen += " ";
        s = "", nr = moveIndex;
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        fen += s;
        return fen;
    }

    uint64_t hash() {
        uint64_t h = 0;
        h ^= turn;
        for (int i = 0; i < 64; i++) {
            if (board[i])
                h ^= hashKey[board[i]][i];
        }
        for (int i = 0; i < 4; i++)
            h ^= castleKey[i / 2][i % 2] * ((castleRights >> i) & 1);

        h ^= (enPas >= 0 ? enPasKey[enPas] : 0);
        return h;
    }

    bool isMaterialDraw() {
        /// KvK, KBvK, KNvK, KNNvK
        int num = count(pieces[WHITE]) + count(pieces[BLACK]);
        return (num == 2 || (num == 3 && (bb[WN] || bb[BN] || bb[WB] || bb[BB])) ||
            (num == 4 && (count(bb[WN]) == 2 || count(bb[BN]) == 2)));
    }
};

class Info {
public:
    long double startTime, stopTime;
    long double goodTimeLim, hardTimeLim;
    int depth, sel_depth, multipv;
    int timeset;
    int movestogo;
    int64_t nodes;

    bool sanMode;

    bool quit, stopped;
    bool chess960;
};