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
#include "board.h"
#include "defs.h"
#include "thread.h"


int histMax = 7616;

int histUpdateDiv = 16384;

int counterHistUpdateDiv = 16384;

int capHistUpdateDiv = 16384;

int A_mult = 16;
int B_mult = 320;


void updateHist(int& hist, int score) {
    hist += score - hist * abs(score) / histUpdateDiv;
}

void updateCounterHist(int& hist, int score) {
    hist += score - hist * abs(score) / counterHistUpdateDiv;
}

void updateCapHist(int& hist, int score) {
    hist += score - hist * abs(score) / capHistUpdateDiv;
}

int getHistoryBonus(int depth) {
    return std::min(A_mult * depth * depth + B_mult * depth, histMax);
}

void updateMoveHistory(Search* searcher, StackEntry*& stack, uint16_t move, int bonus) {
    int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from);

    updateHist(searcher->hist[searcher->board.turn][from][to], bonus);

    if ((stack - 1)->move)
        updateCounterHist((*(stack - 1)->continuationHist)[piece][to], bonus);

    if ((stack - 2)->move)
        updateCounterHist((*(stack - 2)->continuationHist)[piece][to], bonus);
}

void updateCaptureMoveHistory(Search* searcher, uint16_t move, int bonus) {
    int to = sqTo(move), from = sqFrom(move), cap = searcher->board.piece_type_at(to), piece = searcher->board.piece_at(from);

    if (type(move) == ENPASSANT)
        cap = PAWN;

    updateCapHist(searcher->capHist[piece][to][cap], bonus);
}

void updateHistory(Search* searcher, StackEntry* stack, int nrQuiets, int ply, int bonus) {
    if (ply < 2 || !nrQuiets) /// we can't update if we don't have a follow move or no quiets
        return;

    uint16_t counterMove = (stack - 1)->move;
    int counterPiece = (stack - 1)->piece;
    int counterTo = sqTo(counterMove);

    uint16_t best = stack->quiets[nrQuiets - 1];
    bool turn = searcher->board.turn;

    if (counterMove)
        searcher->cmTable[1 ^ turn][counterPiece][counterTo] = best; /// update counter move table

    for (int i = 0; i < nrQuiets - 1; i++)
        updateMoveHistory(searcher, stack, stack->quiets[i], -bonus);
    updateMoveHistory(searcher, stack, best, bonus);
}

void updateCapHistory(Search* searcher, StackEntry* stack, int nrCaptures, uint16_t best, int ply, int bonus) {
    for (int i = 0; i < nrCaptures; i++) {
        int move = stack->captures[i];
        int score = (move == best ? bonus : -bonus);
        int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from), cap = searcher->board.piece_type_at(to);

        if (type(move) == ENPASSANT)
            cap = PAWN;

        updateCapHist(searcher->capHist[piece][to][cap], score);
    }
}

int getCapHist(Search* searcher, uint16_t move) {
    int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from), cap = searcher->board.piece_type_at(to);

    if (type(move) == ENPASSANT)
        cap = PAWN;

    return searcher->capHist[piece][to][cap];
}

void getHistory(Search* searcher, StackEntry* stack, uint16_t move, int &h, int &ch, int &fh) {
    int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from);

    h = searcher->hist[searcher->board.turn][from][to];

    ch = (*(stack - 1)->continuationHist)[piece][to];
    
    fh = (*(stack - 2)->continuationHist)[piece][to];
}
