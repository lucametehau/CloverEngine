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

class BoardState {
public:
    bool turn, chess960;
    int8_t enPas;
    uint8_t castleRights;
    uint8_t captured;
    uint8_t board[64], rookSq[2][2];
    uint16_t halfMoves, moveIndex;
    uint64_t checkers, pinnedPieces;
    uint64_t key;
    uint64_t bb[13], pieces[2];
};

class Board {
public:
    bool turn;
    uint8_t castle_rights_delta[2][64];
    int16_t ply, game_ply;
    BoardState history[2000];

    Network NN;

    Board() {
        set_fen(START_POS_FEN);
    }

    BoardState& state() {
        return history[game_ply];
    }

    uint64_t key() {
        return state().key;
    }

    uint64_t diagonal_sliders(int color) {
        return state().bb[get_piece(BISHOP, color)] | state().bb[get_piece(QUEEN, color)];
    }

    uint64_t orthogonal_sliders(int color) {
        return state().bb[get_piece(ROOK, color)] | state().bb[get_piece(QUEEN, color)];
    }

    uint64_t& get_bb(int piece) {
        return state().bb[piece];
    }

    uint64_t get_pieces(bool color) {
        return state().pieces[color];
    }

    uint64_t get_pinned_pieces() {
        return state().pinnedPieces;
    }

    uint64_t get_checkers() {
        return state().checkers;
    }

    uint64_t castle_rights() {
        return state().castleRights;
    }

    int piece_type_at(int sq) {
        return piece_type(piece_at(sq));
    }

    int piece_at(int sq) {
        return state().board[sq];
    }

    int king(bool color) {
        return Sq(state().bb[get_piece(KING, color)]);
    }

    bool isCapture(uint16_t move) {
        return type(move) != CASTLE && (piece_at(sq_to(move)) > 0);
    }

    void make_move(uint16_t move);

    void undo_move(uint16_t move);

    void make_null_move();

    void undo_null_move();

    bool has_non_pawn_material(bool color) {
        return (get_pieces(color) ^ get_bb(get_piece(KING, color)) ^ get_bb(get_piece(PAWN, color))) != 0;
    }

    void clear() {
        ply = 0;

        NetInput input = to_netinput();

        NN.calc(input, turn);
    }

    void print() {
        for (int i = 7; i >= 0; i--) {
            for (int j = 0; j <= 7; j++)
                std::cerr << pieceChar[piece_at(8 * i + j)] << " ";
            std::cerr << "\n";
        }
    }

    NetInput to_netinput() {
        NetInput ans;

        int kingsSide[2] = {
            king(BLACK), king(WHITE)
        };

        for (int i = 1; i <= 12; i++) {
            uint64_t b = get_bb(i);
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
        state().board[sq] = piece;
        state().key ^= hashKey[piece][sq];

        state().pieces[(piece > 6)] |= (1ULL << sq);
        state().bb[piece] |= (1ULL << sq);
    }

    void set_fen(const std::string fen); // check init.h

    void setFRCside(bool color, int idx);
    
    void set_dfrc(int idx);

    std::string fen() {
        std::string fen;
        BoardState& curr = state();

        for (int i = 7; i >= 0; i--) {
            int cnt = 0;
            for (int j = 0, sq = i * 8; j < 8; j++, sq++) {
                if (piece_at(sq) == 0)
                    cnt++;
                else {
                    if (cnt)
                        fen += char(cnt + '0');
                    cnt = 0;
                    fen += pieceChar[piece_at(sq)];
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
        if (curr.castleRights & 8)
            fen += "K";
        if (curr.castleRights & 4)
            fen += "Q";
        if (curr.castleRights & 2)
            fen += "k";
        if (curr.castleRights & 1)
            fen += "q";
        if (!curr.castleRights)
            fen += "-";
        fen += " ";
        if (curr.enPas >= 0) {
            fen += char('a' + curr.enPas % 8);
            fen += char('1' + curr.enPas / 8);
        }
        else
            fen += "-";
        fen += " ";
        std::string s;
        int nr = curr.halfMoves;
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        if (curr.halfMoves)
            fen += s;
        else
            fen += "0";
        fen += " ";
        s = "", nr = curr.moveIndex;
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        fen += s;
        return fen;
    }

    uint64_t speculative_next_key(uint64_t move) {
        const int from = sq_from(move), to = sq_to(move), piece = piece_at(from);
        return state().key ^ hashKey[piece][from] ^ hashKey[piece][to] ^ (piece_at(to) ? hashKey[piece_at(to)][to] : 0) ^ 1;
    }

    bool isMaterialDraw() {
        /// KvK, KBvK, KNvK, KNNvK
        int num = count(get_pieces(WHITE)) + count(get_pieces(BLACK));
        return (num == 2 || (num == 3 && (get_bb(WN) || get_bb(BN) || get_bb(WB) || get_bb(BB))) ||
            (num == 4 && (count(get_bb(WN)) == 2 || count(get_bb(BN)) == 2)));
    }
    
    bool is_repetition(int ply) {
        int cnt = 1;
        const uint64_t key = state().key;
        for (int i = game_ply - 2; i >= game_ply - state().halfMoves; i -= 2) {
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
    int64_t nodes;

    bool sanMode;

    bool quit, stopped;
    bool chess960;

    bool ponder;
};