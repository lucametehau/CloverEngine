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
    return 600 +
        (count(board.pieces[WN] | board.pieces[BN]) * seeVal[KNIGHT] +
        count(board.pieces[WB] | board.pieces[BB]) * seeVal[BISHOP] +
        count(board.pieces[WR] | board.pieces[BR]) * seeVal[ROOK] +
        count(board.pieces[WQ] | board.pieces[BQ]) * seeVal[QUEEN]) / 32;
}

void bringUpToDate(Board& board, Network& NN) {
    int histSz = NN.histSz;
    for (auto& c : { BLACK, WHITE }) {
        if (!NN.hist[histSz - 1].calc[c]) {
            int i = NN.getGoodParent(c) + 1;
            if (i != 0) { // no full refresh required
                for (int j = i; j < histSz; j++) {
                    NetHist hist = NN.hist[j];
                    NN.hist[j].calc[c] = 1;
                    NN.processMove(hist.move, hist.piece, hist.cap, board.king(c), c, NN.histOutput[j][c], NN.histOutput[j - 1][c]);
                }
            }
            else {
                uint64_t b = board.pieces[BLACK] | board.pieces[WHITE];
                NN.addSz = 0;
                while (b) {
                    uint64_t b2 = lsb(b);
                    int sq = Sq(b2);
                    NN.addInput(netInd(board.piece_at(sq), sq, board.king(c), c));
                    b ^= b2;
                }
                NN.applyInitial(c);
                NN.hist[histSz - 1].calc[c] = 1;
            }
        }
    }
}

int evaluate(Board& board) {
    bringUpToDate(board, board.NN);

    int eval = board.NN.getOutput(board.turn);

    //std::cout << scale(board) << "\n";

    eval = eval * scale(board) / 1024;

    return eval;
}