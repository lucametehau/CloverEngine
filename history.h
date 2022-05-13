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

struct Heuristics {
    Heuristics() : h(0), ch(0), fh(0) {}
    int h, ch, fh;
};


int histMax = 400; // 1000

int histMult = 32; // 50
int histUpdateDiv = 512; // 300

int counterHistMult = 32;
int counterHistUpdateDiv = 512;

int capHistMult = 32;
int capHistUpdateDiv = 512;


void updateHist(int& hist, int score) {
    hist += score * histMult - hist * abs(score) / histUpdateDiv;
}

void updateCounterHist(int& hist, int score) {
    hist += score * counterHistMult - hist * abs(score) / counterHistUpdateDiv;
}

void updateCapHist(int& hist, int score) {
    hist += score * capHistMult - hist * abs(score) / capHistUpdateDiv;
}

void updateHistory(Search* searcher, uint16_t* quiets, int nrQuiets, int ply, int bonus) {
    if (ply < 2 || !nrQuiets) /// we can't update if we don't have a follow move or no quiets
        return;

    uint16_t counterMove = searcher->Stack[ply - 1].move, followMove = searcher->Stack[ply - 2].move;
    int counterPiece = searcher->Stack[ply - 1].piece, followPiece = searcher->Stack[ply - 2].piece;
    int counterTo = sqTo(counterMove), followTo = sqTo(followMove);

    uint16_t best = quiets[nrQuiets - 1];
    bool turn = searcher->board.turn;

    if (counterMove)
        searcher->cmTable[1 ^ turn][counterPiece][counterTo] = best; /// update counter move table

    bonus = std::min(bonus, histMax);

    for (int i = 0; i < nrQuiets; i++) {
        /// increase value for best move, decrease value for the other moves
        /// so we have an early cut-off

        int move = quiets[i];
        int score = (move == best ? bonus : -bonus);
        int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from);

        updateHist(searcher->hist[turn][from][to], score);

        if (counterMove)
            updateCounterHist(searcher->follow[0][counterPiece][counterTo][piece][to], score);

        if (followMove)
            updateCounterHist(searcher->follow[1][followPiece][followTo][piece][to], score);
    }
}

void updateCapHistory(Search* searcher, uint16_t* captures, int nrCaptures, uint16_t best, int ply, int bonus) {
    bonus = std::min(bonus, histMax);

    for (int i = 0; i < nrCaptures; i++) {
        /// increase value for best move, decrease value for the other moves
        /// so we have an early cut-off

        int move = captures[i];
        int score = (move == best ? bonus : -bonus);
        int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from), cap = searcher->board.piece_type_at(to);

        if (type(move) == ENPASSANT)
            cap = PAWN;

        updateCapHist(searcher->capHist[piece][to][cap], score);
    }
}

void getHistory(Search* searcher, uint16_t move, int ply, Heuristics& H) {
    int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from);

    H.h = searcher->hist[searcher->board.turn][from][to];

    uint16_t counterMove = searcher->Stack[ply - 1].move, followMove = (ply >= 2 ? searcher->Stack[ply - 2].move : NULLMOVE);
    int counterPiece = searcher->Stack[ply - 1].piece, followPiece = (ply >= 2 ? searcher->Stack[ply - 2].piece : 0);
    int counterTo = sqTo(counterMove), followTo = sqTo(followMove);

    H.ch = searcher->follow[0][counterPiece][counterTo][piece][to];

    H.fh = searcher->follow[1][followPiece][followTo][piece][to];
}
