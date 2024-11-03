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
#include <algorithm>
#include <iostream>
#include "defs.h"
#include "net.h"
#include "attacks.h"

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

    std::array<Piece, 64> board;
    MultiArray<Square, 2, 2> rookSq;    
    MultiArray<uint8_t, 2, 64> castleRightsDelta;

    uint16_t ply, game_ply;

    std::array<Bitboard, 12> bb;
    std::array<Bitboard, 2> pieces;

    Board();

    void clear();

    void print();

    uint64_t& key() { return state.key; }
    uint64_t& pawn_key() { return state.pawn_key; }
    uint64_t& mat_key(const bool color) { return state.mat_key[color]; }
    Bitboard& checkers() { return state.checkers; }
    Bitboard& pinned_pieces() { return state.pinnedPieces; }
    Square& enpas() { return state.enPas; }
    uint16_t& half_moves() { return state.halfMoves; }
    uint16_t& move_index() { return state.moveIndex; }
    uint8_t& castle_rights() { return state.castleRights; }
    Piece& captured() { return state.captured; }

    Bitboard get_bb_piece(const Piece piece, const bool color);
    Bitboard get_bb_color(const bool color);
    Bitboard get_bb_piece_type(const Piece piece_type);

    Bitboard diagonal_sliders(const bool color);
    Bitboard orthogonal_sliders(const bool color);

    Bitboard get_attackers(const bool color, const Bitboard blockers, const Square sq);
    Bitboard get_pinned_pieces();

    Bitboard get_pawn_attacks(const bool color);

    Piece piece_type_at(const Square sq);
    Piece get_captured_type(const Move move);
    Piece piece_at(const Square sq);

    Square get_king(const bool color);

    bool is_capture(const Move move);
    bool is_noisy_move(const Move move);

    bool is_attacked_by(const bool color, const Square sq);
    
    void make_move(const Move move, Network* NN = nullptr);
    void undo_move(const Move move, Network* NN = nullptr);
    void make_null_move();
    void undo_null_move();

    int gen_legal_moves(MoveList &moves);
    int gen_legal_noisy_moves(MoveList &moves);
    int gen_legal_quiet_moves(MoveList &moves);

    bool has_non_pawn_material(const bool color);

    void bring_up_to_date(Network* NN);

    NetInput to_netinput();

    void place_piece_at_sq(Piece piece, Square sq);

    void set_fen(const std::string fen);
    void set_frc_side(bool color, int idx);
    void set_dfrc(int idx);

    std::string fen();

    uint64_t speculative_next_key(const Move move);

    bool is_material_draw();
    bool is_repetition(const int ply);
    bool is_draw(int ply);
    bool has_upcoming_repetition(int ply);
};

class Info {
public:
    int64_t startTime, stopTime;
    int64_t goodTimeLim, hardTimeLim;
    int depth, multipv;
    int movestogo;
    int64_t nodes, min_nodes, max_nodes;

    bool timeset;
    bool sanMode;
    bool chess960;

    Info() : depth(MAX_DEPTH), multipv(1), nodes(-1), min_nodes(-1), max_nodes(-1), chess960(false) {}
};