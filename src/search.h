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

template <bool checkTime>
bool Search::checkForStop() {
    if (!principalSearcher) return 0;

    if (flag & TERMINATED_SEARCH) return 1;

    if (SMPThreadExit || (info->nodes != -1 && info->nodes <= (int64_t)nodes)) {
        flag |= TERMINATED_BY_LIMITS;
        return 1;
    }

    checkCount++;

    if (checkCount == (1 << 10)) {
        if constexpr (checkTime) {
            if (info->timeset && getTime() > info->startTime + info->hardTimeLim && !info->ponder) flag |= TERMINATED_BY_LIMITS;
        }
        checkCount = 0;
    }

    return (flag & TERMINATED_SEARCH);
}

bool printStats = true; /// default true
const bool PROBE_ROOT = true; /// default true

uint32_t probe_TB(Board& board, int depth, bool probeAtRoot = 0, int halfMoves = 0, bool castlingRights = 0) {
    if (probeAtRoot) {
        unsigned pieces = count(board.pieces[WHITE] | board.pieces[BLACK]);

        if (pieces <= TB_LARGEST) {
            int ep = board.enPas;
            if (ep == -1)
                ep = 0;

            return tb_probe_root(board.pieces[WHITE], board.pieces[BLACK],
                board.bb[WK] | board.bb[BK], board.bb[WQ] | board.bb[BQ], board.bb[WR] | board.bb[BR],
                board.bb[WB] | board.bb[BB], board.bb[WN] | board.bb[BN], board.bb[WP] | board.bb[BP],
                halfMoves, castlingRights, ep, board.turn, nullptr);
        }
        return TB_RESULT_FAILED;
    }
    if (TB_LARGEST && depth >= 2 && !board.halfMoves && !board.castleRights) {
        unsigned pieces = count(board.pieces[WHITE] | board.pieces[BLACK]);

        if (pieces <= TB_LARGEST) {
            int ep = board.enPas;
            if (ep == -1)
                ep = 0;

            return tb_probe_wdl(board.pieces[WHITE], board.pieces[BLACK],
                board.bb[WK] | board.bb[BK], board.bb[WQ] | board.bb[BQ], board.bb[WR] | board.bb[BR],
                board.bb[WB] | board.bb[BB], board.bb[WN] | board.bb[BN], board.bb[WP] | board.bb[BP],
                0, 0, ep, board.turn);
        }
    }
    return TB_RESULT_FAILED;
}

Threats getThreats(Board& board, const bool us) {
    const bool enemy = 1 ^ us;
    uint64_t ourPieces = board.pieces[us] ^ board.bb[get_piece(PAWN, us)];
    uint64_t att = getPawnAttacks(enemy, board.bb[get_piece(PAWN, enemy)]), attPieces = att & ourPieces;
    uint64_t pieces, b, all = board.pieces[WHITE] | board.pieces[BLACK];

    ourPieces ^= board.bb[get_piece(KNIGHT, us)] | board.bb[get_piece(BISHOP, us)];

    pieces = board.bb[get_piece(KNIGHT, enemy)];
    while (pieces) {
        b = lsb(pieces);
        att |= knightBBAttacks[Sq(b)];
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    pieces = board.bb[get_piece(BISHOP, enemy)];
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksBishop(all, Sq(b));
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    ourPieces ^= board.bb[get_piece(ROOK, us)];

    pieces = board.bb[get_piece(ROOK, enemy)];
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksRook(all, Sq(b));
        attPieces |= att & ourPieces;
        pieces ^= b;
    }

    pieces = board.bb[get_piece(QUEEN, enemy)];
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksRook(all, Sq(b));
        pieces ^= b;
    }

    att |= kingBBAttacks[board.king(enemy)];

    return { att, attPieces };
}

std::string getSanString(Board& board, uint16_t move) {
    if (type(move) == CASTLE) return (sq_to(move) > sq_from(move) ? "O-O" : "O-O-O");
    int from = sq_from(move), to = sq_to(move), prom = (type(move) == PROMOTION ? promoted(move) + KNIGHT + 6 : 0), piece = board.piece_type_at(from);
    std::string san;

    if (piece != PAWN) san += pieceChar[piece + 6];
    else san += char('a' + (from & 7));
    if (board.isCapture(move)) san += "x";
    if (piece != PAWN || board.isCapture(move)) san += char('a' + (to & 7));

    san += char('1' + (to >> 3));

    if (prom) san += "=", san += pieceChar[prom];

    board.make_move(move);
    if (board.checkers) san += '+';
    board.undo_move(move);

    return san;
}

void Search::printPv() {
    for (int i = 0; i < pvTableLen[0]; i++) {
        if (!info->sanMode)
            std::cout << move_to_string(pvTable[0][i], info->chess960) << " ";
        else {
            std::cout << getSanString(board, pvTable[0][i]) << " ";
            board.make_move(pvTable[0][i]);
        }
    }

    if (info->sanMode) {
        for (int i = pvTableLen[0] - 1; i >= 0; i--) board.undo_move(pvTable[0][i]);
    }
}

void Search::update_pv(int ply, int move) {
    pvTable[ply][0] = move;
    for (int i = 0; i < pvTableLen[ply + 1]; i++) pvTable[ply][i + 1] = pvTable[ply + 1][i];
    pvTableLen[ply] = 1 + pvTableLen[ply + 1];
}

template <bool pvNode>
int Search::quiesce(int alpha, int beta, StackEntry* stack) {
    const int ply = board.ply;

    if (ply >= DEPTH) return evaluate(board);

    pvTableLen[ply] = 0;

    nodes++;
    qsNodes++;

    if (board.is_draw(ply)) return 1 - (nodes & 2);

    if (checkForStop<false>()) return evaluate(board);

    const uint64_t key = board.key;
    int score = INF, best = -INF, alphaOrig = alpha;
    int bound = NONE;
    uint16_t bestMove = NULLMOVE, ttMove = NULLMOVE;

    bool ttHit = false;
    Entry* entry = TT->probe(key, ttHit);

    int eval = INF, ttValue = INF;
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
    Threats threatsEnemy{};
    if (isCheck) threatsEnemy = getThreats(board, board.turn);
    int futilityValue;

    if (isCheck) {
        stack->eval = INF;
        best = futilityValue = -INF;
    }
    else if (eval == INF) {
        stack->eval = best = eval = evaluate(board);
        futilityValue = best + QuiesceFutilityBias;
    }
    else { /// ttValue might be a better evaluation
        stack->eval = eval;
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
        NULLMOVE,
        0,
        threatsEnemy.threatsEnemy);

    uint16_t move;
    int played = 0;

    while ((move = noisyPicker.get_next_move(this, stack, board, !isCheck, true))) {
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
        stack->continuationHist = &continuationHistory[!isNoisyMove(board, move)][stack->piece][sq_to(move)];

        board.make_move(move);
        score = -quiesce<pvNode>(-beta, -alpha, stack + 1);
        board.undo_move(move);

        if (flag & TERMINATED_SEARCH) return best;

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
    TT->save(entry, key, best, 0, ply, bound, bestMove, stack->eval, wasPV);

    return best;
}
template <bool rootNode, bool pvNode>
int Search::search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack) {
    const int ply = board.ply;

    if (checkForStop<true>()) return evaluate(board);

    if (ply >= DEPTH) return evaluate(board);

    if (depth <= 0) return quiesce<pvNode>(alpha, beta, stack);

    const bool allNode = (!pvNode && !cutNode);
    const bool nullSearch = ((stack - 1)->move == NULLMOVE);
    const int alphaOrig = alpha;
    const uint64_t key = board.key;
    uint16_t ttMove = NULLMOVE;
    uint16_t nrQuiets = 0;
    uint16_t nrCaptures = 0;
    int played = 0, bound = NONE, skip = 0;
    int best = -INF;
    uint16_t bestMove = NULLMOVE;
    int ttValue = 0, ttDepth = -100;
    bool ttHit = false;

    nodes++;
    selDepth = std::max(selDepth, ply);

    pvTableLen[ply] = 0;

    if constexpr (!rootNode) {
        if (board.is_draw(ply)) return 1 - (nodes & 2); /// beal effect, credit to Ethereal for this

        /// mate distance pruning   
        alpha = std::max(alpha, -INF + ply), beta = std::min(beta, INF - ply - 1);
        if (alpha >= beta) return alpha;
    }

    Entry* entry = TT->probe(key, ttHit);

    /// transposition table probing
    int eval = INF;
    bool wasPV = pvNode;

    if (!stack->excluded && ttHit) {
        const int score = entry->value(ply);
        ttValue = score;
        bound = entry->bound();
        ttMove = entry->move;
        eval = entry->eval;
        ttDepth = entry->depth();
        wasPV |= entry->wasPV();
        if constexpr (!pvNode) {
            if (score != VALUE_NONE && ttDepth >= depth && (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))) return score;
        }
    }

    /// tablebase probing
    if constexpr (!rootNode) {
        const auto probe = probe_TB(board, depth);
        if (probe != TB_RESULT_FAILED) {
            int type = NONE, score;
            tbHits++;
            switch (probe) {
            case TB_WIN:
                score = TB_WIN_SCORE - DEPTH - ply;
                type = LOWER;
                break;
            case TB_LOSS:
                score = -(TB_WIN_SCORE - DEPTH - ply);
                type = UPPER;
                break;
            default:
                score = 0;
                type = EXACT;
                break;
            }

            if (type == EXACT || (type == UPPER && score <= alpha) || (type == LOWER && score >= beta)) {
                TT->save(entry, key, score, DEPTH, 0, type, NULLMOVE, 0, wasPV);
                return score;
            }
        }
    }

    const bool isCheck = (board.checkers != 0);
    const Threats threatsEnemy = getThreats(board, board.turn);
    const bool quietUs = !threatsEnemy.threatsPieces;
    //int quietEnemy = quietness(board, 1 ^ board.turn);

    if (isCheck) { /// when in check, don't evaluate
        stack->eval = eval = INF;
    }
    else if (!ttHit) { /// if we are in a singular search, we already know the evaluation
        if (stack->excluded) eval = stack->eval;
        else {
            stack->eval = eval = evaluate(board);
            TT->save(entry, key, VALUE_NONE, 0, ply, 0, NULLMOVE, eval, wasPV);
        }
    }
    else { /// ttValue might be a better evaluation
        stack->eval = eval;
        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval)) eval = ttValue;
    }

    const int staticEval = stack->eval;
    int eval_diff = 0;
    if (ply > 1 && (stack - 2)->eval != INF)
        eval_diff = staticEval - (stack - 2)->eval;
    else if (ply > 3 && (stack - 4)->eval != INF)
        eval_diff = staticEval - (stack - 4)->eval;

    const int improving = (isCheck || eval_diff == 0 ? 0 :
        eval_diff > 0 ? 1 :
        eval_diff < NegativeImprovingMargin ? -1 : 0);

    (stack + 1)->killer = NULLMOVE;

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    if (pvNode && depth >= IIRPvNodeDepth && !ttHit) depth -= IIRPvNodeReduction;
    /// also for cut nodes
    if (cutNode && depth >= IIRCutNodeDepth && !ttHit) depth -= IIRCutNodeReduction;

    if constexpr (!pvNode) {
        if (!isCheck) {
            // razoring
            if (depth <= RazoringDepth && eval + RazoringMargin * depth <= alpha) {
                int value = quiesce<false>(alpha, alpha + 1, stack);
                if (value <= alpha) return value;
            }

            /// static null move pruning (don't prune when having a mate line, again stability)
            if (depth <= SNMPDepth && eval > beta && eval - (SNMPMargin - SNMPImproving * improving) * (depth - quietUs) > beta && eval < MATE) return eval;

            /// null move pruning (when last move wasn't null, we still have non pawn material, we have a good position)
            if (!nullSearch && !stack->excluded && quietUs &&
                eval >= beta + NMPEvalMargin * (depth <= 3) && eval >= staticEval &&
                board.has_non_pawn_material(board.turn)) {
                int R = NMPReduction + depth / NMPDepthDiv + (eval - beta) / NMPEvalDiv + improving;

                stack->move = NULLMOVE;
                stack->piece = 0;
                stack->continuationHist = &continuationHistory[0][0][0];

                board.make_null_move();
                int score = -search<false, false>(-beta, -beta + 1, depth - R, !cutNode, stack + 1);
                board.undo_null_move();

                if (score >= beta) return (abs(score) > MATE ? beta : score); /// don't trust mate scores
            }

            // probcut
            int probBeta = beta + ProbcutMargin;
            if (depth >= ProbcutDepth && abs(beta) < MATE && !(ttHit && ttDepth >= depth - 3 && ttValue < probBeta)) {
                Movepick picker((ttMove && isNoisyMove(board, ttMove) && see(board, ttMove, probBeta - staticEval) ? ttMove : NULLMOVE),
                    NULLMOVE,
                    NULLMOVE,
                    probBeta - staticEval,
                    threatsEnemy.threatsEnemy);

                uint16_t move;
                while ((move = picker.get_next_move(this, stack, board, true, true)) != NULLMOVE) {
                    if (move == stack->excluded)
                        continue;

                    stack->move = move;
                    stack->piece = board.piece_at(sq_from(move));
                    stack->continuationHist = &continuationHistory[0][stack->piece][sq_to(move)];

                    board.make_move(move);

                    int score = -quiesce<false>(-probBeta, -probBeta + 1, stack + 1);

                    if (score >= probBeta) score = -search<false, false>(-probBeta, -probBeta + 1, depth - ProbcutReduction, !cutNode, stack + 1);
                    board.undo_move(move);

                    if (score >= probBeta) {
                        TT->save(entry, key, score, depth - 3, ply, LOWER, move, staticEval, wasPV);
                        return score;
                    }
                }
            }
        }
    }

    constexpr int see_depth_coef = (rootNode ? RootSeeDepthCoef : PVSSeeDepthCoef);

    Movepick picker(ttMove,
        stack->killer,
        nullSearch ? NULLMOVE : cmTable[(stack - 1)->piece][sq_to((stack - 1)->move)], // counter
        -see_depth_coef * depth,
        threatsEnemy.threatsEnemy);

    uint16_t move;
    const bool ttCapture = ttMove && isNoisyMove(board, ttMove);

    while ((move = picker.get_next_move(this, stack, board, skip, false)) != NULLMOVE) {
        if constexpr (rootNode) {
            bool searched = false;
            for (int i = 1; i < multipv_index; i++) {
                if (move == bestMoves[i]) {
                    searched = true;
                    break;
                }
            }
            if (searched) continue;
        }
        if (move == stack->excluded)
            continue;

        const bool isQuiet = !isNoisyMove(board, move), refutationMove = (picker.trueStage == STAGE_KILLER || picker.trueStage == STAGE_COUNTER);
        int hist = 0;

        /// quiet move pruning

#ifdef GENERATE
        if (best > -MATE && board.has_non_pawn_material(board.turn) && !pvNode) {
            if (isQuiet) {
                getHistory(this, stack, move, threatsEnemy.threatsEnemy, hist);

                /// approximately the new depth for the next search
                int newDepth = std::max(0, depth - lmrRed[std::min(63, depth)][std::min(63, played)] + improving);

                /// futility pruning
                if (newDepth <= FPDepth && !isCheck && staticEval + FPBias + FPMargin * newDepth <= alpha) skip = 1;

                /// late move pruning
                if (newDepth <= LMPDepth && played >= (LMPBias + newDepth * newDepth) / (2 - improving)) skip = 1;

                if (depth <= SEEPruningQuietDepth && !isCheck && !see(board, move, -SEEPruningQuietMargin * depth)) continue;
            }
            else {
                if (depth <= SEEPruningNoisyDepth && !isCheck && picker.trueStage > STAGE_GOOD_NOISY && !see(board, move, -SEEPruningNoisyMargin * depth * depth)) continue;

                if (depth <= FPNoisyDepth && !isCheck && staticEval + FPBias + seeVal[board.piece_type_at(sq_to(move))] + FPMargin * depth <= alpha) continue;
            }
        }
#else
        if constexpr (!rootNode) {
            if (best > -MATE && board.has_non_pawn_material(board.turn)) {
                if (isQuiet) {
                    getHistory(this, stack, move, threatsEnemy.threatsEnemy, hist);

                    /// approximately the new depth for the next search
                    int newDepth = std::max(0, depth - lmrRed[std::min(63, depth)][std::min(63, played)] + improving + hist / 16384);

                    /// futility pruning
                    if (newDepth <= FPDepth && !isCheck && staticEval + FPBias + FPMargin * newDepth <= alpha) skip = 1;

                    /// late move pruning
                    if (newDepth <= LMPDepth && played >= (LMPBias + newDepth * newDepth) / (2 - improving)) skip = 1;

                    if (newDepth <= SEEPruningQuietDepth && !isCheck && !see(board, move, -SEEPruningQuietMargin * newDepth)) continue;
                }
                else {
                    if (depth <= SEEPruningNoisyDepth && !isCheck && picker.trueStage > STAGE_GOOD_NOISY && !see(board, move, -SEEPruningNoisyMargin * depth * depth)) continue;

                    if (depth <= FPNoisyDepth && !isCheck && staticEval + FPBias + seeVal[board.piece_type_at(sq_to(move))] + FPMargin * depth <= alpha) continue;
                }
            }
        }
#endif

        int ex = 0;
        /// avoid extending too far (might cause stack overflow)
        if (ply < 2 * tDepth && !rootNode) {
            /// singular extension (look if the tt move is better than the rest)
            if (!stack->excluded && !allNode && move == ttMove && abs(ttValue) < MATE && depth >= SEDepth && ttDepth >= depth - 3 && (bound & LOWER)) {
                int rBeta = ttValue - SEMargin * depth / 64;

                stack->excluded = move;
                int score = search<false, false>(rBeta - 1, rBeta, (depth - 1) / 2, cutNode, stack);
                stack->excluded = NULLMOVE;

                if (score < rBeta) ex = 1 + (!pvNode && rBeta - score > SEDoubleExtensionsMargin) + (!pvNode && !ttCapture && rBeta - score > 100);
                else if (rBeta >= beta) return rBeta; // multicut
                else if (ttValue >= beta || ttValue <= alpha) ex = -1 - !pvNode;
            }
            else if (isCheck) ex = 1;
        }
        else if (allNode && played >= 1 && ttDepth >= depth - 3 && bound == UPPER) ex = -1;

        /// update stack info
        TT->prefetch(board.speculative_next_key(move));
        stack->move = move;
        stack->piece = board.piece_at(sq_from(move));
        stack->continuationHist = &continuationHistory[isQuiet][stack->piece][sq_to(move)];

        board.make_move(move);
        played++;

        if constexpr (rootNode) {
            /// current root move info
            if (principalSearcher && printStats && getTime() > info->startTime + 2500 && !info->sanMode) {
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
                R = lmrRed[std::min(63, depth)][std::min(63, played)];
                R += !wasPV + (improving <= 0); /// not on pv or not improving
                R -= !rootNode && pvNode;
                R += quietUs && !isCheck && eval - seeVal[KNIGHT] > beta; /// if the position is relatively quiet and eval is bigger than beta by a margin
                R += quietUs && !isCheck && staticEval - rootEval > EvalDifferenceReductionMargin && ply % 2 == 0; /// the position in quiet and static eval is way bigger than root eval
                R -= 2 * refutationMove; /// reduce for refutation moves
                R -= board.checkers != 0; /// move gives check
                R -= hist / HistReductionDiv; /// reduce based on move history
            }
            else if (!wasPV) {
                R = lmrRedNoisy[std::min(63, depth)][std::min(63, played)];
                R += improving <= 0; /// not improving
                R += quietUs && picker.trueStage == STAGE_BAD_NOISY; /// if the position is relatively quiet and the capture is "very losing"
            }

            R += 2 * cutNode;
            R -= wasPV && ttDepth >= depth;
            R += ttCapture;
            R += quietUs && !isCheck && staticEval + 250 <= alpha;

            R = std::min(newDepth, std::max(R, 1)); /// clamp R
            score = -search<false, false>(-alpha - 1, -alpha, newDepth - R, true, stack + 1);

            if (R > 1 && score > alpha) {
                newDepth += (score > best + 75) - (score < best + newDepth);
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
        nodesSearched[fromTo(move)] += nodes - initNodes;

        if (flag & TERMINATED_SEARCH) /// stop search
            return best;

        if (score > best) {
            best = score;
            bestMove = move;
            if (score > alpha) {
                if constexpr (rootNode) {
                    bestMoves[multipv_index] = move;
                    rootScores[multipv_index] = score;
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
        if (!isNoisyMove(board, bestMove)) {
            if (stack->killer != bestMove)
                stack->killer = bestMove;
            updateHistory(this, stack, nrQuiets, ply, threatsEnemy.threatsEnemy, getHistoryBonus(depth + (staticEval <= alpha)));
        }
        updateCapHistory(this, stack, nrCaptures, bestMove, ply, getHistoryBonus(depth + (staticEval <= alpha)));
    }

    /// update tt only if we aren't in a singular search
    if (!stack->excluded) {
        bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
        TT->save(entry, key, best, depth, ply, bound, (bound == UPPER ? NULLMOVE : bestMove), staticEval, wasPV);
    }

    return best;
}

std::pair <int, uint16_t> Search::start_search(Info* _info) {
    int alpha, beta;

    nodes = qsNodes = selDepth = tbHits = 0;
    t0 = getTime();
    flag = 0;
    checkCount = 0;
    bestMoveCnt = 0;

    memset(pvTable, 0, sizeof(pvTable));
    info = _info;

    if (principalSearcher) {
        /// corner cases
        uint16_t moves[256];
        int nrMoves = gen_legal_moves(board, moves);

        /// only 1 move legal
        if (PROBE_ROOT && nrMoves == 1) {
            if (printStats)
                std::cout << "bestmove " << move_to_string(moves[0], info->chess960) << std::endl;
            return { 0, moves[0] };
        }

        /// position is in tablebase
        if (PROBE_ROOT && count(board.pieces[WHITE] | board.pieces[BLACK]) <= (int)TB_LARGEST) {
            int move = NULLMOVE;
            auto probe = probe_TB(board, 0, 1, board.halfMoves, (board.castleRights & (3 << board.turn)) > 0);
            if (probe != TB_RESULT_CHECKMATE && probe != TB_RESULT_FAILED && probe != TB_RESULT_STALEMATE) {
                int to = int(TB_GET_TO(probe)), from = int(TB_GET_FROM(probe)), promote = TB_GET_PROMOTES(probe), ep = TB_GET_EP(probe);
                if (!promote && !ep) {
                    move = getMove(from, to, 0, NEUT);
                }
                else {
                    int prom = 0;
                    switch (promote) {
                    case TB_PROMOTES_KNIGHT:
                        prom = KNIGHT;
                        break;
                    case TB_PROMOTES_BISHOP:
                        prom = BISHOP;
                        break;
                    case TB_PROMOTES_ROOK:
                        prom = ROOK;
                        break;
                    case TB_PROMOTES_QUEEN:
                        prom = QUEEN;
                        break;
                    }
                    move = getMove(from, to, prom - KNIGHT, PROMOTION);
                }
            }

            for (int i = 0; i < nrMoves; i++) {
                if (moves[i] == move) {
                    if (printStats)
                        std::cout << "bestmove " << move_to_string(move, info->chess960) << std::endl;
                    return { 0, move };
                }
            }
        }
    }

    if (principalSearcher)
        start_workers(info);

    uint64_t totalNodes = 0, totalHits = 0;
    int limitDepth = (principalSearcher ? info->depth : DEPTH); /// when limited by depth, allow helper threads to pass the fixed depth
    int mainThreadScore = 0;
    uint16_t mainThreadBestMove = NULLMOVE;

    memset(scores, 0, sizeof(scores));
    StackEntry search_stack[DEPTH + 15];
    StackEntry* stack = search_stack + 10;

    memset(search_stack, 0, sizeof(search_stack));
    memset(bestMoves, 0, sizeof(bestMoves));
    memset(ponderMoves, 0, sizeof(ponderMoves));
    memset(rootScores, 0, sizeof(rootScores));

    rootEval = (!board.checkers ? evaluate(board) : INF);

    for (int i = 1; i <= 10; i++)
        (stack - i)->continuationHist = &continuationHistory[0][0][0], (stack - i)->eval = INF, (stack - i)->move = NULLMOVE;

    //values[0].init("nmp_pv_rate");

    completedDepth = 0;

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
                selDepth = 0;
                scores[i] = search<true, true>(alpha, beta, depth, false, stack);
                if (flag & TERMINATED_SEARCH)
                    break;

                if (principalSearcher && printStats && ((alpha < scores[i] && scores[i] < beta) || (i == 1 && getTime() > t0 + 3000))) {
                    if (principalSearcher) {
                        totalNodes = nodes;
                        totalHits = tbHits;
                        for (int i = 0; i < threadCount; i++) {
                            totalNodes += params[i].nodes;
                            totalHits += params[i].tbHits;
                        }
                    }
                    uint64_t t = (uint64_t)getTime() - t0;
                    if (!info->sanMode) {
                        std::cout << "info ";
                        std::cout << "multipv " << i << " ";
                        std::cout << "score ";
                        if (scores[i] > MATE)
                            std::cout << "mate " << (INF - scores[i] + 1) / 2;
                        else if (scores[i] < -MATE)
                            std::cout << "mate -" << (INF + scores[i] + 1) / 2;
                        else
                            std::cout << "cp " << scores[i] * 100 / NormalizeToPawnValue;
                        if (scores[i] >= beta)
                            std::cout << " lowerbound";
                        else if (scores[i] <= alpha)
                            std::cout << " upperbound";
                        int ply = board.moveIndex * 2 - 1 - board.turn;
                        int wdlWin = winrate_model(scores[i], ply);
                        int wdlLose = winrate_model(-scores[i], ply);
                        int wdlDraw = 1000 - wdlWin - wdlLose;
                        std::cout << " wdl " << wdlWin << " " << wdlDraw << " " << wdlLose;
                        std::cout << " depth " << depth << " seldepth " << selDepth << " nodes " << totalNodes << " qsNodes " << qsNodes;
                        if (t)
                            std::cout << " nps " << totalNodes * 1000 / t;
                        std::cout << " time " << t << " ";
                        std::cout << "tbhits " << totalHits << " hashfull " << TT->tableFull() << " ";
                        std::cout << "pv ";
                        printPv();
                        std::cout << std::endl;
                    }
                    else {
                        std::cout << std::setw(3) << depth << "/" << std::setw(3) << selDepth << " ";
                        if (t < 10 * 1000)
                            std::cout << std::setw(7) << std::setprecision(2) << t / 1000.0 << "s   ";
                        else
                            std::cout << std::setw(7) << t / 1000 << "s   ";
                        std::cout << std::setw(7) << totalNodes * 1000 / (t + 1) << "n/s   ";
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
                        printPv();
                        std::cout << std::endl;
                        //std::cout << "Branching factor is " << std::fixed << std::setprecision(5) << pow(nodes, 1.0 / tDepth) << std::endl;
                    }
                }

                if (scores[i] <= alpha) {
                    beta = (beta + alpha) / 2;
                    alpha = std::max(-INF, alpha - window);
                    depth = tDepth;
                    completedDepth = tDepth - 1;
                }
                else if (beta <= scores[i]) {
                    beta = std::min(INF, beta + window);
                    depth--;
                    completedDepth = tDepth;
                    if (pvTableLen[0] > 1)
                        ponderMoves[i] = pvTable[0][1];
                    else
                        ponderMoves[i] = NULLMOVE;
                }
                else {
                    completedDepth = tDepth;
                    if (pvTableLen[0] > 1)
                        ponderMoves[i] = pvTable[0][1];
                    else
                        ponderMoves[i] = NULLMOVE;
                    break;
                }

                window += window * AspirationWindowExpandMargin / 100 + AspirationWindowExpandBias;
            }
        }

        if (principalSearcher && !(flag & TERMINATED_SEARCH)) {
            double scoreChange = 1.0, bestMoveStreak = 1.0, nodesSearchedPercentage = 1.0;
            float _tmNodesSearchedMaxPercentage = TimeManagerNodesSearchedMaxPercentage / 1000.0;
            float _tmBestMoveMax = TimeManagerBestMoveMax / 1000.0;
            float _tmBestMoveStep = TimeManagerBestMoveStep / 1000.0;
            if (tDepth >= TimeManagerMinDepth) {
                scoreChange = std::max(TimeManagerScoreMin / 100.0, std::min(TimeManagerScoreBias / 100.0 + 1.0 * (mainThreadScore - rootScores[1]) / TimeManagerScoreDiv, TimeManagerScoreMax / 100.0)); /// adjust time based on score change
                bestMoveCnt = (bestMoves[1] == mainThreadBestMove ? bestMoveCnt + 1 : 1);
                /// adjust time based on how many nodes from the total searched nodes were used for the best move
                nodesSearchedPercentage = 1.0 * nodesSearched[fromTo(bestMoves[1])] / nodes;
                nodesSearchedPercentage = TimeManagerNodesSeachedMaxCoef / 100.0 * _tmNodesSearchedMaxPercentage -
                    TimeManagerNodesSearchedCoef / 100.0 * nodesSearchedPercentage;
                bestMoveStreak = _tmBestMoveMax - _tmBestMoveStep * std::min(10, bestMoveCnt); /// adjust time based on how long the best move was the same
            }
            //std::cout << "Scale factor for tm is " << scoreChange * bestMoveStreak * nodesSearchedPercentage * 100 << "%\n";
            //std::cout << scoreChange * 100 << " " << bestMoveStreak * 100 << " " << _tmNodesSearchedMaxPercentage - nodesSearchedPercentage << "\n";
            info->stopTime = info->startTime + info->goodTimeLim * scoreChange * bestMoveStreak * nodesSearchedPercentage;
            mainThreadScore = rootScores[1];
            mainThreadBestMove = bestMoves[1];
        }

        if (flag & TERMINATED_SEARCH)
            break;

        if (info->ponder) continue;

        if (info->timeset && getTime() > info->stopTime) {
            flag |= TERMINATED_BY_LIMITS;
            break;
        }

        if (tDepth == limitDepth) {
            flag |= TERMINATED_BY_LIMITS;
            break;
        }
    }

    int bs = 0;
    uint16_t bm = NULLMOVE, pm = NULLMOVE;
    if (principalSearcher) {
        stop_workers();
        int bestDepth = completedDepth;
        bs = rootScores[1];
        bm = bestMoves[1];
        pm = ponderMoves[1];
        for (int i = 0; i < threadCount; i++) {
            if (params[i].rootScores[1] > bs && params[i].completedDepth >= bestDepth) {
                bs = params[i].rootScores[1];
                bm = params[i].bestMoves[1];
                pm = params[i].ponderMoves[1];
                bestDepth = params[i].completedDepth;
            }
        }
    }

    while (!(flag & TERMINATED_SEARCH) && info->ponder && info->timeset);

    if (principalSearcher && printStats) {
        std::cout << "bestmove " << move_to_string(bm, info->chess960);
        if (pm)
            std::cout << " ponder " << move_to_string(pm, info->chess960);
        std::cout << std::endl;
    }

    //TT->age();

    return std::make_pair(bs, bm);
}

void Search::clear_history() {
    memset(hist, 0, sizeof(hist));
    memset(capHist, 0, sizeof(capHist));
    memset(cmTable, 0, sizeof(cmTable));
    memset(continuationHistory, 0, sizeof(continuationHistory));
    for (int i = 0; i < threadCount; i++) {
        memset(params[i].hist, 0, sizeof(params[i].hist));
        memset(params[i].capHist, 0, sizeof(params[i].capHist));
        memset(params[i].cmTable, 0, sizeof(params[i].cmTable));
        memset(params[i].continuationHistory, 0, sizeof(params[i].continuationHistory));
    }
}

void Search::clear_stack() {
    memset(pvTableLen, 0, sizeof(pvTableLen));
    memset(pvTable, 0, sizeof(pvTable));
    memset(nodesSearched, 0, sizeof(nodesSearched));
    for (int i = 0; i < threadCount; i++) {
        memset(params[i].pvTableLen, 0, sizeof(params[i].pvTableLen));
        memset(params[i].pvTable, 0, sizeof(params[i].pvTable));
        memset(params[i].nodesSearched, 0, sizeof(params[i].nodesSearched));
    }
}

void Search::start_workers(Info* info) {
    for (int i = 0; i < threadCount; i++) {
        params[i].readyMutex.lock();
        params[i].nodes = 0;
        params[i].selDepth = 0;
        params[i].tbHits = 0;
        params[i].setTime(info);
        params[i].t0 = t0;
        params[i].SMPThreadExit = false;
        params[i].lazyFlag = 1;
        params[i].flag = flag;
        params[i].readyMutex.unlock();
        params[i].lazyCV.notify_one();
    }
}

void Search::flag_workers_stop() {
    for (int i = 0; i < threadCount; i++) {
        params[i].SMPThreadExit = true;
        params[i].flag |= TERMINATED_BY_LIMITS;
    }
}

void Search::stop_workers() {
    flag_workers_stop();
    for (int i = 0; i < threadCount; i++) {
        while (params[i].lazyFlag);
    }
}

void Search::isReady() {
    flag_workers_stop();
    flag |= TERMINATED_BY_USER;
    std::unique_lock <std::mutex> lk(readyMutex);
    std::cout << "readyok" << std::endl;
}

void Search::start_principal_search(Info* info) {
    principalSearcher = true;
    readyMutex.lock();
    setTime(info);
    lazyFlag = 1;
    readyMutex.unlock();
    lazyCV.notify_one();
    if (!principalThread)
        principalThread.reset(new std::thread(&Search::lazySMPSearcher, this));
}

void Search::stop_principal_search() {
    info->ponder = false;
    flag |= TERMINATED_BY_USER;
}

void Search::set_num_threads(int nrThreads) {
    if (nrThreads == threadCount)
        return;
    releaseThreads();
    threadCount = nrThreads;
    threads.reset(new std::thread[nrThreads]);
    params.reset(new Search[nrThreads]);
    for (int i = 0; i < nrThreads; i++)
        threads[i] = std::thread(&Search::lazySMPSearcher, &params[i]);
}

void Search::lazySMPSearcher() {
    while (!terminateSMP) {
        {
            std::unique_lock <std::mutex> lk(readyMutex);
            lazyCV.wait(lk, std::bind(&Search::isLazySMP, this));
            if (terminateSMP)
                return;

            start_search(info);
            resetLazySMP();
        }
    }
}

void Search::releaseThreads() {
    for (int i = 0; i < threadCount; i++) {
        if (threads[i].joinable()) {
            params[i].terminateSMP = true;
            {
                std::unique_lock <std::mutex> lk(params[i].readyMutex);
                params[i].lazyFlag = 1;
            }
            params[i].lazyCV.notify_one();
            threads[i].join();
        }
    }
}

void Search::kill_principal_search() {
    if (principalThread && principalThread->joinable()) {
        terminateSMP = true;
        {
            std::unique_lock <std::mutex> lk(readyMutex);
            lazyFlag = 1;
        }
        lazyCV.notify_one();
        principalThread->join();
    }
}

void Search::set_fen(std::string fen, bool chess960) {
    board.chess960 = chess960;
    board.set_fen(fen);
    for (int i = 0; i < threadCount; i++) {
        params[i].board.chess960 = chess960;
        params[i].board.set_fen(fen);
    }
}

void Search::set_dfrc(int idx) {
    board.chess960 = (idx > 0);
    board.set_dfrc(idx);
    for (int i = 0; i < threadCount; i++) {
        params[i].board.chess960 = (idx > 0);
        params[i].board.set_dfrc(idx);
    }
}

void Search::make_move(uint16_t move) {
    board.make_move(move);
    for (int i = 0; i < threadCount; i++)
        params[i].board.make_move(move);
}

void Search::clear_board() {
    board.clear();
    for (int i = 0; i < threadCount; i++)
        params[i].board.clear();
}