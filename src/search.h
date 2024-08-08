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
#include <algorithm>
#include "evaluate.h"
#include "movepick.h"
#include "tt.h"
#include "Fathom/src/tbprobe.h"
#include "thread.h"
#include <cstring>
#include <cmath>
#include <fstream>
#include <iomanip>

void delete_pool() {
    for(auto &thread : threads) {
        if(thread.joinable())
            thread.join();
    }
    threads.clear();
    threads_data.clear();
}

void create_pool(int thread_count) {
    delete_pool();
    for(int i = 0; i < thread_count; i++) {
        threads_data.emplace_back();
        threads_data[i].thread_id = i;
        threads_data[i].flag_stopped = false;
    }
}

void pool_stop() {
    for(auto &thread_data : threads_data)
        thread_data.flag_stopped = true;
}

void clear_stack_pool() {
    for(auto &thread_data : threads_data)
        thread_data.clear_stack();
}

void clear_history_pool() {
    for(auto &thread_data : threads_data)
        thread_data.clear_history();
}

void set_fen_pool(std::string fen, bool chess960 = false) {
    for(auto &thread_data : threads_data) {
        thread_data.board.chess960 = chess960;
        thread_data.board.set_fen(fen);
    }
}

void set_dfrc_pool(int idx) {
    for(auto &thread_data : threads_data) {
        thread_data.board.chess960 = (idx > 0);
        thread_data.board.set_dfrc(idx);
    }
}

void make_move_pool(Move move) {
    for (auto &thread_data : threads_data)
        thread_data.board.make_move(move);
}

void clear_board_pool() {
    for (auto &thread_data : threads_data)
        thread_data.board.clear();
}

uint64_t get_total_nodes_pool() {
    std::lock_guard <std::mutex> lock(threads_mutex);
    uint64_t nodes = 0;
    for(auto &thread_data : threads_data)
        nodes += thread_data.nodes;
    return nodes;
}

uint64_t get_total_tb_hits_pool() {
    std::lock_guard <std::mutex> lock(threads_mutex);
    uint64_t nodes = 0;
    for(auto &thread_data : threads_data)
        nodes += thread_data.tb_hits;
    return nodes;
}

template <bool checkTime>
bool SearchData::check_for_stop() {
    if (thread_id) return 0;

    if (flag_stopped) return 1;

    if ((info->nodes != -1 && info->nodes <= nodes) || (info->max_nodes != -1 && nodes >= info->max_nodes)) {
        flag_stopped = true;
        return 1;
    }

    checkCount++;
    if (checkCount == (1 << 10)) {
        if constexpr (checkTime) {
            if (info->timeset && getTime() > info->startTime + info->hardTimeLim) flag_stopped = true;
        }
        checkCount = 0;
    }

    return flag_stopped;
}

bool printStats = true; /// default true

uint32_t probe_TB(Board& board, int depth) {
    if (TB_LARGEST && depth >= 2 && !board.halfMoves) {
        if (count(board.get_bb_color(WHITE) | board.get_bb_color(BLACK)) <= TB_LARGEST)
            return tb_probe_wdl(board.get_bb_color(WHITE), board.get_bb_color(BLACK),
                board.get_bb_piece_type(KING), board.get_bb_piece_type(QUEEN), board.get_bb_piece_type(ROOK),
                board.get_bb_piece_type(BISHOP), board.get_bb_piece_type(KNIGHT), board.get_bb_piece_type(PAWN),
                0, 0, (board.enPas == -1 ? 0 : board.enPas), board.turn);
    }
    return TB_RESULT_FAILED;
}

Threats get_threats(Board& board, const bool us) {
    const bool enemy = 1 ^ us;
    uint64_t ourPieces = board.get_bb_color(us) ^ board.get_bb_piece(PAWN, us);
    uint64_t att = getPawnAttacks(enemy, board.get_bb_piece(PAWN, enemy)), attPieces = att & ourPieces;
    uint64_t pieces, b, all = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);

    ourPieces ^= board.get_bb_piece(KNIGHT, us) | board.get_bb_piece(BISHOP, us);

    pieces = board.get_bb_piece(KNIGHT, enemy);
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksKnight(Sq(b));
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    pieces = board.get_bb_piece(BISHOP, enemy);
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksBishop(all, Sq(b));
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    ourPieces ^= board.get_bb_piece(ROOK, us);

    pieces = board.get_bb_piece(ROOK, enemy);
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksRook(all, Sq(b));
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    pieces = board.get_bb_piece(QUEEN, enemy);
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksRook(all, Sq(b));
        pieces ^= b;
    }

    att |= genAttacksKing(board.king(enemy));

    return { att, attPieces };
}

std::string getSanString(Board& board, Move move) {
    if (type(move) == CASTLE) return (sq_to(move) > sq_from(move) ? "O-O" : "O-O-O");
    int from = sq_from(move), to = sq_to(move), prom = (type(move) == PROMOTION ? promoted(move) + KNIGHT + 6 : 0), piece = board.piece_type_at(from);
    std::string san;

    if (piece != PAWN) san += piece_char[piece + 6];
    else san += char('a' + (from & 7));
    if (board.is_capture(move)) san += "x";
    if (piece != PAWN || board.is_capture(move)) san += char('a' + (to & 7));

    san += char('1' + (to >> 3));

    if (prom) san += "=", san += piece_char[prom];

    board.make_move(move);
    if (board.checkers) san += '+';
    board.undo_move(move);

    return san;
}

void SearchData::print_pv() {
    for (int i = 0; i < pv_table_len[0]; i++) {
        if (!info->sanMode)
            std::cout << move_to_string(pv_table[0][i], info->chess960) << " ";
        else {
            std::cout << getSanString(board, pv_table[0][i]) << " ";
            board.make_move(pv_table[0][i]);
        }
    }

    if (info->sanMode) {
        for (int i = pv_table_len[0] - 1; i >= 0; i--) board.undo_move(pv_table[0][i]);
    }
}

void SearchData::update_pv(int ply, int move) {
    pv_table[ply][0] = move;
    for (int i = 0; i < pv_table_len[ply + 1]; i++) pv_table[ply][i + 1] = pv_table[ply + 1][i];
    pv_table_len[ply] = 1 + pv_table_len[ply + 1];
}

template <bool pvNode>
int SearchData::quiesce(int alpha, int beta, StackEntry* stack) {
    const int ply = board.ply;

    if (ply >= MAX_DEPTH) return evaluate(board);

    pv_table_len[ply] = 0;

    nodes++;

    if (board.is_draw(ply)) return 1 - (nodes & 2);

    if (check_for_stop<false>()) return evaluate(board);

    const uint64_t key = board.key;
    const bool turn = board.turn;
    int score = INF, best = -INF, alphaOrig = alpha;
    int bound = NONE;
    Move bestMove = NULLMOVE, ttMove = NULLMOVE;

    bool ttHit = false;
    Entry* entry = TT->probe(key, ttHit);

    int eval = INF, ttValue = INF, raw_eval{};
    bool wasPV = pvNode;

    /// probe transposition table
    if (ttHit) {
        best = eval = entry->eval;
        ttValue = score = entry->value(ply);
        bound = entry->bound();
        ttMove = entry->move;
        wasPV |= entry->wasPV();
        if constexpr (!pvNode) {
            if (score != VALUE_NONE && (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))) return score;
        }
    }

    const bool isCheck = (board.checkers != 0);
    Threats threats{};
    if (isCheck) threats = get_threats(board, turn);
    int futilityValue;

    if (isCheck) {
        raw_eval = stack->eval = INF;
        best = futilityValue = -INF;
    }
    else if (!ttHit) {
        raw_eval = evaluate(board);
        stack->eval = best = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key);
        futilityValue = best + QuiesceFutilityBias;
    }
    else { /// ttValue might be a better evaluation
        raw_eval = eval;
        stack->eval = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key);
        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval)) best = ttValue;
        futilityValue = best + QuiesceFutilityBias;
    }

    /// stand-pat
    if (best >= beta) return best;

    /// delta pruning
    if (!isCheck && best + DeltaPruningMargin < alpha) return alpha;

    alpha = std::max(alpha, best);

    Movepick noisyPicker(!isCheck && see(board, ttMove, 0) ? ttMove : NULLMOVE,
        NULLMOVE,
        0,
        threats.threats_enemy);

    Move move;
    int played = 0;

    while ((move = noisyPicker.get_next_move(histories, stack, board, !isCheck, true))) {
        // futility pruning
        played++;
        if (played == 4) break;
        if (played > 1) {
            if (futilityValue > -MATE) {
                const int value = futilityValue + seeVal[board.piece_type_at(sq_to(move))];
                if (type(move) != PROMOTION && value <= alpha) {
                    best = std::max(best, value);
                    continue;
                }
            }
            if (isCheck) break;
        }
        // update stack info
        TT->prefetch(board.speculative_next_key(move));
        stack->move = move;
        stack->piece = board.piece_at(sq_from(move));
        stack->cont_hist = &histories.cont_history[!board.is_noisy_move(move)][stack->piece][sq_to(move)];

        board.make_move(move);
        score = -quiesce<pvNode>(-beta, -alpha, stack + 1);
        board.undo_move(move);

        if (flag_stopped) return best;

        if (score > best) {
            best = score;
            bestMove = move;
            if (score > alpha) {
                alpha = score;
                update_pv(ply, move);
                if (alpha >= beta) break;
            }
        }
    }

    if (isCheck && best == -INF) return -INF + ply;

    /// store info in transposition table
    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(entry, key, best, 0, ply, bound, bestMove, raw_eval, wasPV);

    return best;
}
template <bool rootNode, bool pvNode>
int SearchData::search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack) {
    const int ply = board.ply;

    if (check_for_stop<true>() || ply >= MAX_DEPTH) return evaluate(board);

    if (depth <= 0) return quiesce<pvNode>(alpha, beta, stack);

    const bool allNode = !pvNode && !cutNode;
    const bool nullSearch = (stack - 1)->move == NULLMOVE;
    const int alphaOrig = alpha;
    const uint64_t key = board.key;
    const bool turn = board.turn;

    uint16_t nrQuiets = 0;
    uint16_t nrCaptures = 0;
    int played = 0, skip = 0;
    int best = -INF;
    Move bestMove = NULLMOVE;

    Move ttMove = NULLMOVE;
    int ttBound = NONE, ttValue = 0, ttDepth = -100;
    bool ttHit = false;

    nodes++;
    sel_depth = std::max(sel_depth, ply);

    pv_table_len[ply] = 0;

    if constexpr (!rootNode) {
        if (board.is_draw(ply)) return 1 - (nodes & 2); /// beal effect, credit to Ethereal for this

        /// mate distance pruning   
        alpha = std::max(alpha, -INF + ply);
        beta = std::min(beta, INF - ply - 1);
        if (alpha >= beta) return alpha;
    }

    Entry* entry = TT->probe(key, ttHit);

    /// transposition table probing
    int eval = INF;
    bool wasPV = pvNode;

    if (!stack->excluded && ttHit) {
        const int score = entry->value(ply);
        ttValue = score;
        ttBound = entry->bound();
        ttMove = entry->move;
        eval = entry->eval;
        ttDepth = entry->depth();
        wasPV |= entry->wasPV();
        if constexpr (!pvNode) {
            if (score != VALUE_NONE && ttDepth >= depth && (ttBound & (score >= beta ? LOWER : UPPER))) return score;
        }
    }

    /// tablebase probing
    if constexpr (!rootNode) {
        const auto probe = probe_TB(board, depth);
        if (probe != TB_RESULT_FAILED) {
            int type = NONE, score;
            tb_hits++;
            switch (probe) {
            case TB_WIN:
                score = TB_WIN_SCORE - MAX_DEPTH - ply;
                type = LOWER;
                break;
            case TB_LOSS:
                score = -(TB_WIN_SCORE - MAX_DEPTH - ply);
                type = UPPER;
                break;
            default:
                score = 0;
                type = EXACT;
                break;
            }

            if (type == EXACT || (type == UPPER && score <= alpha) || (type == LOWER && score >= beta)) {
                TT->save(entry, key, score, MAX_DEPTH, 0, type, NULLMOVE, 0, wasPV);
                return score;
            }
        }
    }

    const bool isCheck = (board.checkers != 0);
    const Threats threats = get_threats(board, turn);
    const bool enemy_has_no_threats = !threats.threats_pieces;
    int raw_eval{};
    //int quietEnemy = quietness(board, 1 ^ turn);

    if (isCheck) { /// when in check, don't evaluate
        stack->eval = raw_eval = eval = INF;
    }
    else if (!ttHit) { /// if we are in a singular search, we already know the evaluation
        if (stack->excluded) raw_eval = eval = stack->eval;
        else {
            raw_eval = evaluate(board);
            stack->eval = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key);
            TT->save(entry, key, VALUE_NONE, 0, ply, 0, NULLMOVE, raw_eval, wasPV);
        }
    }
    else { /// ttValue might be a better evaluation
        if (stack->excluded) raw_eval = evaluate(board);
        else raw_eval = eval;
        stack->eval = eval = histories.get_corrected_eval(raw_eval, turn, board.pawn_key);
        if (ttBound == EXACT || (ttBound == LOWER && ttValue > eval) || (ttBound == UPPER && ttValue < eval)) eval = ttValue;
    }

    const int static_eval = stack->eval;
    int eval_diff = 0;
    if (ply > 1 && (stack - 2)->eval != INF)
        eval_diff = static_eval - (stack - 2)->eval;
    else if (ply > 3 && (stack - 4)->eval != INF)
        eval_diff = static_eval - (stack - 4)->eval;

    const int improving = (isCheck || eval_diff == 0 ? 0 :
        eval_diff > 0 ? 1 :
        eval_diff < NegativeImprovingMargin ? -1 : 0);

    (stack + 1)->killer = NULLMOVE;

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    if (pvNode && depth >= IIRPvNodeDepth && (!ttHit || ttDepth + 4 <= depth)) depth -= IIRPvNodeReduction;
    /// also for cut nodes
    if (cutNode && depth >= IIRCutNodeDepth && (!ttHit || ttDepth + 4 <= depth)) depth -= IIRCutNodeReduction;

    if constexpr (!pvNode) {
        if (!isCheck) {
            // razoring
            if (depth <= RazoringDepth && eval + RazoringMargin * depth <= alpha) {
                int value = quiesce<false>(alpha, alpha + 1, stack);
                if (value <= alpha) return value;
            }

            /// static null move pruning (don't prune when having a mate line, again stability)
            if (depth <= SNMPDepth && eval > beta && 
                eval - (SNMPMargin - SNMPImproving * improving) * (depth - enemy_has_no_threats) > beta && eval < MATE) return (beta > -MATE ? (eval + beta) / 2 : eval);

            /// null move pruning (when last move wasn't null, we still have non pawn material, we have a good position)
            if (!nullSearch && !stack->excluded && enemy_has_no_threats &&
                eval >= beta + NMPEvalMargin * (depth <= 3) && eval >= static_eval &&
                board.has_non_pawn_material(turn)) {
                int R = NMPReduction + depth / NMPDepthDiv + (eval - beta) / NMPEvalDiv + improving;

                stack->move = NULLMOVE;
                stack->piece = 0;
                stack->cont_hist = &histories.cont_history[0][0][0];

                board.make_null_move();
                int score = -search<false, false>(-beta, -beta + 1, depth - R, !cutNode, stack + 1);
                board.undo_null_move();

                if (score >= beta) return (abs(score) > MATE ? beta : score); /// don't trust mate scores
            }

            // probcut
            int probBeta = beta + ProbcutMargin;
            if (depth >= ProbcutDepth && abs(beta) < MATE && !(ttHit && ttDepth >= depth - 3 && ttValue < probBeta)) {
                Movepick picker((ttMove && board.is_noisy_move(ttMove) && see(board, ttMove, probBeta - static_eval) ? ttMove : NULLMOVE),
                    NULLMOVE,
                    probBeta - static_eval,
                    threats.threats_enemy);

                Move move;
                while ((move = picker.get_next_move(histories, stack, board, true, true)) != NULLMOVE) {
                    if (move == stack->excluded)
                        continue;

                    stack->move = move;
                    stack->piece = board.piece_at(sq_from(move));
                    stack->cont_hist = &histories.cont_history[0][stack->piece][sq_to(move)];

                    board.make_move(move);

                    int score = -quiesce<false>(-probBeta, -probBeta + 1, stack + 1);

                    if (score >= probBeta) score = -search<false, false>(-probBeta, -probBeta + 1, depth - ProbcutReduction, !cutNode, stack + 1);
                    board.undo_move(move);

                    if (score >= probBeta) {
                        if(!stack->excluded)
                            TT->save(entry, key, score, depth - 3, ply, LOWER, move, raw_eval, wasPV);
                        return score;
                    }
                }
            }
        }
    }

#ifndef TUNE_FLAG
    constexpr int see_depth_coef = (rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef);
#else
    int see_depth_coef = (rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef);
#endif
    const bool bad_static_eval = (static_eval <= alpha);

    Movepick picker(ttMove,
        stack->killer,
        -see_depth_coef * depth,
        threats.threats_enemy);

    Move move;
    const bool ttCapture = ttMove && board.is_noisy_move(ttMove);

    while ((move = picker.get_next_move(histories, stack, board, skip, false)) != NULLMOVE) {
        if constexpr (rootNode) {
            bool searched = false;
            for (int i = 1; i < multipv_index; i++) {
                if (move == best_move[i]) {
                    searched = true;
                    break;
                }
            }
            if (searched) continue;
        }
        if (move == stack->excluded)
            continue;

        const bool isQuiet = !board.is_noisy_move(move);
        const int from = sq_from(move), to = sq_to(move), piece = board.piece_at(from);
        int history = 0;

        /// quiet move pruning

#ifdef GENERATE
        if (best > -MATE && board.has_non_pawn_material(turn) && !pvNode) {
            if (isQuiet) {
                history = histories.get_history_search(move, piece, threats.threats_enemy, turn, stack);

                /// approximately the new depth for the next search
                int newDepth = std::max(0, depth - lmr_red[std::min(63, depth)][std::min(63, played)] + improving);

                /// futility pruning
                if (newDepth <= FPDepth && !isCheck && 
                    static_eval + FPBias + FPMargin * newDepth <= alpha) skip = 1;

                /// late move pruning
                if (newDepth <= LMPDepth && played >= (LMPBias + newDepth * newDepth) / (2 - improving)) skip = 1;

                if (depth <= 3 && bad_static_eval && history < -4096 * depth) continue;

                if (depth <= SEEPruningQuietDepth && !isCheck && 
                    !see(board, move, -SEEPruningQuietMargin * depth)) continue;
            }
            else {
                history = histories.get_cap_hist(piece, to, type(move) == ENPASSANT ? PAWN : board.piece_type_at(to));
                if (depth <= SEEPruningNoisyDepth && !isCheck && picker.trueStage > STAGE_GOOD_NOISY && 
                    !see(board, move, -SEEPruningNoisyMargin * (depth + bad_static_eval) * (depth + bad_static_eval) - history / 256)) continue;

                if (depth <= FPNoisyDepth && !isCheck && 
                    static_eval + FPBias + seeVal[board.piece_type_at(sq_to(move))] + FPMargin * depth <= alpha) continue;
            }
        }
#else
        if constexpr (!rootNode) {
            if (best > -MATE && board.has_non_pawn_material(turn)) {
                if (isQuiet) {
                    history = histories.get_history_search(move, piece, threats.threats_enemy, turn, stack);
                    
                    /// approximately the new depth for the next search
                    int newDepth = std::max(0, depth - lmr_red[std::min(63, depth)][std::min(63, played)] + improving + history / MoveloopHistDiv);

                    /// futility pruning
                    if (newDepth <= FPDepth && !isCheck && 
                        static_eval + FPBias + FPMargin * newDepth <= alpha) skip = 1;

                    /// late move pruning
                    if (newDepth <= LMPDepth && played >= (LMPBias + newDepth * newDepth) / (2 - improving)) skip = 1;

                    if (depth <= HistoryPruningDepth && bad_static_eval && history < -HistoryPruningMargin * depth) continue;

                    if (newDepth <= SEEPruningQuietDepth && !isCheck && 
                        !see(board, move, -SEEPruningQuietMargin * newDepth)) continue;
                }
                else {
                    history = histories.get_cap_hist(piece, to, type(move) == ENPASSANT ? PAWN : board.piece_type_at(to));
                    if (depth <= SEEPruningNoisyDepth && !isCheck && picker.trueStage > STAGE_GOOD_NOISY && 
                        !see(board, move, -SEEPruningNoisyMargin * (depth + bad_static_eval) * (depth + bad_static_eval) - history / 256)) continue;

                    if (depth <= FPNoisyDepth && !isCheck && 
                        static_eval + FPBias + seeVal[board.piece_type_at(sq_to(move))] + FPMargin * depth <= alpha) continue;
                }
            }
        }
#endif

        int ex = 0;
        /// avoid extending too far (might cause stack overflow)
        if (ply < 2 * tDepth && !rootNode) {
            /// singular extension (look if the tt move is better than the rest)
            if (!stack->excluded && !allNode && move == ttMove && abs(ttValue) < MATE 
                && depth >= SEDepth && ttDepth >= depth - 3 && (ttBound & LOWER)) {
                int rBeta = ttValue - SEMargin * depth / 64;

                stack->excluded = move;
                int score = search<false, false>(rBeta - 1, rBeta, (depth - 1) / 2, cutNode, stack);
                stack->excluded = NULLMOVE;

                if (score < rBeta) ex = 1 + (!pvNode && rBeta - score > SEDoubleExtensionsMargin) + (!pvNode && !ttCapture && rBeta - score > SETripleExtensionsMargin);
                else if (rBeta >= beta) return rBeta; // multicut
                else if (ttValue >= beta || ttValue <= alpha) ex = -1 - !pvNode;
            }
            else if (isCheck) ex = 1;
        }
        else if (allNode && played >= 1 && ttDepth >= depth - 3 && ttBound == UPPER) ex = -1;

        /// update stack info
        TT->prefetch(board.speculative_next_key(move));
        stack->move = move;
        stack->piece = piece;
        stack->cont_hist = &histories.cont_history[isQuiet][piece][to];

        board.make_move(move);
        played++;

        if constexpr (rootNode) {
            /// current root move info
            if (thread_id == 0 && printStats && getTime() > info->startTime + 2500 && !info->sanMode) {
                std::cout << "info depth " << depth << " currmove " << move_to_string(move, info->chess960) << " currmovenumber " << played << std::endl;
            }
        }

        /// store quiets for history
        if (isQuiet)
            stack->quiets[nrQuiets++] = move;
        else
            stack->captures[nrCaptures++] = move;

        int newDepth = depth + ex, R = 1;

        uint64_t initNodes = nodes;
        int score = -INF;

        if (depth >= 2 && played > 1 + pvNode + rootNode) { /// first few moves we don't reduce
            if (isQuiet) {
                R = lmr_red[std::min(63, depth)][std::min(63, played)];
                R += !wasPV + (improving <= 0); /// not on pv or not improving
                R -= !rootNode && pvNode;
                R += enemy_has_no_threats && !isCheck && eval - seeVal[KNIGHT] > beta; /// if the position is relatively quiet and eval is bigger than beta by a margin
                R += enemy_has_no_threats && !isCheck && static_eval - rootEval > EvalDifferenceReductionMargin && ply % 2 == 0; /// the position in quiet and static eval is way bigger than root eval
                R -= 2 * (picker.trueStage == STAGE_KILLER); /// reduce for refutation moves
                R -= board.checkers != 0; /// move gives check
                R -= history / HistReductionDiv; /// reduce based on move history
            }
            else if (!wasPV) {
                R = lmr_red_noisy[std::min(63, depth)][std::min(63, played)];
                R += improving <= 0; /// not improving
                R += enemy_has_no_threats && picker.trueStage == STAGE_BAD_NOISY; /// if the position is relatively quiet and the capture is "very losing"
            }

            R += 2 * cutNode;
            R -= wasPV && ttDepth >= depth;
            R += ttCapture;
            R += enemy_has_no_threats && !isCheck && static_eval + LMRBadStaticEvalMargin <= alpha;

            R = std::clamp(R, 1, newDepth); /// clamp R
            score = -search<false, false>(-alpha - 1, -alpha, newDepth - R, true, stack + 1);

            if (R > 1 && score > alpha) {
                newDepth += (score > best + DeeperMargin) - (score < best + newDepth);
                score = -search<false, false>(-alpha - 1, -alpha, newDepth - 1, !cutNode, stack + 1);
            }
        }
        else if (!pvNode || played > 1) {
            score = -search<false, false>(-alpha - 1, -alpha, newDepth - 1, !cutNode, stack + 1);
        }

        if constexpr (pvNode) {
            if (played == 1 || score > alpha)
                score = -search<false, true>(-beta, -alpha, newDepth - 1, false, stack + 1);
        }

        board.undo_move(move);
        nodes_seached[fromTo(move)] += nodes - initNodes;

        if (flag_stopped) /// stop search
            return best;

        if (score > best) {
            best = score;
            bestMove = move;
            if (score > alpha) {
                if constexpr (rootNode) {
                    best_move[multipv_index] = move;
                    root_score[multipv_index] = score;
                }
                alpha = score;
                if constexpr(pvNode)
                    update_pv(ply, move);
                if (alpha >= beta) {
                    break;
                }
            }
        }
    }

    if (!played) {
        return (isCheck ? -INF + ply : 0);
    }

    /// update killer and history heuristics in case of a cutoff
    if (best >= beta) {
        if (!board.is_noisy_move(bestMove)) {
            stack->killer = bestMove;
            if (nrQuiets) {
                const int quiet_bonus = getHistoryBonus(depth + bad_static_eval);
                assert(stack->quiets[nrQuiets - 1] == bestMove);
                if (nrQuiets > 1 || depth >= HistoryUpdateMinDepth)
                    histories.update_hist_quiet_move(bestMove, board.piece_at(sq_from(bestMove)), threats.threats_enemy, turn, stack, quiet_bonus);
                for (int i = 0; i < nrQuiets - 1; i++) {
                    const Move move = stack->quiets[i];
                    histories.update_hist_quiet_move(move, board.piece_at(sq_from(move)), threats.threats_enemy, turn, stack, -quiet_bonus);
                }
            }
        }
        const int noisy_bonus = getHistoryBonus(depth + bad_static_eval);
        for (int i = 0; i < nrCaptures; i++) {
            const Move move = stack->captures[i];
            histories.update_cap_hist_move(board.piece_at(sq_from(move)), sq_to(move), type(move) == ENPASSANT ? PAWN : board.piece_type_at(sq_to(move)), move == bestMove ? noisy_bonus : -noisy_bonus);
        }
    }

    /// update tt only if we aren't in a singular search
    if (!stack->excluded) {
        ttBound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
        if ((ttBound == UPPER || !board.is_noisy_move(bestMove)) && !isCheck && !(ttBound == LOWER && best <= static_eval) && !(ttBound == UPPER && best >= static_eval))
            histories.update_corr_hist(turn, board.pawn_key, depth, best - static_eval);
        TT->save(entry, key, best, depth, ply, ttBound, (ttBound == UPPER ? NULLMOVE : bestMove), raw_eval, wasPV);
    }

    return best;
}

void main_thread_handler(Info *info) {
    for(std::size_t i = 1; i < threads_data.size(); i++)
        threads.push_back(std::thread(&SearchData::start_search, &threads_data[i], info));
    threads_data[0].start_search(info);
    pool_stop();

    for(auto &thread : threads) {
        if(thread.joinable())
           thread.join();
    }

    threads.clear();

    int bs = 0;
    Move bm = NULLMOVE;
    
    int bestDepth = threads_data[0].completed_depth;
    bs = threads_data[0].root_score[1];
    bm = threads_data[0].best_move[1];
    for (std::size_t i = 1; i < threads_data.size(); i++) {
        if (threads_data[i].root_score[1] > bs && threads_data[i].completed_depth >= bestDepth) {
            bs = threads_data[i].root_score[1];
            bm = threads_data[i].best_move[1];
            bestDepth = threads_data[i].completed_depth;
        }
    }

    if (printStats) {
        std::cout << "bestmove " << move_to_string(bm, info->chess960);
        std::cout << std::endl;
    }
}

void SearchData::start_search(Info* _info) {
    int alpha, beta;

    nodes = sel_depth = tb_hits = 0;
    t0 = getTime();
    flag_stopped = false;
    checkCount = 0;
    best_move_cnt = 0;

    fill_multiarray<Move, MAX_DEPTH + 5, 2 * MAX_DEPTH + 5>(pv_table, 0);
    info = _info;

    int limitDepth = (thread_id == 0 ? info->depth : MAX_DEPTH); /// when limited by depth, allow helper threads to pass the fixed depth
    int mainThreadScore = 0;
    Move mainThreadBestMove = NULLMOVE;

    scores.fill(0);
    best_move.fill(0);
    root_score.fill(0);
    std::array<StackEntry, MAX_DEPTH + 15> search_stack;
    StackEntry* stack = search_stack.data() + 10;

    search_stack.fill(StackEntry());

    rootEval = (!board.checkers ? evaluate(board) : INF);

    for (int i = 1; i <= 10; i++) {
        (stack - i)->cont_hist = &histories.cont_history[0][0][0], (stack - i)->eval = INF, (stack - i)->move = NULLMOVE;
    }

    completed_depth = 0;

    for (tDepth = 1; tDepth <= limitDepth; tDepth++) {
        multipv_index = 0;
        for (int i = 1; i <= info->multipv; i++) {
            multipv_index++;
            int window = AspirationWindosValue;
            if (tDepth >= AspirationWindowsDepth) {
                alpha = std::max(-INF, scores[i] - window);
                beta = std::min(INF, scores[i] + window);
            }
            else {
                alpha = -INF;
                beta = INF;
            }

            int depth = tDepth;
            while (true) {
                depth = std::max({ depth, 1, tDepth - 4 });
                sel_depth = 0;
                scores[i] = search<true, true>(alpha, beta, depth, false, stack);
                if (flag_stopped)
                    break;

                if (thread_id == 0 && printStats && ((alpha < scores[i] && scores[i] < beta) || (i == 1 && getTime() > t0 + 3000))) {
                    uint64_t total_nodes = get_total_nodes_pool();
                    uint64_t total_tb_hits = get_total_tb_hits_pool();
                    uint64_t t = (uint64_t)getTime() - t0;
                    if (!info->sanMode) {
                        std::cout << "info multipv " << i << " score ";

                        if (scores[i] > MATE) std::cout << "mate " << (INF - scores[i] + 1) / 2;
                        else if (scores[i] < -MATE) std::cout << "mate -" << (INF + scores[i] + 1) / 2;

                        else std::cout << "cp " << scores[i] * 100 / NormalizeToPawnValue;
                        if (scores[i] >= beta) std::cout << " lowerbound";
                        else if (scores[i] <= alpha) std::cout << " upperbound";

                        int ply = board.moveIndex * 2 - 1 - board.turn;
                        int wdlWin = winrate_model(scores[i], ply);
                        int wdlLose = winrate_model(-scores[i], ply);
                        int wdlDraw = 1000 - wdlWin - wdlLose;
                        uint64_t shady_stuff = total_nodes * 8 / 7;
                        std::cout << " wdl " << wdlWin << " " << wdlDraw << " " << wdlLose;

                        std::cout << " depth " << depth << " sel_depth " << sel_depth << " nodes " << shady_stuff;
                        if (t)
                            std::cout << " nps " << shady_stuff * 1000 / t;
                        std::cout << " time " << t << " ";
                        std::cout << "tb_hits " << total_tb_hits << " hashfull " << TT->tableFull() << " ";
                        std::cout << "pv ";
                        print_pv();
                        std::cout << std::endl;
                    }
                    else {
                        uint64_t total_nodes = get_total_nodes_pool();
                        std::cout << std::setw(3) << depth << "/" << std::setw(3) << sel_depth << " ";
                        if (t < 10 * 1000)
                            std::cout << std::setw(7) << std::setprecision(2) << t / 1000.0 << "s   ";
                        else
                            std::cout << std::setw(7) << t / 1000 << "s   ";
                        std::cout << std::setw(7) << total_nodes * 1000 / (t + 1) << "n/s   ";
                        if (scores[i] > MATE)
                            std::cout << "#" << (INF - scores[i] + 1) / 2 << " ";
                        else if (scores[i] < -MATE)
                            std::cout << "#-" << (INF + scores[i] + 1) / 2 << " ";
                        else {
                            int score = abs(scores[i] * NormalizeToPawnValue / 100);
                            std::cout << (scores[i] >= 0 ? "+" : "-") <<
                                score / 100 << "." << (score % 100 >= 10 ? std::to_string(score % 100) : "0" + std::to_string(score % 100)) << " ";
                        }
                        std::cout << "  ";
                        print_pv();
                        std::cout << std::endl;
                        //std::cout << "Branching factor is " << std::fixed << std::setprecision(5) << pow(nodes, 1.0 / tDepth) << std::endl;
                    }
                }

                if (scores[i] <= alpha) {
                    beta = (beta + alpha) / 2;
                    alpha = std::max(-INF, alpha - window);
                    depth = tDepth;
                    completed_depth = tDepth - 1;
                }
                else if (beta <= scores[i]) {
                    beta = std::min(INF, beta + window);
                    depth--;
                    completed_depth = tDepth;
                }
                else {
                    completed_depth = tDepth;
                    break;
                }

                window += window * AspirationWindowExpandMargin / 100 + AspirationWindowExpandBias;
            }
        }

        if (thread_id == 0 && !flag_stopped) {
            double scoreChange = 1.0, best_movetreak = 1.0, nodesSearchedPercentage = 1.0;
            float _tmNodesSearchedMaxPercentage = TimeManagerNodesSearchedMaxPercentage / 1000.0;
            float _tmBestMoveMax = TimeManagerBestMoveMax / 1000.0;
            float _tmbest_movetep = TimeManagerbest_movetep / 1000.0;
            if (tDepth >= TimeManagerMinDepth) {
                scoreChange = std::max(TimeManagerScoreMin / 100.0, std::min(TimeManagerScoreBias / 100.0 + 1.0 * (mainThreadScore - root_score[1]) / TimeManagerScoreDiv, TimeManagerScoreMax / 100.0)); /// adjust time based on score change
                best_move_cnt = (best_move[1] == mainThreadBestMove ? best_move_cnt + 1 : 1);
                /// adjust time based on how many nodes from the total searched nodes were used for the best move
                nodesSearchedPercentage = 1.0 * nodes_seached[fromTo(best_move[1])] / nodes;
                nodesSearchedPercentage = TimeManagerNodesSeachedMaxCoef / 100.0 * _tmNodesSearchedMaxPercentage -
                    TimeManagerNodesSearchedCoef / 100.0 * nodesSearchedPercentage;
                best_movetreak = _tmBestMoveMax - _tmbest_movetep * std::min(10, best_move_cnt); /// adjust time based on how long the best move was the same
            }
            //std::cout << "Scale factor for tm is " << scoreChange * best_movetreak * nodesSearchedPercentage * 100 << "%\n";
            //std::cout << scoreChange * 100 << " " << best_movetreak * 100 << " " << _tmNodesSearchedMaxPercentage - nodesSearchedPercentage << "\n";
            info->stopTime = info->startTime + info->goodTimeLim * scoreChange * best_movetreak * nodesSearchedPercentage;
            mainThreadScore = root_score[1];
            mainThreadBestMove = best_move[1];
        }

        if (flag_stopped)
            break;

        if (info->timeset && getTime() > info->stopTime) {
            flag_stopped = true;
            break;
        }

        if (tDepth == limitDepth) {
            flag_stopped = true;
            break;
        }

        if (info->min_nodes != -1 && nodes >= info->min_nodes) {
            flag_stopped = true;
            break;
        }
    }
}