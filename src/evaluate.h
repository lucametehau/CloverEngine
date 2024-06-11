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
        (count(board.bb[WP] | board.bb[BP]) * seeVal[PAWN] + 
        count(board.bb[WN] | board.bb[BN]) * seeVal[KNIGHT] +
        count(board.bb[WB] | board.bb[BB]) * seeVal[BISHOP] +
        count(board.bb[WR] | board.bb[BR]) * seeVal[ROOK] +
        count(board.bb[WQ] | board.bb[BQ]) * seeVal[QUEEN]) / 32 - 
        board.halfMoves * EvalShuffleCoef;
}

void bring_up_to_date(Board& board) {
    Network* NN = &board.NN;
    int histSz = NN->histSz;
    for (auto c : { BLACK, WHITE }) {
        if (!NN->hist[histSz - 1].calc[c]) {
            int i = NN->get_computed_parent(c) + 1;
            if (i != 0) { // no full refresh required
                for (int j = i; j < histSz; j++) {
                    NetHist *hist = &NN->hist[j];
                    hist->calc[c] = 1;
                    NN->process_move(hist->move, hist->piece, hist->cap, board.king(c), c, NN->output_history[j][c], NN->output_history[j - 1][c]);
                }
            }
            else {
                const int kingSq = board.king(c);
                KingBucketState* state = &NN->state[c][5 * ((kingSq & 7) >= 4) + kingIndTable[kingSq ^ (56 * !c)]];
                NN->addSz = 0;
                NN->subSz = 0;
                for (int i = 1; i <= 12; i++) {
                    uint64_t prev = state->bb[i];
                    uint64_t curr = board.bb[i];

                    uint64_t b = curr & ~prev; // additions
                    while (b) NN->addInput(net_index(i, sq_lsb(b), kingSq, c));

                    b = prev & ~curr; // removals
                    while (b) NN->remInput(net_index(i, sq_lsb(b), kingSq, c));

                    state->bb[i] = board.bb[i];
                }
                NN->apply_updates(state->output, state->output);
                memcpy(NN->output_history[histSz - 1][c], state->output, sizeof(NN->output_history[histSz - 1][c]));
                NN->hist[histSz - 1].calc[c] = 1;
            }
        }
    }
}

int evaluate(Board& board) {
    bring_up_to_date(board);

    int eval = board.NN.get_output(board.turn);

    eval = eval * scale(board) / 1024;

    return eval;
}