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
#include <algorithm>
#include <vector>

constexpr int CORR_HIST_SIZE = (1 << 16);
constexpr int CORR_HIST_MASK = CORR_HIST_SIZE - 1;

constexpr int KP_MOVE_SIZE = (1 << 14);
constexpr int KP_MOVE_MASK = KP_MOVE_SIZE - 1;

constexpr int PAWN_HIST_SIZE = (1 << 12);
constexpr int PAWN_HIST_MASK = PAWN_HIST_SIZE - 1;

template <int16_t Divisor> class History
{
  public:
    int16_t hist;
    constexpr History() = default;
    constexpr History(int16_t hist) : hist(hist)
    {
    }

    void update(int16_t score)
    {
        hist += score - hist * abs(score) / Divisor;
    }

    operator int16_t() const
    {
        return hist;
    }

    History &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

typedef History<1024> CorrectionHistory;

class SearchMove
{
  public:
    Move move;
    int tried_count;
    constexpr SearchMove() = default;
    constexpr SearchMove(Move _move, int _tried_count) : move(_move), tried_count(_tried_count)
    {
    }
};

class StackEntry
{ /// info to keep in the stack
  public:
    constexpr StackEntry()
        : piece(NO_PIECE), move(NULLMOVE), killer(NULLMOVE), excluded(NULLMOVE), eval(0), R(0), cutoff_cnt(0),
          threats(0)
    {
    }
    Piece piece;
    Move move, killer, excluded;
    int eval;
    int R;          // reduction
    int cutoff_cnt; // number of cutoffs in the current search
    MultiArray<History<16384>, 13, 64> *cont_hist;
    MultiArray<CorrectionHistory, 13, 64> *cont_corr_hist;
    Bitboard threats;
};

class Histories
{
  private:
    MultiArray<History<16384>, 2, 2, 2, 64 * 64> hist;
    MultiArray<History<16384>, 2, 2, 64> from_hist, to_hist;
    MultiArray<History<16384>, 12, 64, 7> cap_hist;
    MultiArray<History<16384>, PAWN_HIST_SIZE, 12, 64> pawn_hist;
    MultiArray<CorrectionHistory, 2, CORR_HIST_SIZE> corr_hist;
    MultiArray<CorrectionHistory, 2, 2, CORR_HIST_SIZE> mat_corr_hist;

  public:
    MultiArray<History<16384>, 2, 13, 64, 13, 64> cont_history;
    MultiArray<CorrectionHistory, 13, 64, 13, 64> cont_corr_hist;

  public:
    void clear_history()
    {
        fill_multiarray<History<16384>, 2, 2, 2, 64 * 64>(hist, 0);
        fill_multiarray<History<16384>, 2, 2, 64>(from_hist, 0);
        fill_multiarray<History<16384>, 2, 2, 64>(to_hist, 0);
        fill_multiarray<History<16384>, 12, 64, 7>(cap_hist, 0);
        fill_multiarray<History<16384>, PAWN_HIST_SIZE, 12, 64>(pawn_hist, 0);
        fill_multiarray<History<16384>, 2, 13, 64, 13, 64>(cont_history, 0);
        fill_multiarray<CorrectionHistory, 2, CORR_HIST_SIZE>(corr_hist, CorrectionHistory(0));
        fill_multiarray<CorrectionHistory, 2, 2, CORR_HIST_SIZE>(mat_corr_hist, CorrectionHistory(0));
        fill_multiarray<CorrectionHistory, 13, 64, 13, 64>(cont_corr_hist, CorrectionHistory(0));
    }

    Histories()
    {
        clear_history();
    }

    History<16384> &get_hist(const Square from, const Square to, const int from_to, const bool turn,
                             const Bitboard threats)
    {
        return hist[turn][!threats.has_square(from)][!threats.has_square(to)][from_to];
    }

    const History<16384> get_hist(const Square from, const Square to, const int from_to, const bool turn,
                                  const Bitboard threats) const
    {
        return hist[turn][!threats.has_square(from)][!threats.has_square(to)][from_to];
    }

    History<16384> &get_from_hist(const Square square, const bool turn, const Bitboard threats)
    {
        return from_hist[turn][!threats.has_square(square)][square];
    }

    const History<16384> get_from_hist(const Square square, const bool turn, const Bitboard threats) const
    {
        return from_hist[turn][!threats.has_square(square)][square];
    }

    History<16384> &get_to_hist(const Square square, const bool turn, const Bitboard threats)
    {
        return to_hist[turn][!threats.has_square(square)][square];
    }

    const History<16384> get_to_hist(const Square square, const bool turn, const Bitboard threats) const
    {
        return to_hist[turn][!threats.has_square(square)][square];
    }

    History<16384> &get_cont_hist(const Piece piece, const Square to, StackEntry *stack, const int delta)
    {
        return (*(stack - delta)->cont_hist)[piece][to];
    }

    const History<16384> get_cont_hist(const Piece piece, const Square to, StackEntry *stack, const int delta) const
    {
        return (*(stack - delta)->cont_hist)[piece][to];
    }

    History<16384> &get_cap_hist(const Piece piece, const Square to, const int cap)
    {
        return cap_hist[piece][to][cap];
    }

    const History<16384> get_cap_hist(const Piece piece, const Square to, const int cap) const
    {
        return cap_hist[piece][to][cap];
    }

    History<16384> &get_pawn_hist(const Piece piece, const Square to, const Key pawn_key)
    {
        return pawn_hist[pawn_key & PAWN_HIST_MASK][piece][to];
    }

    const History<16384> get_pawn_hist(const Piece piece, const Square to, const Key pawn_key) const
    {
        return pawn_hist[pawn_key & PAWN_HIST_MASK][piece][to];
    }

    CorrectionHistory &get_corr_hist(const bool turn, const Key pawn_key)
    {
        return corr_hist[turn][pawn_key & CORR_HIST_MASK];
    }

    const CorrectionHistory get_corr_hist(const bool turn, const Key pawn_key) const
    {
        return corr_hist[turn][pawn_key & CORR_HIST_MASK];
    }

    CorrectionHistory &get_mat_corr_hist(const bool turn, const bool side, const Key mat_key)
    {
        return mat_corr_hist[turn][side][mat_key & CORR_HIST_MASK];
    }

    const CorrectionHistory get_mat_corr_hist(const bool turn, const bool side, const Key mat_key) const
    {
        return mat_corr_hist[turn][side][mat_key & CORR_HIST_MASK];
    }

    CorrectionHistory &get_cont_corr_hist(StackEntry *stack, const int delta)
    {
        return (*(stack - delta)->cont_corr_hist)[(stack - 1)->piece][(stack - 1)->move.get_to()];
    }

    const CorrectionHistory get_cont_corr_hist(StackEntry *stack, const int delta) const
    {
        return (*(stack - delta)->cont_corr_hist)[(stack - 1)->piece][(stack - 1)->move.get_to()];
    }

    void update_cont_hist_move(const Piece piece, const Square to, StackEntry *stack, const int16_t bonus)
    {
        if ((stack - 1)->move)
            get_cont_hist(piece, to, stack, 1).update(bonus);
        if ((stack - 2)->move)
            get_cont_hist(piece, to, stack, 2).update(bonus);
        if ((stack - 4)->move)
            get_cont_hist(piece, to, stack, 4).update(bonus);
    }

    void update_hist_move(const Move move, const Bitboard threats, const bool turn, const int16_t bonus)
    {
        const Square from = move.get_from();
        const Square to = move.get_to();
        get_hist(from, to, move.get_from_to(), turn, threats).update(bonus);
        get_from_hist(from, turn, threats).update(bonus);
        get_to_hist(to, turn, threats).update(bonus);
    }

    void update_pawn_hist_move(const Piece piece, const Square to, const Key pawn_key, const int16_t bonus)
    {
        get_pawn_hist(piece, to, pawn_key).update(bonus);
    }

    void update_hist_quiet_move(const Move move, const Piece piece, const Bitboard threats, const bool turn,
                                StackEntry *&stack, const Key pawn_key, const int16_t bonus)
    {
        update_hist_move(move, threats, turn, bonus);
        update_cont_hist_move(piece, move.get_to(), stack, bonus);
        update_pawn_hist_move(piece, move.get_to(), pawn_key, bonus);
    }

    const int get_history_search(const Move move, const Piece piece, const Bitboard threats, const bool turn,
                                 StackEntry *stack, const Key pawn_key) const
    {
        const Square from = move.get_from();
        const Square to = move.get_to();
        return get_hist(from, to, move.get_from_to(), turn, threats) + get_from_hist(from, turn, threats) / 2 +
               get_to_hist(to, turn, threats) / 2 + get_cont_hist(piece, to, stack, 1) +
               get_cont_hist(piece, to, stack, 2) + get_cont_hist(piece, to, stack, 4);
    }

    const int get_history_movepick(const Move move, const Piece piece, const Bitboard threats, const bool turn,
                                   StackEntry *stack, const Key pawn_key) const
    {
        const Square to = move.get_to();
        return (QuietHistCoef * get_hist(move.get_from(), to, move.get_from_to(), turn, threats) +
                QuietContHist1 * get_cont_hist(piece, to, stack, 1) +
                QuietContHist2 * get_cont_hist(piece, to, stack, 2) +
                QuietContHist4 * get_cont_hist(piece, to, stack, 4) +
                QuietPawnHist * get_pawn_hist(piece, to, pawn_key)) /
               1024;
    }

    void update_cap_hist_move(const Piece piece, const Square to, const int cap, const int16_t bonus)
    {
        get_cap_hist(piece, to, cap).update(bonus);
    }

    void update_corr_hist(const bool turn, const Key pawn_key, const Key white_mat_key, const Key black_mat_key,
                          StackEntry *stack, const int depth, const int delta)
    {
        const int bonus = std::clamp(delta * depth / 8, -256, 256);
        get_corr_hist(turn, pawn_key).update(bonus);
        get_mat_corr_hist(turn, WHITE, white_mat_key).update(bonus);
        get_mat_corr_hist(turn, BLACK, black_mat_key).update(bonus);
        if ((stack - 1)->move)
        {
            if ((stack - 2)->move)
                get_cont_corr_hist(stack, 2).update(bonus);
            if ((stack - 3)->move)
                get_cont_corr_hist(stack, 3).update(bonus);
            if ((stack - 4)->move)
                get_cont_corr_hist(stack, 4).update(bonus);
        }
    }

    const int get_corrected_eval(const int eval, const bool turn, const Key pawn_key, const Key white_mat_key,
                                 const Key black_mat_key, StackEntry *stack) const
    {
        int correction = CorrHistPawn * get_corr_hist(turn, pawn_key) +
                         CorrHistMat * get_mat_corr_hist(turn, WHITE, white_mat_key) +
                         CorrHistMat * get_mat_corr_hist(turn, BLACK, black_mat_key);
        if ((stack - 1)->move)
        {
            if ((stack - 2)->move)
                correction += CorrHistCont2 * get_cont_corr_hist(stack, 2);
            if ((stack - 3)->move)
                correction += CorrHistCont3 * get_cont_corr_hist(stack, 3);
            if ((stack - 4)->move)
                correction += CorrHistCont3 * get_cont_corr_hist(stack, 4);
        }
        return eval + correction / (16 * 1024);
    }
};

inline int getHistoryBonus(int depth)
{
    return std::min<int>(HistoryBonusMargin * depth - HistoryBonusBias, HistoryBonusMax);
}

inline int getHistoryMalus(int depth)
{
    return -std::min<int>(HistoryMalusMargin * depth - HistoryMalusBias, HistoryMalusMax);
}