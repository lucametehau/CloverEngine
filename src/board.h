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

class HistoricalState {
public:
    Square enPas;
    uint8_t castleRights;
    Piece captured;
    uint16_t halfMoves, moveIndex;
    Bitboard checkers, pinnedPieces;
    uint64_t key, pawn_key, mat_key[2];
};

class Board {
public:
    bool turn, chess960;

    HistoricalState state;
    std::array<HistoricalState, STACK_SIZE> history; /// fuck it

    std::array<uint8_t, 64> board;
    MultiArray<uint8_t, 2, 2> rookSq;    
    MultiArray<uint8_t, 2, 64> castleRightsDelta;

    uint16_t ply, game_ply;

    std::array<Bitboard, 12> bb;
    std::array<Bitboard, 2> pieces;

    Network NN;

    Board() { set_fen(START_POS_FEN); }

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

    inline uint64_t& key() { return state.key; }

    inline uint64_t& pawn_key() { return state.pawn_key; }

    inline uint64_t& mat_key(const bool color) { return state.mat_key[color]; }

    inline Bitboard& checkers() { return state.checkers; }

    inline Bitboard& pinned_pieces() { return state.pinnedPieces; }

    inline Square& enpas() { assert(state.enPas <= NO_EP); return state.enPas; }

    inline uint16_t& half_moves() { return state.halfMoves; }

    inline uint16_t& move_index() { return state.moveIndex; }

    inline uint8_t& castle_rights() { return state.castleRights; }

    inline Piece& captured() { return state.captured; }

    inline Bitboard diagonal_sliders(const bool color) const { return bb[get_piece(BISHOP, color)] | bb[get_piece(QUEEN, color)]; }

    inline Bitboard orthogonal_sliders(const bool color) const { return bb[get_piece(ROOK, color)] | bb[get_piece(QUEEN, color)]; }

    inline Bitboard get_bb_piece(const Piece piece, const bool color) const { return bb[get_piece(piece, color)]; }

    inline Bitboard get_bb_color(const bool color) const { return pieces[color]; }

    inline Bitboard get_bb_piece_type(const Piece piece_type) const { 
        return get_bb_piece(piece_type, WHITE) | get_bb_piece(piece_type, BLACK);
    }

    inline Bitboard get_attackers(const bool color, const Bitboard blockers, const Square sq) const {
        return (genAttacksPawn(1 ^ color, sq) & get_bb_piece(PAWN, color)) |
            (genAttacksKnight(sq) & get_bb_piece(KNIGHT, color)) | 
            (genAttacksBishop(blockers, sq) & diagonal_sliders(color)) |
            (genAttacksRook(blockers, sq) & orthogonal_sliders(color)) | 
            (genAttacksKing(sq) & get_bb_piece(KING, color));
    }

    inline Bitboard getOrthSliderAttackers(const bool color, const Bitboard blockers, const Square sq) const {
        return genAttacksRook(blockers, sq) & orthogonal_sliders(color);
    }
    inline Bitboard get_pawn_attacks(const bool color) const {
        const Bitboard b = get_bb_piece(PAWN, color);
        const int fileA = (color == WHITE ? 0 : 7), fileH = 7 - fileA;
        return shift_mask<NORTHWEST>(color, b & ~file_mask[fileA]) | shift_mask<NORTHEAST>(color, b & ~file_mask[fileH]);
    }

    inline Piece piece_type_at(const Square sq) const { return piece_type(board[sq]); }

    inline Piece get_captured_type(const Move move) const { return type(move) == ENPASSANT ? PAWN : piece_type_at(sq_to(move)); }

    inline Piece piece_at(const Square sq) const { return board[sq]; }

    inline Square king(const bool color) const { return get_bb_piece(KING, color).get_lsb_square(); }

    inline bool is_capture(const Move move) const { return type(move) != CASTLE && piece_at(sq_to(move)) != NO_PIECE; }

    inline bool is_noisy_move(const Move move) const { 
        return (type(move) && type(move) != CASTLE) || is_capture(move); 
    }

    inline bool is_attacked_by(const bool color, const Square sq) const { 
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
        const std::array<Square, 2> kingsSide = {
            king(BLACK), king(WHITE)
        };
        for (Piece i = BP; i <= WK; i++) {
            Bitboard b = bb[i];
            while (b) {
                ans.ind[WHITE].push_back(net_index(i, b.get_lsb_square(), kingsSide[WHITE], WHITE));
                ans.ind[BLACK].push_back(net_index(i, b.get_lsb_square(), kingsSide[BLACK], BLACK));
                b ^= b.lsb();
            }
        }

        return ans;
    }

    void place_piece_at_sq(Piece piece, Square sq) {
        board[sq] = piece;
        key() ^= hashKey[piece][sq];
        if (piece_type(piece) == PAWN) pawn_key() ^= hashKey[piece][sq];
        else mat_key(color_of(piece)) ^= hashKey[piece][sq];

        pieces[color_of(piece)] |= (1ULL << sq);
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
                if (piece_at(sq) == NO_PIECE)
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
        if (castle_rights() & 8)
            fen += "K";
        if (castle_rights() & 4)
            fen += "Q";
        if (castle_rights() & 2)
            fen += "k";
        if (castle_rights() & 1)
            fen += "q";
        if (!castle_rights())
            fen += "-";
        fen += " ";
        if (enpas() != NO_EP) {
            fen += char('a' + enpas() % 8);
            fen += char('1' + enpas() / 8);
        }
        else
            fen += "-";
        fen += " ";
        std::string s;
        int nr = half_moves();
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        if (half_moves())
            fen += s;
        else
            fen += "0";
        fen += " ";
        s = "", nr = move_index();
        while (nr)
            s += char('0' + nr % 10), nr /= 10;
        std::reverse(s.begin(), s.end());
        fen += s;
        return fen;
    }

    inline uint64_t speculative_next_key(const Move move) {
        const int from = sq_from(move), to = sq_to(move);
        const Piece piece = piece_at(from);
        return key() ^ hashKey[piece][from] ^ hashKey[piece][to] ^ (piece_at(to) != NO_PIECE ? hashKey[piece_at(to)][to] : 0) ^ 1;
    }

    inline bool isMaterialDraw() const {
        /// KvK, KBvK, KNvK, KNNvK
        const int num = (get_bb_color(WHITE) | get_bb_color(BLACK)).count();
        return (num == 2 || (num == 3 && (get_bb_piece_type(BISHOP) || get_bb_piece_type(KNIGHT))) ||
            (num == 4 && (bb[WN].count() == 2 || bb[BN].count() == 2)));
    }
    
    bool is_repetition(const int ply) {
        int cnt = 1;
        for (int i = game_ply - 2; i >= game_ply - half_moves(); i -= 2) {
            if (history[i].key == key()) {
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