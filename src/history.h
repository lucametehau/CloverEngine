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

void updateHist(int16_t& hist, int16_t score) {
    hist += score - hist * abs(score) / 16384;
}

void updateCounterHist(int16_t& hist, int16_t score) {
    hist += score - hist * abs(score) / 16384;
}

void updateCapHist(int16_t& hist, int16_t score) {
    hist += score - hist * abs(score) / 16384;
}

int getHistoryBonus(int depth) {
    return std::min<int>(HistoryBonusMargin * depth - HistoryBonusBias, HistoryBonusMax);
}

void updateMoveHistory(Search* searcher, StackEntry*& stack, Move move, uint64_t threats, int16_t bonus) {
    const int from = sq_from(move), to = sq_to(move), piece = searcher->board.piece_at(from);

    updateHist(searcher->hist[searcher->board.turn][!!(threats & (1ULL << from))][!!(threats & (1ULL << to))][fromTo(move)], bonus);

    if ((stack - 1)->move)
        updateCounterHist((*(stack - 1)->cont_hist)[piece][to], bonus);

    if ((stack - 2)->move)
        updateCounterHist((*(stack - 2)->cont_hist)[piece][to], bonus);

    if ((stack - 4)->move)
        updateCounterHist((*(stack - 4)->cont_hist)[piece][to], bonus);
}

void updateCaptureMoveHistory(Search* searcher, Move move, int16_t bonus) {
    assert(type(move) != CASTLE);
    const int to = sq_to(move), from = sq_from(move), cap = (type(move) == ENPASSANT ? PAWN : searcher->board.piece_type_at(to)), piece = searcher->board.piece_at(from);

    updateCapHist(searcher->cap_hist[piece][to][cap], bonus);
}

void updateHistory(Search* searcher, StackEntry* stack, int nrQuiets, int ply, uint64_t threats, int16_t bonus) {
    if (!nrQuiets) /// we can't update if we don't have a follow move or no quiets
        return;

    const Move counterMove = (stack - 1)->move;
    const int counterPiece = (stack - 1)->piece;
    const int counterTo = sq_to(counterMove);
    const Move best = stack->quiets[nrQuiets - 1];

    if (counterMove)
        searcher->counter_move[counterPiece][counterTo] = best; /// update counter move table

    for (int i = 0; i < nrQuiets - 1; i++)
        updateMoveHistory(searcher, stack, stack->quiets[i], threats, -bonus);
    updateMoveHistory(searcher, stack, best, threats, bonus);
}

void updateCapHistory(Search* searcher, StackEntry* stack, int nrCaptures, Move best, int ply, int16_t bonus) {
    for (int i = 0; i < nrCaptures; i++) {
        const int move = stack->captures[i], score = (move == best ? bonus : -bonus);
        const int from = sq_from(move), to = sq_to(move), piece = searcher->board.piece_at(from);
        const int cap = (type(move) == ENPASSANT ? PAWN : searcher->board.piece_type_at(to));
        updateCapHist(searcher->cap_hist[piece][to][cap], score);
    }
}

int16_t getCapHist(Search* searcher, Move move) {
    const int from = sq_from(move), to = sq_to(move), piece = searcher->board.piece_at(from);
    const int cap = (type(move) == ENPASSANT ? PAWN : searcher->board.piece_type_at(to));
    return searcher->cap_hist[piece][to][cap];
}

void getHistory(Search* searcher, StackEntry* stack, Move move, uint64_t threats, int &hist) {
    const int from = sq_from(move), to = sq_to(move), piece = searcher->board.piece_at(from);

    hist = 2 * searcher->hist[searcher->board.turn][!!(threats & (1ULL << from))][!!(threats & (1ULL << to))][fromTo(move)];

    hist += 2 * (*(stack - 1)->cont_hist)[piece][to];

    hist += (*(stack - 2)->cont_hist)[piece][to];

    hist += (*(stack - 4)->cont_hist)[piece][to];
}
