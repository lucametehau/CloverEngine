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
#include "3rdparty/Fathom/src/tbprobe.h"
#include "movepick.h"
#include "thread.h"
#include "tt.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>

template <bool checkTime> bool SearchThread::check_for_stop()
{
    if (!main_thread())
        return 0;

    if (must_stop())
        return 1;

    if (info.nodes_limit_passed(nodes) || info.max_nodes_passed(nodes))
    {
        state |= ThreadStates::STOP;
        return 1;
    }

    time_check_count++;
    if (time_check_count == (1 << 10))
    {
        if constexpr (checkTime)
        {
            if (info.hard_limit_passed())
                state |= ThreadStates::STOP;
        }
        time_check_count = 0;
    }

    return must_stop();
}

uint32_t probe_TB(Board &board, int depth)
{
    if (TB_LARGEST && depth >= 2 && !board.half_moves())
    {
        if (static_cast<uint32_t>((board.get_bb_color(WHITE) | board.get_bb_color(BLACK)).count()) <= TB_LARGEST)
            return tb_probe_wdl(board.get_bb_color(WHITE), board.get_bb_color(BLACK),
                                board.get_bb_piece_type(PieceTypes::KING), board.get_bb_piece_type(PieceTypes::QUEEN),
                                board.get_bb_piece_type(PieceTypes::ROOK), board.get_bb_piece_type(PieceTypes::BISHOP),
                                board.get_bb_piece_type(PieceTypes::KNIGHT), board.get_bb_piece_type(PieceTypes::PAWN),
                                0, 0, board.enpas() == NO_SQUARE ? static_cast<Square>(0) : board.enpas(), board.turn);
    }
    return TB_RESULT_FAILED;
}

std::string getSanString(Board &board, Move move, HistoricalState &next_state)
{
    if (move.get_type() == MoveTypes::CASTLE)
        return move.get_to() > move.get_from() ? "O-O" : "O-O-O";
    int from = move.get_from(), to = move.get_to();
    Piece prom = move.is_promo() ? move.get_prom() + PieceTypes::KNIGHT + 6 : static_cast<Piece>(0),
          piece = board.piece_type_at(from);
    std::string san;

    if (piece != PieceTypes::PAWN)
        san += piece_char[piece + 6];
    else
        san += char('a' + (from & 7));
    if (board.is_capture(move))
        san += "x";
    if (piece != PieceTypes::PAWN || board.is_capture(move))
        san += char('a' + (to & 7));

    san += char('1' + (to >> 3));

    if (prom)
        san += "=", san += piece_char[prom];

    board.make_move(move, next_state);
    if (board.checkers())
        san += '+';
    board.undo_move(move);

    return san;
}

void SearchThread::print_pv()
{
    if (info.is_san_mode())
    {
        std::unique_ptr<std::deque<HistoricalState>> states = std::make_unique<std::deque<HistoricalState>>(0);
        for (int i = 0; i < pv_table_len[0]; i++)
        {
            states->emplace_back();
            std::cout << getSanString(board, pv_table[0][i], states->back()) << " ";
            board.make_move(pv_table[0][i], states->back());
        }
        for (int i = pv_table_len[0] - 1; i >= 0; i--)
            board.undo_move(pv_table[0][i]);
    }
    else
    {
        for (int i = 0; i < pv_table_len[0]; i++)
        {
            std::cout << pv_table[0][i].to_string(board.chess960) << " ";
        }
    }
}

void SearchThread::update_pv(int ply, Move move)
{
    pv_table[ply][0] = move;
    for (int i = 0; i < pv_table_len[ply + 1]; i++)
        pv_table[ply][i + 1] = pv_table[ply + 1][i];
    pv_table_len[ply] = 1 + pv_table_len[ply + 1];
}

template <bool pvNode> int SearchThread::quiesce(int alpha, int beta, StackEntry *stack)
{
    const int ply = board.ply;
    if (ply >= MAX_DEPTH)
        return evaluate(board, NN);

    pv_table_len[ply] = 0;
    nodes++;
    if (board.is_draw(ply))
        return draw_score();

    if (check_for_stop<false>())
        return evaluate(board, NN);

    const Key key = board.key();
    const bool turn = board.turn;
    int score = INF, best = -INF;
    int ttBound = NONE;
    Move bestMove = NULLMOVE, ttMove = NULLMOVE;

    bool ttHit = false;
    Entry *entry = TT->probe(key, ttHit);

    int eval = INF, ttValue = INF, raw_eval{};
    bool was_pv = pvNode;

    /// probe transposition table
    if (ttHit)
    {
        best = eval = entry->eval;
        ttValue = score = entry->value(ply);
        ttBound = entry->bound();
        ttMove = entry->move;
        was_pv |= entry->was_pv();
        if constexpr (!pvNode)
        {
            if (score != VALUE_NONE && (ttBound & (score >= beta ? TTBounds::LOWER : TTBounds::UPPER)))
                return score;
        }
    }

    HistoricalState next_state;
    const bool in_check = board.checkers() != 0;
    int futility_base;

    if (in_check)
    {
        raw_eval = stack->eval = INF;
        best = futility_base = -INF;
    }
    else if (!ttHit)
    {
        raw_eval = evaluate(board, NN);
        stack->eval = best = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key(), board.mat_key(WHITE),
                                                                 board.mat_key(BLACK), stack);
        futility_base = best + QuiesceFutilityBias;
    }
    else
    {
        // ttValue might be a better evaluation
        raw_eval = eval;
        stack->eval = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key(), board.mat_key(WHITE),
                                                          board.mat_key(BLACK), stack);
        if (ttBound == TTBounds::EXACT || (ttBound == TTBounds::LOWER && ttValue > eval) ||
            (ttBound == TTBounds::UPPER && ttValue < eval))
            best = ttValue;
        futility_base = best + QuiesceFutilityBias;
    }

    // stand-pat
    if (best >= beta)
    {
        if (abs(best) < MATE && abs(beta) < MATE)
            best = (best + beta) / 2;
        if (!ttHit)
            TT->save(entry, key, best, 0, ply, TTBounds::LOWER, NULLMOVE, raw_eval, was_pv);
        return best;
    }

    // delta pruning
    if (!in_check && best + DeltaPruningMargin < alpha)
        return alpha;

    alpha = std::max(alpha, best);

    Movepick qs_movepicker(!in_check && ttMove && see(board, ttMove, 0) ? ttMove : NULLMOVE, board.threats(), in_check);
    Move move;
    int played = 0;

    while ((move = qs_movepicker.get_next_move(histories, stack, board)))
    {
        if (qs_movepicker.stage == Stages::STAGE_QS_NOISY && !see(board, move, 0))
        {
            continue;
        }
        played++;

        if (played == 4)
            break;

        if (played > 1)
        {
            // futility pruning
            if (futility_base > -MATE)
            {
                const int futility_value = futility_base + seeVal[board.get_captured_type(move)];
                if (!move.is_promo() && futility_value <= alpha)
                {
                    best = std::max(best, futility_value);
                    continue;
                }
            }

            if (in_check)
                break;
        }

        // update stack info
        TT->prefetch(board.speculative_next_key(move));
        stack->move = move;
        stack->piece = board.piece_at(move.get_from());
        stack->cont_hist = &histories.cont_history[!board.is_noisy_move(move)][stack->piece][move.get_to()];
        stack->cont_corr_hist = &histories.cont_corr_hist[stack->piece][move.get_to()];

        make_move(move, next_state);

        // PVS
        if (!pvNode || played > 1)
            score = -quiesce<false>(-alpha - 1, -alpha, stack + 1);

        if constexpr (pvNode)
        {
            if (played == 1 || score > alpha)
                score = -quiesce<pvNode>(-beta, -alpha, stack + 1);
        }

        undo_move(move);

        if (must_stop())
            return best;

        if (score > best)
        {
            best = score;
            bestMove = move;
            if (score > alpha)
            {
                alpha = score;
                if (alpha >= beta)
                    break;
            }
        }
    }

    if (in_check && played == 0)
        return -INF + ply;

    // store info in transposition table
    ttBound = best >= beta ? TTBounds::LOWER : TTBounds::UPPER;
    TT->save(entry, key, best, 0, ply, ttBound, bestMove, raw_eval, was_pv);

    return best;
}

template <bool rootNode, bool pvNode, bool cutNode>
int SearchThread::search(int alpha, int beta, int depth, StackEntry *stack)
{
    const int ply = board.ply;

    if (check_for_stop<true>() || ply >= MAX_DEPTH)
        return evaluate(board, NN);

    if (depth <= 0)
        return quiesce<pvNode>(alpha, beta, stack);

    constexpr bool allNode = !pvNode && !cutNode;
    const bool nullSearch = (stack - 1)->move == NULLMOVE;
    const int previous_R = (stack - 1)->R;
    const int original_alpha = alpha;
    const Key key = board.key(), pawn_key = board.pawn_key(), white_mat_key = board.mat_key(WHITE),
              black_mat_key = board.mat_key(BLACK);
    const bool turn = board.turn;

    std::array<SearchMove, 32> noisies, quiets;
    uint16_t nr_quiets = 0;
    uint16_t nr_noisies = 0;

    int played = 0;
    int best = -INF;
    Move bestMove = NULLMOVE;

    Move ttMove = NULLMOVE;
    int ttBound = NONE, ttValue = 0, ttDepth = -100;
    bool ttHit = false;

    nodes++;
    sel_depth = std::max(sel_depth, ply);
    (stack - 1)->R = 0; // reset to not have to reset in LMR

    pv_table_len[ply] = 0;

    if constexpr (!rootNode)
    {
        if (alpha < 0 && board.has_upcoming_repetition(ply))
            alpha = draw_score();
        if (board.is_draw(ply))
            return draw_score(); /// beal effect, credit to Ethereal for this

        /// mate distance pruning
        alpha = std::max(alpha, -INF + ply);
        beta = std::min(beta, INF - ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    Entry *entry = TT->probe(key, ttHit);

    /// transposition table probing
    int eval = INF;
    bool was_pv = pvNode;

    if (!stack->excluded && ttHit)
    {
        const int score = entry->value(ply);
        ttValue = score;
        ttBound = entry->bound();
        ttMove = entry->move;
        eval = entry->eval;
        ttDepth = entry->depth();
        was_pv |= entry->was_pv();
        if constexpr (!pvNode)
        {
            if (score != VALUE_NONE && ttDepth >= depth &&
                (ttBound & (score >= beta ? TTBounds::LOWER : TTBounds::UPPER)))
                return score;
        }
    }

    /// tablebase probing
    if constexpr (!rootNode)
    {
        const auto probe = probe_TB(board, depth);
        if (probe != TB_RESULT_FAILED)
        {
            tb_hits++;

            const auto [score, ttBound] = [probe, ply]() -> std::pair<int, int> {
                if (probe == TB_WIN)
                    return {TB_WIN_SCORE - MAX_DEPTH - ply, TTBounds::LOWER};
                if (probe == TB_LOSS)
                    return {-TB_WIN_SCORE + MAX_DEPTH + ply, TTBounds::UPPER};
                return {0, TTBounds::EXACT};
            }();

            if (ttBound & (score >= beta ? TTBounds::LOWER : TTBounds::UPPER))
            {
                TT->save(entry, key, score, MAX_DEPTH, 0, ttBound, NULLMOVE, 0, was_pv);
                return score;
            }
        }
    }

    HistoricalState next_state;
    const bool in_check = (board.checkers() != 0);
    const bool enemy_has_no_threats = !board.threats().threatened_pieces;
    int raw_eval;

    if (in_check)
    { /// when in check, don't evaluate
        stack->eval = raw_eval = eval = INF;
    }
    else if (!ttHit)
    { /// if we are in a singular search, we already know the evaluation
        if (stack->excluded)
            raw_eval = eval = stack->eval;
        else
        {
            raw_eval = evaluate(board, NN);
            stack->eval = eval =
                histories.get_corrected_eval(raw_eval, turn, pawn_key, white_mat_key, black_mat_key, stack);
            TT->save(entry, key, VALUE_NONE, 0, ply, 0, NULLMOVE, raw_eval, was_pv);
        }
    }
    else
    { /// ttValue might be a better evaluation
        if (stack->excluded)
            raw_eval = evaluate(board, NN);
        else
            raw_eval = eval;
        stack->eval = eval =
            histories.get_corrected_eval(raw_eval, turn, pawn_key, white_mat_key, black_mat_key, stack);
        if (ttBound == TTBounds::EXACT || (ttBound == TTBounds::LOWER && ttValue > eval) ||
            (ttBound == TTBounds::UPPER && ttValue < eval))
            eval = ttValue;
    }

    const int static_eval = stack->eval;
    const int complexity = std::abs(raw_eval - static_eval);
    const auto eval_diff = [&]() {
        if ((stack - 2)->eval != INF)
            return static_eval - (stack - 2)->eval;
        if ((stack - 4)->eval != INF)
            return static_eval - (stack - 4)->eval;
        return 0;
    }();

    const auto improving = [&]() {
        if (in_check || eval_diff == 0)
            return 0;
        if (eval_diff > 0)
            return 1;
        return eval_diff < NegativeImprovingMargin ? -1 : 0;
    }();

    const auto improving_after_move = [&]() {
        if (in_check || (stack - 1)->eval == INF)
            return false;
        return static_eval + (stack - 1)->eval > 0;
    }();

    const bool is_ttmove_noisy = ttMove && board.is_noisy_move(ttMove);
    const bool bad_static_eval = static_eval <= alpha;

    (stack + 2)->cutoff_cnt = 0;
    (stack + 1)->killer = NULLMOVE;
    stack->threats = board.threats().all_threats;

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    if (pvNode && depth >= IIRPvNodeDepth && (!ttHit || ttDepth + 4 <= depth))
        depth -= IIRPvNodeReduction;
    /// also for cut nodes
    if (cutNode && depth >= IIRCutNodeDepth && (!ttHit || ttDepth + 4 <= depth))
        depth -= IIRCutNodeReduction;

    if (!stack->excluded && !in_check && !nullSearch && (stack - 1)->eval != INF && board.captured() == NO_PIECE)
    {
        int bonus = std::clamp<int>(-EvalHistCoef * ((stack - 1)->eval + static_eval), EvalHistMin, EvalHistMax) +
                    EvalHistMargin;
        histories.update_hist_move((stack - 1)->move, (stack - 1)->threats, 1 ^ turn, bonus);
    }

    if (previous_R >= 3 && !improving_after_move && !in_check && (stack - 1)->eval != INF)
        depth += 1 + bad_static_eval;

    if constexpr (!pvNode)
    {
        if (!in_check)
        {
            // razoring
            auto razoring_margin = [&](int depth) { return RazoringMargin * depth; };
            if (depth <= RazoringDepth && eval + razoring_margin(depth) <= alpha)
            {
                int value = quiesce<false>(alpha, alpha + 1, stack);
                if (value <= alpha)
                    return value;
            }

            /// static null move pruning (don't prune when having a mate line, again stability)
            auto snmp_margin = [&](int depth, int improving, bool improving_after_move, bool is_cutnode,
                                   int complexity) {
                return (SNMPMargin - SNMPImproving * improving) * depth -
                       SNMPImprovingAfterMove * improving_after_move - SNMPCutNode * is_cutnode +
                       complexity * SNMPComplexityCoef / 1024 + SNMPBase;
            };
            if (depth <= SNMPDepth && eval > beta && eval < MATE && (!ttMove || is_ttmove_noisy) &&
                eval - snmp_margin(depth - enemy_has_no_threats, improving, improving_after_move, cutNode, complexity) >
                    beta)
                return beta > -MATE ? (eval + beta) / 2 : eval;

            /// null move pruning (when last move wasn't null, we still have non pawn material, we have a good position)
            if (!nullSearch && !stack->excluded && enemy_has_no_threats && board.has_non_pawn_material(turn) &&
                eval >= beta + NMPEvalMargin * (depth <= 3) && eval >= static_eval &&
                static_eval + 15 * depth - 100 >= beta)
            {
                int R = NMPReduction + depth / NMPDepthDiv + (eval - beta) / NMPEvalDiv + improving + is_ttmove_noisy;

                stack->move = NULLMOVE;
                stack->piece = NO_PIECE;
                stack->cont_hist = &histories.cont_history[0][NO_PIECE][0];
                stack->cont_corr_hist = &histories.cont_corr_hist[NO_PIECE][0];

                board.make_null_move(next_state);
                int score = -search<false, false, !cutNode>(-beta, -beta + 1, depth - R, stack + 1);
                board.undo_null_move();

                if (score >= beta)
                    return abs(score) > MATE ? beta : score; /// don't trust mate scores
                else if (previous_R && abs(score) < MATE && score < beta - NMPHindsightMargin)
                    depth++;
            }

            // probcut
            const int probcut_beta = beta + ProbcutMargin;
            if (depth >= ProbcutDepth && abs(beta) < MATE && !(ttHit && ttDepth >= depth - 3 && ttValue < probcut_beta))
            {
                Movepick picker(ttMove && board.is_noisy_move(ttMove) && see(board, ttMove, probcut_beta - static_eval)
                                    ? ttMove
                                    : NULLMOVE,
                                probcut_beta - static_eval, board.threats());

                Move move;
                while ((move = picker.get_next_move(histories, stack, board)) != NULLMOVE)
                {
                    if (move == stack->excluded)
                        continue;

                    stack->move = move;
                    stack->piece = board.piece_at(move.get_from());
                    stack->cont_hist = &histories.cont_history[0][stack->piece][move.get_to()];
                    stack->cont_corr_hist = &histories.cont_corr_hist[stack->piece][move.get_to()];

                    make_move(move, next_state);

                    int score = -quiesce<false>(-probcut_beta, -probcut_beta + 1, stack + 1);
                    if (score >= probcut_beta)
                        score = -search<false, false, !cutNode>(-probcut_beta, -probcut_beta + 1,
                                                                depth - ProbcutReduction, stack + 1);

                    undo_move(move);

                    if (score >= probcut_beta)
                    {
                        if (!stack->excluded)
                            TT->save(entry, key, score, depth - 3, ply, TTBounds::LOWER, move, raw_eval, was_pv);
                        return score;
                    }
                }
            }

            // ????
            if (ttHit && ttBound == TTBounds::LOWER && ttDepth >= depth / 2 && cutNode)
            {
                Movepick picker(ttMove, stack->killer, kp_move[turn][board.king_pawn_key() & KP_MOVE_MASK], 0,
                                board.threats());

                int beat_beta_count = 0;
                int played = 0;
                Move move;
                while ((move = picker.get_next_move(histories, stack, board)) != NULLMOVE)
                {
                    /// update stack info
                    TT->prefetch(board.speculative_next_key(move));
                    stack->move = move;
                    stack->piece = board.piece_at(move.get_from());
                    stack->cont_hist = &histories.cont_history[!board.is_noisy_move(move)][stack->piece][move.get_to()];
                    stack->cont_corr_hist = &histories.cont_corr_hist[stack->piece][move.get_to()];

                    make_move(move, next_state);
                    played++;

                    int R = depth / 2; // ??
                    int score = -search<false, false, !cutNode>(-beta, -beta + 1, depth - R, stack);

                    undo_move(move);

                    if (score >= beta)
                    {
                        beat_beta_count++;
                        if (beat_beta_count >= 3)
                            return beta;
                    }

                    if (played == 10)
                        break;
                }
            }
        }
    }

#ifndef TUNE_FLAG
    constexpr int see_depth_coef = rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef;
#else
    int see_depth_coef = rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef;
#endif

    Movepick picker(ttMove, stack->killer, kp_move[turn][board.king_pawn_key() & KP_MOVE_MASK], -see_depth_coef * depth,
                    board.threats());

    Move move;

    while ((move = picker.get_next_move(histories, stack, board)) != NULLMOVE)
    {
        if constexpr (rootNode)
        {
            bool move_was_searched = false;
            for (int i = 1; i < multipv; i++)
            {
                if (move == best_moves[i])
                {
                    move_was_searched = true;
                    break;
                }
            }
            if (move_was_searched)
                continue;
        }
        if (move == stack->excluded)
            continue;

        const bool is_quiet = !board.is_noisy_move(move);
        const Square from = move.get_from(), to = move.get_to();
        const Piece piece = board.piece_at(from);
        int history = 0;

#ifdef GENERATE
        if constexpr (!pvNode)
        {
#else
        if constexpr (!rootNode)
        {
#endif
            if (best > -MATE && board.has_non_pawn_material(turn))
            {
                if (is_quiet)
                {
                    history =
                        histories.get_history_search(move, piece, board.threats().all_threats, turn, stack, pawn_key);

                    // approximately the new depth for the next search
                    int new_depth = std::max(0, depth - lmr_red[std::min(63, depth)][std::min(63, played)] / LMRGrain +
                                                    improving + history / MoveloopHistDiv);

                    // futility pruning
                    auto futility_margin = [&](int depth) { return FPBias + FPMargin * depth; };
                    if (new_depth <= FPDepth && !in_check && static_eval + futility_margin(new_depth) <= alpha)
                        picker.skip_quiets();

                    // late move pruning
                    if (new_depth <= LMPDepth && played >= (LMPBias + new_depth * new_depth) / (2 - improving))
                        picker.skip_quiets();

                    // history pruning
                    auto history_margin = [&](int depth) { return -HistoryPruningMargin * depth; };
                    if (depth <= HistoryPruningDepth && bad_static_eval && history < history_margin(depth))
                    {
                        picker.skip_quiets();
                        continue;
                    }

                    // see pruning for quiet moves
                    if (new_depth <= SEEPruningQuietDepth && !in_check &&
                        !see(board, move, -SEEPruningQuietMargin * new_depth))
                        continue;
                }
                else
                {
                    history = histories.get_cap_hist(piece, to, board.get_captured_type(move));
                    // see pruning for noisy moves
                    auto noisy_see_pruning_margin = [&](int depth, int history) {
                        return -SEEPruningNoisyMargin * depth * depth - history / SEENoisyHistDiv;
                    };
                    if (depth <= SEEPruningNoisyDepth && !in_check && picker.trueStage > Stages::STAGE_GOOD_NOISY &&
                        !see(board, move, noisy_see_pruning_margin(depth + bad_static_eval, history)))
                        continue;

                    // futility pruning for noisy moves
                    auto noisy_futility_margin = [&](int depth, Piece captured) {
                        return FPNoisyBias + seeVal[captured] + FPNoisyMargin * depth;
                    };
                    if (depth <= FPNoisyDepth && !in_check &&
                        static_eval + noisy_futility_margin(depth + is_ttmove_noisy, board.get_captured_type(move)) <=
                            alpha)
                        continue;
                }
            }
        }

        int ex = 0;
        // avoid extending too far (might cause stack overflow)
        if (ply < 2 * id_depth && !rootNode)
        {
            // singular extension (look if the tt move is better than the rest)
            if (!stack->excluded && !allNode && move == ttMove && abs(ttValue) < MATE && depth >= SEDepth &&
                ttDepth >= depth - 3 && (ttBound & TTBounds::LOWER))
            {
                int rBeta = ttValue - (SEMargin + SEWasPVMargin * (!pvNode && was_pv)) * depth / 64;

                stack->excluded = move;
                int score = search<false, false, cutNode>(rBeta - 1, rBeta, (depth - 1) / 2, stack);
                stack->excluded = NULLMOVE;

                if (score < rBeta)
                    ex = 1 + (!pvNode && rBeta - score > SEDoubleExtensionsMargin) +
                         (!pvNode && !is_ttmove_noisy && rBeta - score > SETripleExtensionsMargin);
                else if (rBeta >= beta)
                    return rBeta; // multicut
                else if (ttValue >= beta || ttValue <= alpha)
                    ex = -1 - !pvNode; // negative extensions
            }
            else if (in_check)
                ex = 1;
        }
        else if (allNode && played >= 1 && ttDepth >= depth - 3 && ttBound == TTBounds::UPPER)
            ex = -1;

        /// update stack info
        TT->prefetch(board.speculative_next_key(move));
        stack->move = move;
        stack->piece = piece;
        stack->cont_hist = &histories.cont_history[is_quiet][piece][to];
        stack->cont_corr_hist = &histories.cont_corr_hist[piece][to];

        make_move(move, next_state);
        played++;

        if constexpr (rootNode)
        {
            // current root move info
            if (main_thread() && printStats && info.get_time_elapsed() > 2500 && !info.is_san_mode())
            {
                std::cout << "info depth " << depth << " currmove " << move.to_string(board.chess960)
                          << " currmovenumber " << played << std::endl;
            }
        }

        int new_depth = depth + ex, R = 1 * LMRGrain;

        uint64_t nodes_previously = nodes;
        int score = -INF, tried_count = 0;

        // late move reductions
        if (depth >= 2 && played > 1 + pvNode + rootNode)
        {
            R = lmr_red[std::min(63, depth)][std::min(63, played)];

            R -= LMRGrain * history /
                 (is_quiet ? HistReductionDiv : CapHistReductionDiv); // reduce move based on history
            R -= LMRGivesCheck * (board.checkers() != 0);             // move gives check
            R += LMRWasNotPV * !was_pv;
            R += LMRGoodEval *
                 (enemy_has_no_threats && !in_check &&
                  eval - seeVal[PieceTypes::KNIGHT] >
                      beta); // if the position is relatively quiet and eval is bigger than beta by a margin
            R += LMREvalDifference *
                 (enemy_has_no_threats && !in_check && static_eval - root_eval > EvalDifferenceReductionMargin &&
                  ply % 2 == 0); /// the position in quiet and static eval is way bigger than root eval
            R -= LMRRefutation * (is_quiet && picker.trueStage == Stages::STAGE_KILLER); // reduce for refutation moves
            R += LMRBadNoisy * (enemy_has_no_threats &&
                                picker.trueStage == Stages::STAGE_BAD_NOISY); // if the position is relatively quiet
                                                                              // and the capture is "very losing"
            R += LMRImprovingM1 * (improving == -1);                          // reduce if really not improving
            R += LMRImproving0 * (improving == 0);                            // reduce if not improving
            R -= LMRPVNode * (!rootNode && pvNode);                           // reduce pv nodes
            R += LMRCutNode * cutNode;                                        // reduce cutnodes aggressively
            R -= LMRWasPVHighDepth * (was_pv && ttDepth >= depth);            // reduce ex pv nodes with valuable info
            R += LMRTTMoveNoisy * is_ttmove_noisy;                            // reduce if ttmove is noisy
            R +=
                LMRBadStaticEval * (enemy_has_no_threats && !in_check && static_eval + LMRBadStaticEvalMargin <= alpha);
            R -= LMRGrain * complexity / LMRCorrectionDivisor;
            R += (LMRFailLowPV + LMRFailLowPVHighDepth * (ttDepth > depth)) * (was_pv && ttValue <= alpha && ttHit);
            R += LMRCutoffCount * (stack->cutoff_cnt > 3); // reduce if we had a lot of cutoffs

            R /= LMRGrain;

            R = std::clamp(R, 1, new_depth); // clamp R
            stack->R = R;
            score = -search<false, false, true>(-alpha - 1, -alpha, new_depth - R, stack + 1);
            stack->R = 0;
            tried_count++;

            if (R > 1 && score > alpha)
            {
                new_depth += (score > best + DeeperMargin) - (score < best + new_depth);
                score = -search<false, false, !cutNode>(-alpha - 1, -alpha, new_depth - 1, stack + 1);
                tried_count++;
            }
        }
        else if (!pvNode || played > 1)
        {
            score = -search<false, false, !cutNode>(-alpha - 1, -alpha, new_depth - 1, stack + 1);
            tried_count++;
        }

        if constexpr (pvNode)
        {
            if (played == 1 || score > alpha)
            {
                new_depth += (score > best + PVDeeperMargin);
                score = -search<false, true, false>(-beta, -alpha, new_depth - 1, stack + 1);
                tried_count++;
            }
        }

        undo_move(move);
        nodes_seached[move.get_from_to()] += nodes - nodes_previously;

        [[unlikely]] if (must_stop()) // stop search
            return best;

        if (score > best)
        {
            best = score;
            if (score > alpha)
            {
                if constexpr (rootNode)
                {
                    best_moves[multipv] = move;
                    root_scores[multipv] = score;
                }
                bestMove = move;
                alpha = score;
                if constexpr (pvNode)
                    update_pv(ply, move);
                if (alpha >= beta)
                {
                    const int bonus = getHistoryBonus(depth + bad_static_eval + (cutNode && depth <= 3));
                    const int malus = getHistoryMalus(depth + bad_static_eval + (cutNode && depth <= 3) + allNode);

                    stack->cutoff_cnt++;
                    if (!board.is_noisy_move(bestMove))
                    {
                        stack->killer = bestMove;
                        kp_move[turn][board.king_pawn_key() & KP_MOVE_MASK] = bestMove;
                        if (nr_quiets || depth >= HistoryUpdateMinDepth)
                            histories.update_hist_quiet_move(bestMove, board.piece_at(bestMove.get_from()),
                                                             board.threats().all_threats, turn, stack, pawn_key,
                                                             bonus * tried_count);
                        for (int i = 0; i < nr_quiets; i++)
                        {
                            const auto [move, tried_count] = quiets[i];
                            histories.update_hist_quiet_move(move, board.piece_at(move.get_from()),
                                                             board.threats().all_threats, turn, stack, pawn_key,
                                                             malus * tried_count);
                        }
                    }
                    else
                    {
                        histories.update_cap_hist_move(board.piece_at(bestMove.get_from()), bestMove.get_to(),
                                                       board.get_captured_type(bestMove), bonus * tried_count);
                    }
                    for (int i = 0; i < nr_noisies; i++)
                    {
                        const auto [move, tried_count] = noisies[i];
                        histories.update_cap_hist_move(board.piece_at(move.get_from()), move.get_to(),
                                                       board.get_captured_type(move), malus * tried_count);
                    }
                    break;
                }
            }
        }

        if (move != bestMove)
        {
            if (is_quiet && nr_quiets < 32)
                quiets[nr_quiets++] = SearchMove(move, tried_count);
            else if (!is_quiet && nr_noisies < 32)
                noisies[nr_noisies++] = SearchMove(move, tried_count);
        }
    }

    if (!played)
        return in_check ? -INF + ply : 0;

    if (!bestMove && board.captured() == NO_PIECE && !nullSearch)
    {
        histories.update_cont_hist_move((stack - 1)->piece, (stack - 1)->move.get_to(), stack - 1,
                                        getHistoryBonus(depth) * FailLowContHistCoef / 128);
        histories.update_hist_move((stack - 1)->move, (stack - 1)->threats, 1 ^ turn,
                                   getHistoryBonus(depth) * FailLowHistCoef / 128);
    }

    // update tt only if we aren't in a singular search
    if (!stack->excluded)
    {
        ttBound = best >= beta ? TTBounds::LOWER : (best > original_alpha ? TTBounds::EXACT : TTBounds::UPPER);
        if ((ttBound == TTBounds::UPPER || !board.is_noisy_move(bestMove)) && !in_check &&
            !(ttBound == TTBounds::LOWER && best <= static_eval) &&
            !(ttBound == TTBounds::UPPER && best >= static_eval))
            histories.update_corr_hist(turn, pawn_key, white_mat_key, black_mat_key, stack, depth, best - static_eval);
        TT->save(entry, key, best, depth, ply, ttBound, bestMove, raw_eval, was_pv);
    }

    return best;
}

void SearchThread::print_iteration_info(bool san_mode, int multipv, int score, int alpha, int beta, uint64_t t,
                                        int depth, int sel_depth, uint64_t total_nodes, uint64_t total_tb_hits)
{
    if (!san_mode)
    {
        std::cout << "info multipv " << multipv << " score ";

        if (score > MATE)
            std::cout << "mate " << (INF - score + 1) / 2;
        else if (score < -MATE)
            std::cout << "mate -" << (INF + score + 1) / 2;

        else
            std::cout << "cp " << score;
        if (score >= beta)
            std::cout << " lowerbound";
        else if (score <= alpha)
            std::cout << " upperbound";

        std::cout << " depth " << depth << " seldepth " << sel_depth << " nodes " << total_nodes;
        if (t)
            std::cout << " nps " << total_nodes * 1000 / t;
        std::cout << " time " << t << " ";
        std::cout << "tbhits " << total_tb_hits << " hashfull " << TT->hashfull() << " ";
        std::cout << "pv ";
        print_pv();
        std::cout << std::endl;
    }
    else
    {
        std::cout << std::setw(3) << depth << "/" << std::setw(3) << sel_depth << " ";
        if (t < 10 * 1000)
            std::cout << std::setw(7) << std::setprecision(2) << t / 1000.0 << "s   ";
        else
            std::cout << std::setw(7) << t / 1000 << "s   ";
        std::cout << std::setw(7) << total_nodes * 1000 / (t + 1) << "n/s   ";
        if (score > MATE)
            std::cout << "#" << (INF - score + 1) / 2 << " ";
        else if (score < -MATE)
            std::cout << "#-" << (INF + score + 1) / 2 << " ";
        else
        {
            score = abs(score);
            std::cout << (score >= 0 ? "+" : "-") << score / 100 << "."
                      << (score % 100 >= 10 ? std::to_string(score % 100) : "0" + std::to_string(score % 100)) << " ";
        }
        std::cout << "  ";
        print_pv();
        std::cout << std::endl;
    }
}

void SearchThread::start_search()
{
#ifdef TUNE_FLAG
    if (main_thread())
    {
        for (int i = 1; i < 64; i++)
        { /// depth
            for (int j = 1; j < 64; j++)
            { /// moves played
                lmr_red[i][j] = LMRGrain * (LMRQuietBias + log(i) * log(j) / LMRQuietDiv);
            }
        }
    }
#endif

#ifndef GENERATE
    memcpy(&board, &thread_pool->board, sizeof(Board));
#endif
    NN.init(board);
    clear_stack();
    nodes = sel_depth = tb_hits = 0;
    time_check_count = 0;
    best_move_cnt = 0;
    completed_depth = 0;
    root_eval = !board.checkers() ? evaluate(board, NN) : INF;

    scores.fill(0);
    best_moves.fill(NULLMOVE);
    root_scores.fill(0);
    search_stack.fill(StackEntry());
    stack = search_stack.data() + 10;

#ifndef GENERATE
    info = thread_pool->info;
#endif

    for (int i = 1; i <= 10; i++)
    {
        (stack - i)->cont_hist = &histories.cont_history[0][NO_PIECE][0];
        (stack - i)->cont_corr_hist = &histories.cont_corr_hist[NO_PIECE][0];
        (stack - i)->eval = INF;
        (stack - i)->move = NULLMOVE;
    }
    iterative_deepening();

    if (!main_thread())
        return;

#ifndef GENERATE
    thread_pool->stop();
    thread_pool->wait_for_finish(false); // don't wait for main thread, it's already finished
    thread_pool->pick_and_print_best_thread();
#endif
}

void SearchThread::iterative_deepening()
{
    int alpha, beta;
    int limitDepth = main_thread() ? info.get_depth_limit()
                                   : MAX_DEPTH; // when limited by depth, allow helper threads to pass the fixed depth
    int last_root_score = 0;
    Move last_best_move = NULLMOVE;

    MoveList moves;
    int nr_moves = board.gen_legal_moves<MOVEGEN_ALL>(moves);

    if (nr_moves)
    {
        best_moves[1] = moves[0];
        root_scores[1] = 0;
    }
    else
    {
        root_scores[1] = INF + 69;
        return; // no legal moves, return immediately
    }

    for (id_depth = 1; id_depth <= limitDepth; id_depth++)
    {
        for (multipv = 1; multipv <= info.get_multipv(); multipv++)
        {
            int window = AspirationWindosValue + root_scores[1] * root_scores[1] / AspirationWindowsDivisor;
            if (id_depth >= AspirationWindowsDepth)
            {
                alpha = std::max(-INF, scores[multipv] - window);
                beta = std::min(INF, scores[multipv] + window);
            }
            else
            {
                alpha = -INF;
                beta = INF;
            }

            int depth = id_depth;
            while (true)
            {
                depth = std::max({depth, 1, id_depth - 4});
                sel_depth = 0;
                scores[multipv] = search<true, true, false>(alpha, beta, depth, stack);

                if (must_stop())
                    break;

                if (main_thread() && printStats &&
                    ((alpha < scores[multipv] && scores[multipv] < beta) ||
                     (multipv == 1 && info.get_time_elapsed() > 3000)))
                {
                    print_iteration_info(info.is_san_mode(), multipv, scores[multipv], alpha, beta,
                                         info.get_time_elapsed(), depth, sel_depth, thread_pool->get_nodes(),
                                         thread_pool->get_tbhits());
                }

                if (scores[multipv] <= alpha)
                {
                    beta = (beta + alpha) / 2;
                    alpha = std::max(-INF, alpha - window);
                    depth = id_depth;
                    completed_depth = id_depth - 1;
                }
                else if (beta <= scores[multipv])
                {
                    beta = std::min(INF, beta + window);
                    depth--;
                    completed_depth = id_depth;
                }
                else
                {
                    completed_depth = id_depth;
                    break;
                }

                window += window * AspirationWindowExpandMargin / 100 + AspirationWindowExpandBias;
            }
        }

        if (main_thread() && !must_stop())
        {
            double scoreChange = 1.0, bestMoveStreak = 1.0, nodesSearchedPercentage = 1.0;
            if (id_depth >= TimeManagerMinDepth)
            {
                scoreChange = std::clamp<double>(
                    TimeManagerScoreBias + 1.0 * (last_root_score - root_scores[1]) / TimeManagerScoreDiv,
                    TimeManagerScoreMin, TimeManagerScoreMax); /// adjust time based on score change
                best_move_cnt = (best_moves[1] == last_best_move ? best_move_cnt + 1 : 1);
                /// adjust time based on how many nodes from the total searched nodes were used for the best move
                nodesSearchedPercentage = 1.0 * nodes_seached[best_moves[1].get_from_to()] / nodes;
                nodesSearchedPercentage =
                    TimeManagerNodesSearchedMaxPercentage - TimeManagerNodesSearchedCoef * nodesSearchedPercentage;
                bestMoveStreak =
                    TimeManagerBestMoveMax -
                    TimeManagerbestMoveStep *
                        std::min(10, best_move_cnt); /// adjust time based on how long the best move was the same
            }
            info.set_recommended_soft_limit(scoreChange * bestMoveStreak * nodesSearchedPercentage);
            last_root_score = root_scores[1];
            last_best_move = best_moves[1];
        }

        if (must_stop())
            break;

        if (id_depth == limitDepth)
        {
            state |= STOP;
            break;
        }

        if (main_thread() && (info.soft_limit_passed() || info.min_nodes_passed(nodes)))
        {
            state |= STOP;
            break;
        }
    }
}
