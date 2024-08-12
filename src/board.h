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
#include "attacks.h"

using namespace attacks;

class Undo {
public:
    int8_t enPas;
    uint8_t castleRights;
    uint8_t captured;
    uint16_t halfMoves, moveIndex;
    uint64_t checkers, pinnedPieces;
    uint64_t key, pawn_key;
};

class Board {
public:
    bool turn, chess960;

    uint8_t captured; /// keeping track of last captured piece so i reduce the size of move
    uint8_t castleRights; /// 1 - bq, 2 - bk, 4 - wq, 8 - wk
    int8_t enPas;
    std::array<uint8_t, 64> board;
    MultiArray<uint8_t, 2, 2> rookSq;

    uint16_t ply, game_ply;
    uint16_t halfMoves, moveIndex;
    
    MultiArray<uint8_t, 2, 64> castleRightsDelta;

    uint64_t checkers, pinnedPieces;
    std::array<uint64_t, 13> bb;
    std::array<uint64_t, 2> pieces;
    uint64_t key, pawn_key;
    std::array<Undo, 2000> history; /// fuck it

    Network NN;

    Board() {
        set_fen(START_POS_FEN);
    }

    void clear() {
        ply = 0;
        NetInput input = to_netinput();
        NN.calc(input, turn);
    }

    void print() {
        for (int i = 7; i >= 0; i--) {
            for (int j = 0; j <= 7; j++)
                std::cerr << piece_char[board[8 * i + j]] << " ";
            std::cerr << "\n";
        }
    }

    inline uint64_t diagonal_sliders(const bool color) const { return bb[get_piece(BISHOP, color)] | bb[get_piece(QUEEN, color)]; }

    inline uint64_t orthogonal_sliders(const bool color) const { return bb[get_piece(ROOK, color)] | bb[get_piece(QUEEN, color)]; }

    inline uint64_t get_bb_piece(const int piece, const bool color) const { return bb[get_piece(piece, color)]; }

    inline uint64_t get_bb_color(const bool color) const { return pieces[color]; }

    inline uint64_t get_bb_piece_type(const int piece_type) const { 
        return get_bb_piece(piece_type, WHITE) | get_bb_piece(piece_type, BLACK);
    }

    inline uint64_t get_attackers(const int color, const uint64_t blockers, const int sq) const {
        return (genAttacksPawn(1 ^ color, sq) & get_bb_piece(PAWN, color)) |
            (genAttacksKnight(sq) & get_bb_piece(KNIGHT, color)) | 
            (genAttacksBishop(blockers, sq) & diagonal_sliders(color)) |
            (genAttacksRook(blockers, sq) & orthogonal_sliders(color)) | 
            (genAttacksKing(sq) & get_bb_piece(KING, color));
    }

    inline uint64_t getOrthSliderAttackers(const int color, const uint64_t blockers, const int sq) const {
        return genAttacksRook(blockers, sq) & orthogonal_sliders(color);
    }
    inline uint64_t get_pawn_attacks(const int color) const {
        const uint64_t b = get_bb_piece(PAWN, color);
        const int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
        return shift(color, NORTHWEST, b & ~file_mask[fileA]) | shift(color, NORTHEAST, b & ~file_mask[fileH]);
    }

    inline int piece_type_at(const int sq) const { return piece_type(board[sq]); }

    inline int get_captured_type(const Move move) const { return type(move) == ENPASSANT ? PAWN : piece_type_at(sq_to(move)); }

    inline int piece_at(const int sq) const { return board[sq]; }

    inline int king(const bool color) const { return sq_single_bit(get_bb_piece(KING, color)); }

    inline bool is_capture(const Move move) const { return type(move) != CASTLE && piece_at(sq_to(move)); }

    inline bool is_noisy_move(const Move move) const { 
        return (type(move) && type(move) != CASTLE) || is_capture(move); 
    }

    inline bool is_attacked_by(const int color, const int sq) const { 
        return get_attackers(color, get_bb_color(WHITE) | get_bb_color(BLACK), sq); 
    }

    void make_move(const Move move);

    void undo_move(const Move move);

    void make_null_move();

    void undo_null_move();

    inline bool has_non_pawn_material(const bool color) const {
        return (get_bb_piece(KING, color) ^ get_bb_piece(PAWN, color)) != get_bb_color(color);
    }

    void bring_up_to_date();

    NetInput to_netinput() const {
        NetInput ans;
        const std::array <int, 2> kingsSide = {
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
        if (piece_type(piece) == PAWN) pawn_key ^= hashKey[piece][sq];

        pieces[(piece > 6)] |= (1ULL << sq);
        bb[piece] |= (1ULL << sq);
    }

    void set_fen(const std::string fen); // check init.h

    void set_frc_side(bool color, int idx);
    
    void set_dfrc(int idx);

    std::string fen() {
        std::string fen;
        for (int i = 7; i >= 0; i--) {
            int cnt = 0;
            for (int j = 0, sq = i * 8; j < 8; j++, sq++) {
                if (piece_at(sq) == 0)
                    cnt++;
                else {
                    if (cnt)
                        fen += char(cnt + '0');
                    cnt = 0;
                    fen += piece_char[piece_at(sq)];
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

    inline uint64_t speculative_next_key(const Move move) const {
        const int from = sq_from(move), to = sq_to(move), piece = piece_at(from);
        return key ^ hashKey[piece][from] ^ hashKey[piece][to] ^ (piece_at(to) ? hashKey[piece_at(to)][to] : 0) ^ 1;
    }

    inline bool isMaterialDraw() const {
        /// KvK, KBvK, KNvK, KNNvK
        const int num = count(get_bb_color(WHITE) | get_bb_color(BLACK));
        return (num == 2 || (num == 3 && (get_bb_piece_type(BISHOP) || get_bb_piece_type(KNIGHT))) ||
            (num == 4 && (count(bb[WN]) == 2 || count(bb[BN]) == 2)));
    }
    
    bool is_repetition(const int ply) const {
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
    int depth, multipv;
    int timeset;
    int movestogo;
    int64_t min_nodes, max_nodes, nodes;

    bool sanMode;

    bool chess960;
};