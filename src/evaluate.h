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

int pawnScaleStart = 71;
int pawnScaleStep = 1;
int pawnsOn1Flank = 13;

int scale(Board& board) {
    int pawnCount = count(board.bb[getType(PAWN, board.turn)]);
    uint64_t allPawns = board.bb[WP] | board.bb[BP];

    return std::min(100, pawnScaleStart + pawnScaleStep * pawnCount - pawnsOn1Flank * !((allPawns & flankMask[0]) && (allPawns & flankMask[1])));
}

void bringUpToDate(Board &board, Network &NN) {
    int histSz = NN.histSz;
    for (auto& c : { BLACK, WHITE }) {
        if (!NN.hist[histSz - 1].calc[c]) {
            int i = board.NN.getGoodParent(c) + 1;
            if (i != 0) { // no full refresh required
                for (int j = i; j < histSz; j++) {
                    NetHist hist = NN.hist[j];
                    NN.hist[j].calc[c] = 1;
                    NN.updateSz = 0;
                    NN.processMove(hist.move, hist.piece, hist.cap, board.king(c), c);
                    NN.apply(NN.histOutput[j][c], NN.histOutput[j - 1][c], NN.updateSz, NN.updates);
                }
            }
            else {
                NN.updateSz = 0;
                uint64_t b = board.pieces[BLACK] | board.pieces[WHITE];
                while (b) {
                    uint64_t b2 = lsb(b);
                    int sq = Sq(b2);
                    NN.addInput(netInd(board.piece_at(sq), sq, board.king(c), c));
                    b ^= b2;
                }
                NN.applyInitUpdates(c);
                NN.hist[histSz - 1].calc[c] = 1;
            }
        }
    }
}

int evaluate(Board &board) {
    //board.print();
    bringUpToDate(board, board.NN);
    int eval = board.NN.getOutput(board.turn);

    eval = eval * scale(board) / 100;

    return eval;
}