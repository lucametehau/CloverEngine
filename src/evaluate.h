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
#include "board.h"
#include "defs.h"
#include "net.h"

#ifndef TUNE_FLAG
constexpr int seeVal[] = {SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0};
#else
int seeVal[] = {SeeValPawn, SeeValKnight, SeeValBishop, SeeValRook, SeeValQueen, 20000, 0};
#endif

inline int scale(Board &board)
{
    return EvalScaleBias +
           (board.get_bb_piece_type(PieceTypes::PAWN).count() * seeVal[PieceTypes::PAWN] +
            board.get_bb_piece_type(PieceTypes::KNIGHT).count() * seeVal[PieceTypes::KNIGHT] +
            board.get_bb_piece_type(PieceTypes::BISHOP).count() * seeVal[PieceTypes::BISHOP] +
            board.get_bb_piece_type(PieceTypes::ROOK).count() * seeVal[PieceTypes::ROOK] +
            board.get_bb_piece_type(PieceTypes::QUEEN).count() * seeVal[PieceTypes::QUEEN]) /
               32 -
           board.half_moves() * EvalShuffleCoef;
}

int evaluate(Board &board, Network &NN)
{
    NN.bring_up_to_date(board);

    int eval = NN.get_output(board.turn, board.get_output_bucket());
    eval = eval * scale(board) / 1024;
    return eval;
}