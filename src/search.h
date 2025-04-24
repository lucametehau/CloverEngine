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

    if (m_info.nodes_limit_passed(m_nodes) || m_info.max_nodes_passed(m_nodes))
    {
        m_state |= ThreadStates::STOP;
        return 1;
    }

    m_time_check_count++;
    if (m_time_check_count == (1 << 10))
    {
        if constexpr (checkTime)
        {
            if (m_info.hard_limit_passed())
                m_state |= ThreadStates::STOP;
        }
        m_time_check_count = 0;
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
    if (m_info.is_san_mode())
    {
        std::unique_ptr<std::deque<HistoricalState>> states = std::make_unique<std::deque<HistoricalState>>(0);
        for (int i = 0; i < m_pv_table_len[0]; i++)
        {
            states->emplace_back();
            std::cout << getSanString(m_board, m_pv_table[0][i], states->back()) << " ";
            m_board.make_move(m_pv_table[0][i], states->back());
        }
        for (int i = m_pv_table_len[0] - 1; i >= 0; i--)
            m_board.undo_move(m_pv_table[0][i]);
    }
    else
    {
        for (int i = 0; i < m_pv_table_len[0]; i++)
        {
            std::cout << m_pv_table[0][i].to_string(m_info.is_chess960()) << " ";
        }
    }
}

void SearchThread::update_pv(int ply, Move move)
{
    m_pv_table[ply][0] = move;
    for (int i = 0; i < m_pv_table_len[ply + 1]; i++)
        m_pv_table[ply][i + 1] = m_pv_table[ply + 1][i];
    m_pv_table_len[ply] = 1 + m_pv_table_len[ply + 1];
}

template <bool pvNode> int SearchThread::quiesce(int alpha, int beta, StackEntry *stack)
{
    const int ply = m_board.ply;
    if (ply >= MAX_DEPTH)
        return evaluate(m_board, NN);

    m_pv_table_len[ply] = 0;
    m_nodes++;
    if (m_board.is_draw(ply))
        return draw_score();

    if (check_for_stop<false>())
        return evaluate(m_board, NN);

    const uint64_t key = m_board.key();
    const bool turn = m_board.turn;
    int score = INF, best = -INF, original_alpha = alpha;
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
    const bool in_check = m_board.checkers() != 0;
    int futility_base;

    if (in_check)
    {
        raw_eval = stack->eval = INF;
        best = futility_base = -INF;
    }
    else if (!ttHit)
    {
        raw_eval = evaluate(m_board, NN);
        stack->eval = best = eval = m_histories.get_corrected_eval(
            raw_eval, turn, m_board.pawn_key(), m_board.mat_key(WHITE), m_board.mat_key(BLACK), stack);
        futility_base = best + QuiesceFutilityBias;
    }
    else
    { /// ttValue might be a better evaluation
        raw_eval = eval;
        stack->eval = eval = m_histories.get_corrected_eval(raw_eval, turn, m_board.pawn_key(), m_board.mat_key(WHITE),
                                                            m_board.mat_key(BLACK), stack);
        if (ttBound == TTBounds::EXACT || (ttBound == TTBounds::LOWER && ttValue > eval) ||
            (ttBound == TTBounds::UPPER && ttValue < eval))
            best = ttValue;
        futility_base = best + QuiesceFutilityBias;
    }

    /// stand-pat
    if (best >= beta)
    {
        if (!ttHit)
            TT->save(entry, key, best, 0, ply, TTBounds::LOWER, NULLMOVE, raw_eval, was_pv);
        return best;
    }

    /// delta pruning
    if (!in_check && best + DeltaPruningMargin < alpha)
        return alpha;

    alpha = std::max(alpha, best);

    Movepick noisyPicker(!in_check && see(m_board, ttMove, 0) ? ttMove : NULLMOVE, NULLMOVE, NULLMOVE, 0,
                         m_board.threats());

    Move move;
    int played = 0;

    while ((move = noisyPicker.get_next_move(m_histories, stack, m_board, !in_check, true)))
    {
        played++;
        if (played == 4)
            break;
        if (played > 1)
        {
            // futility pruning
            if (futility_base > -MATE)
            {
                const int value = futility_base + seeVal[m_board.get_captured_type(move)];
                if (!move.is_promo() && value <= alpha)
                {
                    best = std::max(best, value);
                    continue;
                }
            }
            if (in_check)
                break;
        }
        // update stack info
        TT->prefetch(m_board.speculative_next_key(move));
        stack->move = move;
        stack->piece = m_board.piece_at(move.get_from());
        stack->cont_hist = &m_histories.cont_history[!m_board.is_noisy_move(move)][stack->piece][move.get_to()];

        make_move(move, next_state);
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

    if (in_check && best == -INF)
        return -INF + ply;

    // store info in transposition table
    ttBound = best >= beta ? TTBounds::LOWER : (best > original_alpha ? TTBounds::EXACT : TTBounds::UPPER);
    TT->save(entry, key, best, 0, ply, ttBound, bestMove, raw_eval, was_pv);

    return best;
}

template <bool rootNode, bool pvNode, bool cutNode>
int SearchThread::search(int alpha, int beta, int depth, StackEntry *stack)
{
    const int ply = m_board.ply;

    if (check_for_stop<true>() || ply >= MAX_DEPTH)
        return evaluate(m_board, NN);

    if (depth <= 0)
        return quiesce<pvNode>(alpha, beta, stack);

    constexpr bool allNode = !pvNode && !cutNode;
    const bool nullSearch = (stack - 1)->move == NULLMOVE;
    const int original_alpha = alpha;
    const uint64_t key = m_board.key(), pawn_key = m_board.pawn_key(), white_mat_key = m_board.mat_key(WHITE),
                   black_mat_key = m_board.mat_key(BLACK);
    const bool turn = m_board.turn;

    std::array<SearchMove, 32> noisies, quiets;
    uint16_t nr_quiets = 0;
    uint16_t nr_noisies = 0;

    int played = 0;
    bool skip = 0;
    int best = -INF;
    Move bestMove = NULLMOVE;

    Move ttMove = NULLMOVE;
    int ttBound = NONE, ttValue = 0, ttDepth = -100;
    bool ttHit = false;

    m_nodes++;
    m_sel_depth = std::max(m_sel_depth, ply);

    m_pv_table_len[ply] = 0;

    if constexpr (!rootNode)
    {
        if (alpha < 0 && m_board.has_upcoming_repetition(ply))
            alpha = draw_score();
        if (m_board.is_draw(ply))
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
        const auto probe = probe_TB(m_board, depth);
        if (probe != TB_RESULT_FAILED)
        {
            m_tb_hits++;

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
    const bool in_check = (m_board.checkers() != 0);
    const bool enemy_has_no_threats = !m_board.threats().threatened_pieces;
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
            raw_eval = evaluate(m_board, NN);
            stack->eval = eval =
                m_histories.get_corrected_eval(raw_eval, turn, pawn_key, white_mat_key, black_mat_key, stack);
            TT->save(entry, key, VALUE_NONE, 0, ply, 0, NULLMOVE, raw_eval, was_pv);
        }
    }
    else
    { /// ttValue might be a better evaluation
        if (stack->excluded)
            raw_eval = evaluate(m_board, NN);
        else
            raw_eval = eval;
        stack->eval = eval =
            m_histories.get_corrected_eval(raw_eval, turn, pawn_key, white_mat_key, black_mat_key, stack);
        if (ttBound == TTBounds::EXACT || (ttBound == TTBounds::LOWER && ttValue > eval) ||
            (ttBound == TTBounds::UPPER && ttValue < eval))
            eval = ttValue;
    }

    const int static_eval = stack->eval;
    const auto eval_diff = [&]() {
        if ((stack - 2)->eval != INF)
            return static_eval - (stack - 2)->eval;
        else if ((stack - 4)->eval != INF)
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
    const bool is_ttmove_noisy = ttMove && m_board.is_noisy_move(ttMove);
    const bool bad_static_eval = static_eval <= alpha;

    (stack + 1)->killer = NULLMOVE;

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    if (pvNode && depth >= IIRPvNodeDepth && (!ttHit || ttDepth + 4 <= depth))
        depth -= IIRPvNodeReduction;
    /// also for cut nodes
    if (cutNode && depth >= IIRCutNodeDepth && (!ttHit || ttDepth + 4 <= depth))
        depth -= IIRCutNodeReduction;

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
            auto snmp_margin = [&](int depth, int improving) {
                return (SNMPMargin - SNMPImproving * improving) * depth;
            };
            if (depth <= SNMPDepth && eval > beta && eval < MATE && (!ttMove || is_ttmove_noisy) &&
                eval - snmp_margin(depth - enemy_has_no_threats, improving) > beta)
                return beta > -MATE ? (eval + beta) / 2 : eval;

            /// null move pruning (when last move wasn't null, we still have non pawn material, we have a good position)
            if (!nullSearch && !stack->excluded && enemy_has_no_threats && m_board.has_non_pawn_material(turn) &&
                eval >= beta + NMPEvalMargin * (depth <= 3) && eval >= static_eval)
            {
                int R = NMPReduction + depth / NMPDepthDiv + (eval - beta) / NMPEvalDiv + improving + is_ttmove_noisy;

                stack->move = NULLMOVE;
                stack->piece = NO_PIECE;
                stack->cont_hist = &m_histories.cont_history[0][NO_PIECE][0];

                m_board.make_null_move(next_state);
                int score = -search<false, false, !cutNode>(-beta, -beta + 1, depth - R, stack + 1);
                m_board.undo_null_move();

                if (score >= beta)
                    return abs(score) > MATE ? beta : score; /// don't trust mate scores
            }

            // probcut
            const int probcut_beta = beta + ProbcutMargin;
            if (depth >= ProbcutDepth && abs(beta) < MATE && !(ttHit && ttDepth >= depth - 3 && ttValue < probcut_beta))
            {
                Movepick picker(ttMove && m_board.is_noisy_move(ttMove) &&
                                        see(m_board, ttMove, probcut_beta - static_eval)
                                    ? ttMove
                                    : NULLMOVE,
                                NULLMOVE, NULLMOVE, probcut_beta - static_eval, m_board.threats());

                Move move;
                while ((move = picker.get_next_move(m_histories, stack, m_board, true, true)) != NULLMOVE)
                {
                    if (move == stack->excluded)
                        continue;

                    stack->move = move;
                    stack->piece = m_board.piece_at(move.get_from());
                    stack->cont_hist = &m_histories.cont_history[0][stack->piece][move.get_to()];

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
        }
    }

#ifndef TUNE_FLAG
    constexpr int see_depth_coef = rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef;
#else
    int see_depth_coef = rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef;
#endif

    Movepick picker(ttMove, stack->killer, m_kp_move[turn][m_board.king_pawn_key() & KP_MOVE_MASK],
                    -see_depth_coef * depth, m_board.threats());

    Move move;

    while ((move = picker.get_next_move(m_histories, stack, m_board, skip, false)) != NULLMOVE)
    {
        if constexpr (rootNode)
        {
            bool move_was_searched = false;
            for (int i = 1; i < m_multipv; i++)
            {
                if (move == m_best_moves[i])
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

        const bool is_quiet = !m_board.is_noisy_move(move);
        const Square from = move.get_from(), to = move.get_to();
        const Piece piece = m_board.piece_at(from);
        int history = 0;

#ifdef GENERATE
        if constexpr (!pvNode)
        {
#else
        if constexpr (!rootNode)
        {
#endif
            if (best > -MATE && m_board.has_non_pawn_material(turn))
            {
                if (is_quiet)
                {
                    history = m_histories.get_history_search(move, piece, m_board.threats().all_threats, turn, stack);

                    // approximately the new depth for the next search
                    int new_depth = std::max(0, depth - lmr_red[std::min(63, depth)][std::min(63, played)] / LMRGrain +
                                                    improving + history / MoveloopHistDiv);

                    // futility pruning
                    auto futility_margin = [&](int depth) { return FPBias + FPMargin * depth; };
                    if (new_depth <= FPDepth && !in_check && static_eval + futility_margin(new_depth) <= alpha)
                        skip = 1;

                    // late move pruning
                    if (new_depth <= LMPDepth && played >= (LMPBias + new_depth * new_depth) / (2 - improving))
                        skip = 1;

                    // history pruning
                    auto history_margin = [&](int depth) { return -HistoryPruningMargin * depth; };
                    if (depth <= HistoryPruningDepth && bad_static_eval && history < history_margin(depth))
                    {
                        skip = 1;
                        continue;
                    }

                    // see pruning for quiet moves
                    if (new_depth <= SEEPruningQuietDepth && !in_check &&
                        !see(m_board, move, -SEEPruningQuietMargin * new_depth))
                        continue;
                }
                else
                {
                    history = m_histories.get_cap_hist(piece, to, m_board.get_captured_type(move));
                    // see pruning for noisy moves
                    auto noisy_see_pruning_margin = [&](int depth, int history) {
                        return -SEEPruningNoisyMargin * depth * depth - history / SEENoisyHistDiv;
                    };
                    if (depth <= SEEPruningNoisyDepth && !in_check && picker.trueStage > Stages::STAGE_GOOD_NOISY &&
                        !see(m_board, move, noisy_see_pruning_margin(depth + bad_static_eval, history)))
                        continue;

                    // futility pruning for noisy moves
                    auto noisy_futility_margin = [&](int depth, Piece captured) {
                        return FPBias + seeVal[captured] + FPMargin * depth;
                    };
                    if (depth <= FPNoisyDepth && !in_check &&
                        static_eval + noisy_futility_margin(depth + is_ttmove_noisy, m_board.get_captured_type(move)) <=
                            alpha)
                        continue;
                }
            }
        }

        int ex = 0;
        // avoid extending too far (might cause stack overflow)
        if (ply < 2 * m_id_depth && !rootNode)
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
        TT->prefetch(m_board.speculative_next_key(move));
        stack->move = move;
        stack->piece = piece;
        stack->cont_hist = &m_histories.cont_history[is_quiet][piece][to];

        make_move(move, next_state);
        played++;

        if constexpr (rootNode)
        {
            // current root move info
            if (main_thread() && printStats && m_info.get_time_elapsed() > 2500 && !m_info.is_san_mode())
            {
                std::cout << "info depth " << depth << " currmove " << move.to_string(m_info.is_chess960())
                          << " currmovenumber " << played << std::endl;
            }
        }

        int new_depth = depth + ex, R = 1 * LMRGrain;

        uint64_t nodes_previously = m_nodes;
        int score = -INF, tried_count = 0;

        if (depth >= 2 && played > 1 + pvNode + rootNode)
        { // first few moves we don't reduce
            if (is_quiet)
            {
                R = lmr_red[std::min(63, depth)][std::min(63, played)];
                R += LMRWasNotPV * !was_pv;
                R += LMRImprovingM1 * (improving == -1);
                R += LMRImproving0 * (improving == 0);
                R -= LMRPVNode * (!rootNode && pvNode);
                R += LMRGoodEval *
                     (enemy_has_no_threats && !in_check &&
                      eval - seeVal[PieceTypes::KNIGHT] >
                          beta); // if the position is relatively quiet and eval is bigger than beta by a margin
                R += LMREvalDifference *
                     (enemy_has_no_threats && !in_check && static_eval - m_root_eval > EvalDifferenceReductionMargin &&
                      ply % 2 == 0); /// the position in quiet and static eval is way bigger than root eval
                R -= LMRRefutation * (picker.trueStage == Stages::STAGE_KILLER); // reduce for refutation moves
                R -= LMRGivesCheck * (m_board.checkers() != 0);                  // move gives check
                R -= LMRGrain * history / HistReductionDiv;                      // reduce based on move history
            }
            else if (!was_pv)
            {
                R = lmr_red[std::min(63, depth)][std::min(63, played)];
                R += LMRNoisyNotImproving * (improving <= 0); // not improving
                R += LMRBadNoisy * (enemy_has_no_threats &&
                                    picker.trueStage == Stages::STAGE_BAD_NOISY); // if the position is relatively quiet
                                                                                  // and the capture is "very losing"
                R -= LMRGrain * history / CapHistReductionDiv;                    // reduce based on move history
            }

            R += LMRCutNode * cutNode;                             // reduce cutnodes aggressively
            R -= LMRWasPVHighDepth * (was_pv && ttDepth >= depth); // reduce ex pv nodes with valuable info
            R += LMRTTMoveNoisy * is_ttmove_noisy;                 // reduce if ttmove is noisy
            R +=
                LMRBadStaticEval * (enemy_has_no_threats && !in_check && static_eval + LMRBadStaticEvalMargin <= alpha);
            R -= LMRGrain * std::abs(raw_eval - static_eval) / LMRCorrectionDivisor;
            R += (LMRFailLowPV + LMRFailLowPVHighDepth * (ttDepth > depth)) * (was_pv && ttValue <= alpha && ttHit);

            R /= LMRGrain;

            R = std::clamp(R, 1, new_depth); // clamp R
            score = -search<false, false, true>(-alpha - 1, -alpha, new_depth - R, stack + 1);
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
                score = -search<false, true, false>(-beta, -alpha, new_depth - 1, stack + 1);
                tried_count++;
            }
        }

        undo_move(move);
        m_nodes_seached[move.get_from_to()] += m_nodes - nodes_previously;

        if (must_stop()) // stop search
            return best;

        if (score > best)
        {
            best = score;
            if (score > alpha)
            {
                if constexpr (rootNode)
                {
                    m_best_moves[m_multipv] = move;
                    m_root_scores[m_multipv] = score;
                }
                bestMove = move;
                alpha = score;
                if constexpr (pvNode)
                    update_pv(ply, move);
                if (alpha >= beta)
                {
                    const int bonus = getHistoryBonus(depth + bad_static_eval + (cutNode && depth <= 3));
                    const int malus = getHistoryMalus(depth + bad_static_eval + (cutNode && depth <= 3) + allNode);
                    if (!m_board.is_noisy_move(bestMove))
                    {
                        stack->killer = bestMove;
                        m_kp_move[turn][m_board.king_pawn_key() & KP_MOVE_MASK] = bestMove;
                        if (nr_quiets || depth >= HistoryUpdateMinDepth)
                            m_histories.update_hist_quiet_move(bestMove, m_board.piece_at(bestMove.get_from()),
                                                               m_board.threats().all_threats, turn, stack,
                                                               bonus * tried_count);
                        for (int i = 0; i < nr_quiets; i++)
                        {
                            const auto [move, tried_count] = quiets[i];
                            m_histories.update_hist_quiet_move(move, m_board.piece_at(move.get_from()),
                                                               m_board.threats().all_threats, turn, stack,
                                                               malus * tried_count);
                        }
                    }
                    else
                    {
                        m_histories.update_cap_hist_move(m_board.piece_at(bestMove.get_from()), bestMove.get_to(),
                                                         m_board.get_captured_type(bestMove), bonus * tried_count);
                    }
                    for (int i = 0; i < nr_noisies; i++)
                    {
                        const auto [move, tried_count] = noisies[i];
                        m_histories.update_cap_hist_move(m_board.piece_at(move.get_from()), move.get_to(),
                                                         m_board.get_captured_type(move), malus * tried_count);
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

    // update tt only if we aren't in a singular search
    if (!stack->excluded)
    {
        ttBound = best >= beta ? TTBounds::LOWER : (best > original_alpha ? TTBounds::EXACT : TTBounds::UPPER);
        if ((ttBound == TTBounds::UPPER || !m_board.is_noisy_move(bestMove)) && !in_check &&
            !(ttBound == TTBounds::LOWER && best <= static_eval) &&
            !(ttBound == TTBounds::UPPER && best >= static_eval))
            m_histories.update_corr_hist(turn, pawn_key, white_mat_key, black_mat_key, stack, depth,
                                         best - static_eval);
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
    memcpy(&m_board, &m_thread_pool->m_board, sizeof(Board));
    NN.init(m_board);
    clear_stack();
    m_nodes = m_sel_depth = m_tb_hits = 0;
    m_time_check_count = 0;
    m_best_move_cnt = 0;
    m_completed_depth = 0;
    m_root_eval = !m_board.checkers() ? evaluate(m_board, NN) : INF;

    m_scores.fill(0);
    m_best_moves.fill(NULLMOVE);
    m_root_scores.fill(0);
    m_search_stack.fill(StackEntry());
    m_stack = m_search_stack.data() + 10;

    m_info = m_thread_pool->m_info;

    for (int i = 1; i <= 10; i++)
    {
        (m_stack - i)->cont_hist = &m_histories.cont_history[0][NO_PIECE][0];
        (m_stack - i)->eval = INF;
        (m_stack - i)->move = NULLMOVE;
    }

    iterative_deepening();

    if (!main_thread())
        return;

    m_thread_pool->stop();
    m_thread_pool->wait_for_finish(false); // don't wait for main thread, it's already finished
    m_thread_pool->pick_and_print_best_thread();
}

void SearchThread::iterative_deepening()
{
    int alpha, beta;
    int limitDepth = main_thread() ? m_info.get_depth_limit()
                                   : MAX_DEPTH; // when limited by depth, allow helper threads to pass the fixed depth
    int last_root_score = 0;
    Move last_best_move = NULLMOVE;

    for (m_id_depth = 1; m_id_depth <= limitDepth; m_id_depth++)
    {
        for (m_multipv = 1; m_multipv <= m_info.get_multipv(); m_multipv++)
        {
            int window = AspirationWindosValue + m_root_scores[1] * m_root_scores[1] / 10000;
            if (m_id_depth >= AspirationWindowsDepth)
            {
                alpha = std::max(-INF, m_scores[m_multipv] - window);
                beta = std::min(INF, m_scores[m_multipv] + window);
            }
            else
            {
                alpha = -INF;
                beta = INF;
            }

            int depth = m_id_depth;
            while (true)
            {
                depth = std::max({depth, 1, m_id_depth - 4});
                m_sel_depth = 0;
                m_scores[m_multipv] = search<true, true, false>(alpha, beta, depth, m_stack);

                if (must_stop())
                    break;

                if (main_thread() && printStats &&
                    ((alpha < m_scores[m_multipv] && m_scores[m_multipv] < beta) ||
                     (m_multipv == 1 && m_info.get_time_elapsed() > 3000)))
                {
                    print_iteration_info(m_info.is_san_mode(), m_multipv, m_scores[m_multipv], alpha, beta,
                                         m_info.get_time_elapsed(), depth, m_sel_depth, m_thread_pool->get_nodes(),
                                         m_thread_pool->get_tbhits());
                }

                if (m_scores[m_multipv] <= alpha)
                {
                    beta = (beta + alpha) / 2;
                    alpha = std::max(-INF, alpha - window);
                    depth = m_id_depth;
                    m_completed_depth = m_id_depth - 1;
                }
                else if (beta <= m_scores[m_multipv])
                {
                    beta = std::min(INF, beta + window);
                    depth--;
                    m_completed_depth = m_id_depth;
                }
                else
                {
                    m_completed_depth = m_id_depth;
                    break;
                }

                window += window * AspirationWindowExpandMargin / 100 + AspirationWindowExpandBias;
            }
        }

        if (main_thread() && !must_stop())
        {
            double scoreChange = 1.0, bestMoveStreak = 1.0, nodesSearchedPercentage = 1.0;
            if (m_id_depth >= TimeManagerMinDepth)
            {
                scoreChange = std::clamp<double>(
                    TimeManagerScoreBias + 1.0 * (last_root_score - m_root_scores[1]) / TimeManagerScoreDiv,
                    TimeManagerScoreMin, TimeManagerScoreMax); /// adjust time based on score change
                m_best_move_cnt = (m_best_moves[1] == last_best_move ? m_best_move_cnt + 1 : 1);
                /// adjust time based on how many nodes from the total searched nodes were used for the best move
                nodesSearchedPercentage = 1.0 * m_nodes_seached[m_best_moves[1].get_from_to()] / m_nodes;
                nodesSearchedPercentage =
                    TimeManagerNodesSearchedMaxPercentage - TimeManagerNodesSearchedCoef * nodesSearchedPercentage;
                bestMoveStreak =
                    TimeManagerBestMoveMax -
                    TimeManagerbestMoveStep *
                        std::min(10, m_best_move_cnt); /// adjust time based on how long the best move was the same
            }
            m_info.set_recommended_soft_limit(scoreChange * bestMoveStreak * nodesSearchedPercentage);
            last_root_score = m_root_scores[1];
            last_best_move = m_best_moves[1];
        }

        if (must_stop())
            break;

        if (m_id_depth == limitDepth)
        {
            m_state |= STOP;
            break;
        }

        if (main_thread() && (m_info.soft_limit_passed() || m_info.min_nodes_passed(m_nodes)))
        {
            m_state |= STOP;
            break;
        }
    }
}
