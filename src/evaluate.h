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
#include "attacks.h"
#include "defs.h"
#include "board.h"
#include "thread.h"

inline int scale(Board& board) {
    return EvalScaleBias +
        (count(board.get_bb_piece_type(PAWN)) * seeVal[PAWN] + 
        count(board.get_bb_piece_type(KNIGHT)) * seeVal[KNIGHT] +
        count(board.get_bb_piece_type(BISHOP)) * seeVal[BISHOP] +
        count(board.get_bb_piece_type(ROOK)) * seeVal[ROOK] +
        count(board.get_bb_piece_type(QUEEN)) * seeVal[QUEEN]) / 32 - 
        board.halfMoves * EvalShuffleCoef;
}

inline int get_king_bucket_cache(const int king_sq, const bool c) {
    return 5 * ((king_sq & 7) >= 4) + kingIndTable[king_sq ^ (56 * !c)];
}

void Board::bring_up_to_date() {
    const int hist_size = NN.hist_size;
    for (auto side : { BLACK, WHITE }) {
        if (!NN.hist[hist_size - 1].calc[side]) {
            int last_computed_pos = NN.get_computed_parent(side) + 1;
            const int king_sq = king(side);
            if (last_computed_pos) { // no full refresh required
                while(last_computed_pos < hist_size) {
                    NN.process_historic_update(last_computed_pos, king_sq, side);
                    last_computed_pos++;
                }
            }
            else {
                KingBucketState* state = &NN.cached_states[side][get_king_bucket_cache(king_sq, side)];
                NN.clear_updates();
                for (int i = 1; i <= 12; i++) {
                    uint64_t prev = state->bb[i];
                    uint64_t curr = bb[i];

                    uint64_t b = curr & ~prev; // additions
                    while (b) NN.add_input(net_index(i, sq_lsb(b), king_sq, side));

                    b = prev & ~curr; // removals
                    while (b) NN.remove_input(net_index(i, sq_lsb(b), king_sq, side));

                    state->bb[i] = bb[i];
                }
                NN.apply_updates(state->output, state->output);
                memcpy(&NN.output_history[hist_size - 1][side * SIDE_NEURONS], state->output, SIDE_NEURONS * sizeof(int16_t));
                NN.hist[hist_size - 1].calc[side] = 1;
            }
        }
    }
}

int evaluate(Board& board) {
    board.bring_up_to_date();

    int eval = board.NN.get_output(board.turn);
    eval = eval * scale(board) / 1024;
    return eval;
}