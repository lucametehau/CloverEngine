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

constexpr int CORR_HIST_SIZE = (1 << 16);
constexpr int CORR_HIST_MASK = CORR_HIST_SIZE - 1;

class Histories {
private:
    MultiArray<History<16384>, 2, 2, 2, 64 * 64> hist;
    MultiArray<History<16384>, 13, 64, 7> cap_hist;
    MultiArray<int, 2, CORR_HIST_SIZE> corr_hist;

public:
    MultiArray<History<16384>, 2, 13, 64, 13, 64> cont_history;

public:
    void clear_history() {
        fill_multiarray<History<16384>, 2, 2, 2, 64 * 64>(hist, 0);
        fill_multiarray<History<16384>, 13, 64, 7>(cap_hist, 0);
        fill_multiarray<History<16384>, 2, 13, 64, 13, 64>(cont_history, 0);
        fill_multiarray<int, 2, CORR_HIST_SIZE>(corr_hist, 0);
    }

    Histories() { clear_history(); }

    inline History<16384>& get_hist(const int from, const int to, const int from_to, const bool turn, const uint64_t threats) {
        return hist[turn][!!(threats & (1ULL << from))][!!(threats & (1ULL << to))][from_to];
    }

    inline History<16384>& get_cont_hist(const int piece, const int to, StackEntry *stack, const int delta) {
        return (*(stack - delta)->cont_hist)[piece][to];
    }

    inline History<16384>& get_cap_hist(const int piece, const int to, const int cap) {
        return cap_hist[piece][to][cap];
    }

    inline int& get_corr_hist(const bool turn, const uint64_t pawn_key) {
        return corr_hist[turn][pawn_key & CORR_HIST_MASK];
    }

    inline void update_cont_hist_move(const int piece, const int to, StackEntry *&stack, const int16_t bonus) {
        if ((stack - 1)->move)
            get_cont_hist(piece, to, stack, 1).update(bonus);
        if ((stack - 2)->move)
            get_cont_hist(piece, to, stack, 2).update(bonus);
        if ((stack - 4)->move)
            get_cont_hist(piece, to, stack, 4).update(bonus);
    }

    inline void update_hist_move(const Move move, const uint64_t threats, const bool turn, const int16_t bonus) {
        get_hist(sq_from(move), sq_to(move), from_to(move), turn, threats).update(bonus);
    }

    inline void update_hist_quiet_move(const Move move, const int piece, const uint64_t threats, const bool turn, StackEntry *&stack, const int16_t bonus) {
        update_hist_move(move, threats, turn, bonus);
        update_cont_hist_move(piece, sq_to(move), stack, bonus);
    }

    inline int get_history_search(const Move move, const int piece, const uint64_t threats, const bool turn, StackEntry *stack) {
        const int to = sq_to(move);
        return get_hist(sq_from(move), to, from_to(move), turn, threats)
             + get_cont_hist(piece, to, stack, 1)
             + get_cont_hist(piece, to, stack, 2)
             + get_cont_hist(piece, to, stack, 4);
    }

    inline int get_history_movepick(const Move move, const int piece, const uint64_t threats, const bool turn, StackEntry *stack) {
        const int to = sq_to(move);
        return QuietHistCoef  * get_hist(sq_from(move), to, from_to(move), turn, threats)
             + QuietContHist1 * get_cont_hist(piece, to, stack, 1)
             + QuietContHist2 * get_cont_hist(piece, to, stack, 2)
             + QuietContHist4 * get_cont_hist(piece, to, stack, 4);
    }

    inline void update_cap_hist_move(const int piece, const int to, const int cap, const int16_t bonus) {
        get_cap_hist(piece, to, cap).update(bonus);
    }

    inline void update_corr_hist(const bool turn, const uint64_t pawn_key, const int depth, const int delta) {
        const int w = std::min(4 * (depth + 1) * (depth + 1), 1024);
        int& corr = get_corr_hist(turn, pawn_key);
        corr = (corr * (CorrHistScale - w) + delta * CorrHistDiv * w) / CorrHistScale;
        corr = std::clamp(corr, -32 * CorrHistDiv, 32 * CorrHistDiv);
    }

    inline int get_corrected_eval(const int eval, const bool turn, const uint64_t pawn_key) {
        return eval + get_corr_hist(turn, pawn_key) / CorrHistDiv;
    }
};

int getHistoryBonus(int depth) {
    return std::min<int>(HistoryBonusMargin * depth - HistoryBonusBias, HistoryBonusMax);
}