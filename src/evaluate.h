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
#include <cstring>
#include "defs.h"
#include "board.h"

#ifndef TUNE_FLAG
constexpr int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#else
int seeVal[] = { SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0 };
#endif

inline int scale(Board& board) {
    return EvalScaleBias +
        (board.get_bb_piece_type(PieceTypes::PAWN).count() * seeVal[PieceTypes::PAWN] + 
        board.get_bb_piece_type(PieceTypes::KNIGHT).count() * seeVal[PieceTypes::KNIGHT] +
        board.get_bb_piece_type(PieceTypes::BISHOP).count() * seeVal[PieceTypes::BISHOP] +
        board.get_bb_piece_type(PieceTypes::ROOK).count() * seeVal[PieceTypes::ROOK] +
        board.get_bb_piece_type(PieceTypes::QUEEN).count() * seeVal[PieceTypes::QUEEN]) / 32 - 
        board.half_moves() * EvalShuffleCoef;
}

inline int get_king_bucket_cache(const int king_sq, const bool c) {
    return KING_BUCKETS * ((king_sq & 7) >= 4) + kingIndTable[king_sq ^ (56 * !c)];
}

void Board::bring_up_to_date(Network* NN) {
    const int hist_size = NN->hist_size;
    for (auto side : { BLACK, WHITE }) {
        if (!NN->hist[hist_size - 1].calc[side]) {
            int last_computed_pos = NN->get_computed_parent(side) + 1;
            const int king_sq = get_king(side);
            if (last_computed_pos) { // no full refresh required
                while(last_computed_pos < hist_size) {
                    NN->process_historic_update(last_computed_pos, king_sq, side);
                    last_computed_pos++;
                }
            }
            else {
                KingBucketState* state = &NN->cached_states[side][get_king_bucket_cache(king_sq, side)];
                NN->clear_updates();
                for (Piece i = Pieces::BlackPawn; i <= Pieces::WhiteKing; i++) {
                    Bitboard prev = state->bb[i];
                    Bitboard curr = bb[i];

                    Bitboard b = curr & ~prev; // additions
                    while (b) NN->add_input(net_index(i, b.get_square_pop(), king_sq, side));

                    b = prev & ~curr; // removals
                    while (b) NN->remove_input(net_index(i, b.get_square_pop(), king_sq, side));

                    state->bb[i] = curr;
                }
                NN->apply_updates(state->output, state->output);
                memcpy(&NN->output_history[hist_size - 1][side * SIDE_NEURONS], state->output, SIDE_NEURONS * sizeof(int16_t));
                NN->hist[hist_size - 1].calc[side] = 1;
            }
        }
    }
}

int evaluate(Board& board, Network* NN) {
    board.bring_up_to_date(NN);

    int eval = NN->get_output(board.turn);
    eval = eval * scale(board) / 1024;
    return eval;
}