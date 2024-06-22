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
    bool turn, chess960;

    uint8_t captured; /// keeping track of last captured piece so i reduce the size of move
    uint8_t castleRights; /// 1 - bq, 2 - bk, 4 - wq, 8 - wk
    int8_t enPas;
    uint8_t board[64], rookSq[2][2];

    uint16_t ply, game_ply;
    uint16_t halfMoves, moveIndex;
    
    uint8_t castleRightsDelta[2][64];

    uint64_t checkers, pinnedPieces;
    uint64_t bb[13];
    uint64_t pieces[2];
    uint64_t key;
    Undo history[2000]; /// fuck it

    Network NN;

    Board() {
        set_fen(START_POS_FEN);
    }

    uint64_t diagonal_sliders(int color) {
        return bb[get_piece(BISHOP, color)] | bb[get_piece(QUEEN, color)];
    }

    uint64_t orthogonal_sliders(int color) {
        return bb[get_piece(ROOK, color)] | bb[get_piece(QUEEN, color)];
    }

    int piece_type_at(int sq) {
        return piece_type(board[sq]);
    }

    int piece_at(int sq) {
        return board[sq];
    }

    int king(bool color) {
        return Sq(bb[get_piece(KING, color)]);
    }

    bool isCapture(Move move) {
        return type(move) != CASTLE && (board[sq_to(move)] > 0);
    }

    void make_move(Move move);

    void undo_move(Move move);

    void make_null_move();

    void undo_null_move();

    bool has_non_pawn_material(bool color) {
        return (pieces[color] ^ bb[get_piece(KING, color)] ^ bb[get_piece(PAWN, color)]) != 0;
    }

    void clear() {
        ply = 0;

        NetInput input = to_netinput();

        NN.calc(input, turn);
    }

    void print() {
        for (int i = 7; i >= 0; i--) {
            for (int j = 0; j <= 7; j++)
                std::cerr << pieceChar[board[8 * i + j]] << " ";
            std::cerr << "\n";
        }
    }

    NetInput to_netinput() {
        NetInput ans;

        int kingsSide[2] = {
            king(BLACK), king(WHITE)
        };

        for (int i = 1; i <= 12; i++) {
            uint64_t b = bb[i];
            while (b) {
                uint64_t b2 = lsb(b);
                ans.ind[WHITE].push_back(net_index(i, Sq(b2), kingsSide[WHITE], WHITE));
                ans.ind[BLACK].push_back(net_index(i, Sq(b2), kingsSide[BLACK], BLACK));
                b ^= b2;
            }
        }

        return ans;
    }

    void place_piece_at_sq(int piece, int sq) {
        board[sq] = piece;
        key ^= hashKey[piece][sq];

        pieces[(piece > 6)] |= (1ULL << sq);
        bb[piece] |= (1ULL << sq);
    }

    void set_fen(const std::string fen); // check init.h

    void setFRCside(bool color, int idx);
    
    void set_dfrc(int idx);

    std::string fen() {
        std::string fen;
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
        std::string s;
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

    uint64_t speculative_next_key(uint64_t move) {
        const int from = sq_from(move), to = sq_to(move), piece = board[from];
        return key ^ hashKey[piece][from] ^ hashKey[piece][to] ^ (board[to] ? hashKey[board[to]][to] : 0) ^ 1;
    }

    bool isMaterialDraw() {
        /// KvK, KBvK, KNvK, KNNvK
        int num = count(pieces[WHITE]) + count(pieces[BLACK]);
        return (num == 2 || (num == 3 && (bb[WN] || bb[BN] || bb[WB] || bb[BB])) ||
            (num == 4 && (count(bb[WN]) == 2 || count(bb[BN]) == 2)));
    }
    
    bool is_repetition(int ply) {
        int cnt = 1;
        for (int i = game_ply - 2; i >= game_ply - halfMoves; i -= 2) {
            if (history[i].key == key) {
                cnt++;
                if (i > game_ply - ply)
                    return 1;
                if (cnt == 3)
                    return 1;
            }
        }
        return 0;
    }

    bool is_draw(int ply);
};

class Info {
public:
    int64_t startTime, stopTime;
    int64_t goodTimeLim, hardTimeLim;
    int depth, sel_depth, multipv;
    int timeset;
    int movestogo;
    int64_t min_nodes, max_nodes, nodes;

    bool sanMode;

    bool quit, stopped;
    bool chess960;
};