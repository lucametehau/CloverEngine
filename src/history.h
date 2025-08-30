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

constexpr int KP_MOVE_SIZE = (1 << 14);
constexpr int KP_MOVE_MASK = KP_MOVE_SIZE - 1;

class MainHistory
{
  private:
    int16_t hist;

  public:
    constexpr MainHistory() = default;
    constexpr MainHistory(int16_t hist) : hist(hist)
    {
    }

    static int16_t bonus(int depth)
    {
        return std::min<int>(MainHistoryBonusMargin * depth - MainHistoryBonusBias, MainHistoryBonusMax);
    }

    static int16_t malus(int depth)
    {
        return -std::min<int>(MainHistoryMalusMargin * depth - MainHistoryMalusBias, MainHistoryMalusMax);
    }

    void update(int16_t score)
    {
        hist += score - std::abs(score) * hist / MainHistoryDivisor;
    }

    constexpr operator int16_t() const
    {
        return hist;
    }

    MainHistory &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

class ContinuationHistory
{
  private:
    int16_t hist;

  public:
    constexpr ContinuationHistory() = default;
    constexpr ContinuationHistory(int16_t hist) : hist(hist)
    {
    }

    static int16_t bonus(int depth)
    {
        return std::min<int>(ContinuationHistoryBonusMargin * depth - ContinuationHistoryBonusBias,
                             ContinuationHistoryBonusMax);
    }

    static int16_t malus(int depth)
    {
        return -std::min<int>(ContinuationHistoryMalusMargin * depth - ContinuationHistoryMalusBias,
                              ContinuationHistoryMalusMax);
    }

    void update(int16_t score)
    {
        hist += score - std::abs(score) * hist / ContinuationHistoryDivisor;
    }

    constexpr operator int16_t() const
    {
        return hist;
    }

    ContinuationHistory &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

class CaptureHistory
{
  private:
    int16_t hist;

  public:
    constexpr CaptureHistory() = default;
    constexpr CaptureHistory(int16_t hist) : hist(hist)
    {
    }

    static int16_t bonus(int depth)
    {
        return std::min<int>(CaptureHistoryBonusMargin * depth - CaptureHistoryBonusBias, CaptureHistoryBonusMax);
    }

    static int16_t malus(int depth)
    {
        return -std::min<int>(CaptureHistoryMalusMargin * depth - CaptureHistoryMalusBias, CaptureHistoryMalusMax);
    }

    void update(int16_t score)
    {
        hist += score - std::abs(score) * hist / CaptureHistoryDivisor;
    }

    constexpr operator int16_t() const
    {
        return hist;
    }

    CaptureHistory &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

class PawnHistory
{
  private:
    int16_t hist;

  public:
    static constexpr int SIZE = (1 << 12);
    static constexpr int MASK = SIZE - 1;

    constexpr PawnHistory() = default;
    constexpr PawnHistory(int16_t hist) : hist(hist)
    {
    }

    static int16_t bonus(int depth)
    {
        return std::min<int>(PawnHistoryBonusMargin * depth - PawnHistoryBonusBias, PawnHistoryBonusMax);
    }

    static int16_t malus(int depth)
    {
        return -std::min<int>(PawnHistoryMalusMargin * depth - PawnHistoryMalusBias, PawnHistoryMalusMax);
    }

    void update(int16_t score)
    {
        hist += score - std::abs(score) * hist / PawnHistoryDivisor;
    }

    constexpr operator int16_t() const
    {
        return hist;
    }

    PawnHistory &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

class CorrectionHistory
{
  private:
    int16_t hist;

  public:
    static constexpr int SIZE = (1 << 16);
    static constexpr int MASK = SIZE - 1;

    constexpr CorrectionHistory() = default;
    constexpr CorrectionHistory(int16_t hist) : hist(hist)
    {
    }

    void update(int16_t score)
    {
        hist += score - std::abs(score) * hist / CorrectionHistoryDivisor;
    }

    constexpr operator int16_t() const
    {
        return hist;
    }

    CorrectionHistory &operator=(int16_t value)
    {
        hist = value;
        return *this;
    }
};

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
    MultiArray<ContinuationHistory, 12, 64> *cont_hist;
    MultiArray<CorrectionHistory, 12, 64> *cont_corr_hist;
    Bitboard threats;
};

class Histories
{
  private:
    MultiArray<MainHistory, 2, 2, 2, 64 * 64> hist;
    MultiArray<CaptureHistory, 2, 12, 64, 7> cap_hist;
    MultiArray<PawnHistory, PawnHistory::SIZE, 12, 64> pawn_hist;
    MultiArray<CorrectionHistory, CorrectionHistory::SIZE, 2> corr_hist;
    MultiArray<CorrectionHistory, 2, 2, CorrectionHistory::SIZE> mat_corr_hist;

  public:
    MultiArray<ContinuationHistory, 2, 12, 64, 12, 64> cont_history;
    MultiArray<CorrectionHistory, 12, 64, 12, 64> cont_corr_hist;

  public:
    void clear_history()
    {
        fill_multiarray<MainHistory, 2, 2, 2, 64 * 64>(hist, 0);
        fill_multiarray<CaptureHistory, 2, 12, 64, 7>(cap_hist, 0);
        fill_multiarray<PawnHistory, PawnHistory::SIZE, 12, 64>(pawn_hist, 0);
        fill_multiarray<ContinuationHistory, 2, 12, 64, 12, 64>(cont_history, 0);
        fill_multiarray<CorrectionHistory, CorrectionHistory::SIZE, 2>(corr_hist, CorrectionHistory(0));
        fill_multiarray<CorrectionHistory, 2, 2, CorrectionHistory::SIZE>(mat_corr_hist, CorrectionHistory(0));
        fill_multiarray<CorrectionHistory, 12, 64, 12, 64>(cont_corr_hist, CorrectionHistory(0));
    }

    Histories()
    {
        clear_history();
    }

    MainHistory &get_hist(const Square from, const Square to, const int from_to, const bool turn,
                          const Bitboard threats)
    {
        return hist[turn][!threats.has_square(from)][!threats.has_square(to)][from_to];
    }

    constexpr MainHistory get_hist(const Square from, const Square to, const int from_to, const bool turn,
                                   const Bitboard threats) const
    {
        return hist[turn][!threats.has_square(from)][!threats.has_square(to)][from_to];
    }

    ContinuationHistory &get_cont_hist(const Piece piece, const Square to, StackEntry *stack, const int delta)
    {
        return (*(stack - delta)->cont_hist)[piece][to];
    }

    constexpr ContinuationHistory get_cont_hist(const Piece piece, const Square to, StackEntry *stack,
                                                const int delta) const
    {
        return (*(stack - delta)->cont_hist)[piece][to];
    }

    CaptureHistory &get_cap_hist(const Piece piece, const Square to, const int cap, const Bitboard threats)
    {
        return cap_hist[!threats.has_square(to)][piece][to][cap];
    }

    constexpr CaptureHistory get_cap_hist(const Piece piece, const Square to, const int cap,
                                          const Bitboard threats) const
    {
        return cap_hist[!threats.has_square(to)][piece][to][cap];
    }

    PawnHistory &get_pawn_hist(const Piece piece, const Square to, const Key pawn_key)
    {
        return pawn_hist[pawn_key & PawnHistory::MASK][piece][to];
    }

    constexpr PawnHistory get_pawn_hist(const Piece piece, const Square to, const Key pawn_key) const
    {
        return pawn_hist[pawn_key & PawnHistory::MASK][piece][to];
    }

    CorrectionHistory &get_corr_hist(const bool turn, const Key pawn_key)
    {
        return corr_hist[pawn_key & CorrectionHistory::MASK][turn];
    }

    constexpr CorrectionHistory get_corr_hist(const bool turn, const Key pawn_key) const
    {
        return corr_hist[pawn_key & CorrectionHistory::MASK][turn];
    }

    CorrectionHistory &get_mat_corr_hist(const bool turn, const bool side, const Key mat_key)
    {
        return mat_corr_hist[turn][side][mat_key & CorrectionHistory::MASK];
    }

    constexpr CorrectionHistory get_mat_corr_hist(const bool turn, const bool side, const Key mat_key) const
    {
        return mat_corr_hist[turn][side][mat_key & CorrectionHistory::MASK];
    }

    CorrectionHistory &get_cont_corr_hist(StackEntry *stack, const int delta)
    {
        return (*(stack - delta)->cont_corr_hist)[(stack - 1)->piece][(stack - 1)->move.get_to()];
    }

    constexpr CorrectionHistory get_cont_corr_hist(StackEntry *stack, const int delta) const
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

    void update_main_hist_move(const Move move, const Bitboard threats, const bool turn, const int16_t bonus)
    {
        get_hist(move.get_from(), move.get_to(), move.get_from_to(), turn, threats).update(bonus);
    }

    void update_pawn_hist_move(const Piece piece, const Square to, const Key pawn_key, const int16_t bonus)
    {
        get_pawn_hist(piece, to, pawn_key).update(bonus);
    }

    constexpr int get_history_search(const Move move, const Piece piece, const Bitboard threats, const bool turn,
                                     StackEntry *stack, const Key pawn_key) const
    {
        const Square to = move.get_to();
        return (QuietSearchHistCoef * get_hist(move.get_from(), to, move.get_from_to(), turn, threats) +
                QuietSearchContHist1 * get_cont_hist(piece, to, stack, 1) +
                QuietSearchContHist2 * get_cont_hist(piece, to, stack, 2) +
                QuietSearchContHist4 * get_cont_hist(piece, to, stack, 4)) /
               1024;
    }

    constexpr int get_history_movepick(const Move move, const Piece piece, const Bitboard threats, const bool turn,
                                       StackEntry *stack, const Key pawn_key) const
    {
        const Square to = move.get_to();
        return (QuietMPHistCoef * get_hist(move.get_from(), to, move.get_from_to(), turn, threats) +
                QuietMPContHist1 * get_cont_hist(piece, to, stack, 1) +
                QuietMPContHist2 * get_cont_hist(piece, to, stack, 2) +
                QuietMPContHist4 * get_cont_hist(piece, to, stack, 4) +
                QuietMPPawnHist * get_pawn_hist(piece, to, pawn_key)) /
               1024;
    }

    void update_cap_hist_move(const Piece piece, const Square to, const int cap, const Bitboard threats,
                              const int16_t bonus)
    {
        get_cap_hist(piece, to, cap, threats).update(bonus);
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

    __attribute__((always_inline)) constexpr int get_corrected_eval(const int eval, const bool turn, const Key pawn_key,
                                                                    const Key white_mat_key, const Key black_mat_key,
                                                                    StackEntry *stack) const
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
                correction += CorrHistCont4 * get_cont_corr_hist(stack, 4);
        }
        return eval + correction / (16 * 1024);
    }
};