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
#include "tbprobe.h"
#include "thread.h"
#include <cstring>
#include <cmath>
#include <fstream>

bool Search::checkForStop() {
    if (flag & TERMINATED_SEARCH)
        return 1;

    if (SMPThreadExit) {
        flag |= TERMINATED_BY_LIMITS;
        return 1;
    }

    if (info->nodes != -1 && info->nodes <= (int64_t)nodes) {
        flag |= TERMINATED_BY_LIMITS;
        return 1;
    }

    checkCount++;

    if (checkCount == (1 << 10)) {
        if (info->timeset && getTime() > info->startTime + info->hardTimeLim)
            flag |= TERMINATED_BY_LIMITS;
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

bool quietness(Board& board, bool us) {
    if (board.checkers)
        return 0;
    uint64_t att;
    int enemy = 1 ^ us;
    uint64_t pieces, b, all = board.pieces[WHITE] | board.pieces[BLACK];

    if (getPawnAttacks(enemy, board.bb[getType(PAWN, enemy)]) &
        (board.bb[getType(KNIGHT, us)] | board.bb[getType(BISHOP, us)] | board.bb[getType(ROOK, us)] | board.bb[getType(QUEEN, us)]))
        return 0;

    pieces = board.bb[getType(KNIGHT, enemy)];
    att = 0;
    while (pieces) {
        b = lsb(pieces);
        att |= knightBBAttacks[Sq(b)];
        pieces ^= b;
    }

    if (att & (board.bb[getType(QUEEN, us)] | board.bb[getType(ROOK, us)]))
        return 0;

    pieces = board.bb[getType(BISHOP, enemy)];
    att = 0;
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksBishop(all, Sq(b));
        pieces ^= b;
    }

    if (att & (board.bb[getType(QUEEN, us)] | board.bb[getType(ROOK, us)]))
        return 0;

    pieces = board.bb[getType(ROOK, enemy)];
    att = 0;
    while (pieces) {
        b = lsb(pieces);
        att |= genAttacksRook(all, Sq(b));
        pieces ^= b;
    }

    if (att & board.bb[getType(QUEEN, us)])
        return 0;

    return 1;
}

std::string getSanString(Board& board, uint16_t move) {
    if (type(move) == CASTLE)
        return ((sqTo(move) & 7) == 6 ? "O-O" : "O-O-O");
    int from = sqFrom(move), to = sqTo(move), prom = (type(move) == PROMOTION ? promoted(move) + KNIGHT + 6 : 0), piece = board.piece_type_at(from);
    std::string san = "";

    if (piece != PAWN) {
        san += pieceChar[piece + 6];
    }
    else {
        san += char('a' + (from & 7));
    }
    if (board.isCapture(move))
        san += "x";
    if (piece != PAWN || board.isCapture(move))
        san += char('a' + (to & 7));
    san += char('1' + (to >> 3));

    if (prom)
        san += "=", san += pieceChar[prom];

    board.makeMove(move);
    if (board.checkers)
        san += '+';
    board.undoMove(move);
    return san;
}

void Search::printPv() {
    for (int i = 0; i < pvTableLen[0]; i++) {
        if (!info->sanMode)
            std::cout << toString(pvTable[0][i]) << " ";
        else {
            std::cout << getSanString(board, pvTable[0][i]) << " ";
            board.makeMove(pvTable[0][i]);
        }
    }

    if (info->sanMode) {
        for (int i = pvTableLen[0] - 1; i >= 0; i--)
            board.undoMove(pvTable[0][i]);
    }
}

void Search::updatePv(int ply, int move) {
    pvTable[ply][0] = move;
    for (int i = 0; i < pvTableLen[ply + 1]; i++)
        pvTable[ply][i + 1] = pvTable[ply + 1][i];
    pvTableLen[ply] = 1 + pvTableLen[ply + 1];
}

int Search::quiesce(int alpha, int beta, StackEntry* stack, bool useTT) {
    int ply = board.ply;

    if (ply >= DEPTH)
        return evaluate(board);

    pvTableLen[ply] = 0;

    nodes++;
    qsNodes++;

    if (isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw()) /// check for draw
        return 1 - (nodes & 2);

    if (checkForStop())
        return ABORT;

    bool pvNode = (beta - alpha > 1);
    uint64_t key = board.key;
    int score = 0, best = -INF, alphaOrig = alpha;
    int bound = NONE;
    uint16_t bestMove = NULLMOVE, ttMove = NULLMOVE;

    TT->prefetch(key);

    tt::Entry entry = {};

    int eval = INF, ttValue = 0;

    /// probe transposition table
    if (TT->probe(key, entry)) {
        best = eval = entry.eval;
        ttValue = score = entry.value(ply);
        bound = entry.bound();
        ttMove = entry.move;

        if (!pvNode && (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha)))
            return score;
    }

    bool isCheck = (board.checkers != 0);
    int futilityValue;

    if (isCheck) {
        stack->eval = INF;
        best = futilityValue = -INF;
    }
    else if (eval == INF) {
        stack->eval = best = eval = evaluate(board);
        futilityValue = best + quiesceFutilityCoef;
    }
    else { /// ttValue might be a better evaluation
        stack->eval = eval;
        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval))
            best = ttValue;

        futilityValue = best + quiesceFutilityCoef;
    }

    /// stand-pat
    if (best >= beta) {
        return best;
    }

    /// delta pruning
    if (!isCheck && best + seeVal[QUEEN] < alpha) {
        return alpha;
    }

    alpha = std::max(alpha, best);

    Movepick noisyPicker((!isCheck && see(board, ttMove, 0) ? ttMove : NULLMOVE), NULLMOVE, NULLMOVE, 0);

    uint16_t move;

    while ((move = noisyPicker.nextMove(this, stack, board, !isCheck, true))) {
        // futility pruning
        if (best > -MATE) {
            if (futilityValue > -MATE) {
                int value = futilityValue + seeVal[board.piece_type_at(sqTo(move))];
                if (type(move) != PROMOTION && value <= alpha) {
                    best = std::max(best, value);
                    continue;
                }
            }

            if (isCheck && !isNoisyMove(board, move) && !see(board, move, 0)) {
                continue;
            }
        }
        // update stack info
        stack->move = move;
        stack->piece = board.piece_at(sqFrom(move));
        stack->continuationHist = &continuationHistory[stack->piece][sqTo(move)];

        board.makeMove(move);
        TT->prefetch(board.key);
        score = -quiesce(-beta, -alpha, stack + 1);
        board.undoMove(move);

        if (flag & TERMINATED_SEARCH)
            return ABORT;

        if (score > best) {
            best = score;
            bestMove = move;

            if (score > alpha) {
                alpha = score;
                updatePv(ply, move);

                if (alpha >= beta)
                    break;
            }
        }
    }

    if (isCheck && best == -INF) {
        return -INF + ply;
    }

    /// store info in transposition table
    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(key, best, 0, ply, bound, bestMove, stack->eval);

    return best;
}

int Search::search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack, uint16_t excluded) {
    int ply = board.ply;

    if (checkForStop())
        return ABORT;

    if (ply >= DEPTH)
        return evaluate(board);

    if (depth <= 0)
        return quiesce(alpha, beta, stack);

    bool pvNode = (alpha < beta - 1);
    uint16_t ttMove = NULLMOVE;
    int alphaOrig = alpha;
    uint64_t key = board.key;
    uint16_t nrQuiets = 0;
    uint16_t nrCaptures = 0;
    int played = 0, bound = NONE, skip = 0;
    int best = -INF;
    uint16_t bestMove = NULLMOVE;
    int ttHit = 0, ttValue = 0;
    bool nullSearch = ((stack - 1)->move == NULLMOVE);

    nodes++;
    selDepth = std::max(selDepth, ply);

    pvTableLen[ply] = 0;

    if (isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw())
        return 1 - (nodes & 2); /// beal effect apparently, credit to Ethereal for this

    /// mate distance pruning
    alpha = std::max(alpha, -INF + ply), beta = std::min(beta, INF - ply - 1);
    if (alpha >= beta)
        return alpha;

    tt::Entry entry = {};

    /// transposition table probing
    int eval = INF;

    if (!excluded && TT->probe(key, entry)) {
        int score = entry.value(ply);
        ttHit = 1;
        ttValue = score;
        bound = entry.bound(), ttMove = entry.move;
        eval = entry.eval;
        if (entry.depth() >= depth && !pvNode && (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))) {
            return score;
        }
    }

    /// tablebase probing
    auto probe = probe_TB(board, depth);
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
            TT->save(key, score, DEPTH, 0, type, NULLMOVE, 0);
            return score;
        }
    }

    bool isCheck = (board.checkers != 0);
    bool quietUs = quietness(board, board.turn);
    //int quietEnemy = quietness(board, 1 ^ board.turn);

    if (isCheck) { /// when in check, don't evaluate
        stack->eval = eval = INF;
    }
    else if (!ttHit) { /// if we are in a singular search, we already know the evaluation
        if (excluded)
            eval = stack->eval;
        else
            stack->eval = eval = evaluate(board);
    }
    else { /// ttValue might be a better evaluation
        stack->eval = eval;
        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval))
            eval = ttValue;
    }

    int staticEval = stack->eval;
    bool improving = (!isCheck && staticEval > (stack - 2)->eval); /// (TO DO: make all pruning dependent of this variable?)

    (stack + 1)->killer = NULLMOVE;

    if (!pvNode && !isCheck) {
        /// razoring (searching 1 more ply can't change the score much, drop in quiesce)
        if (depth <= 1 && staticEval + RazorCoef < alpha) {
            return quiesce(alpha, beta, stack);
        }

        /// static null move pruning (don't prune when having a mate line, again stability)
        if (depth <= SNMPDepth && eval > beta && eval - (SNMPCoef1 - SNMPCoef2 * improving) * (depth - quietUs) > beta && eval < MATE) {
            return eval;
        }

        /// null move pruning (when last move wasn't null, we still have non pawn material, we have a good position)
        if (!nullSearch && !excluded && depth >= 2 && (quietUs || eval - 100 * depth > beta) &&
            eval >= beta && eval >= staticEval &&
            board.hasNonPawnMaterial(board.turn)) {
            int R = nmpR + depth / nmpDepthDiv + (eval - beta) / nmpEvalDiv + improving;

            stack->move = NULLMOVE;
            stack->piece = 0;
            stack->continuationHist = &continuationHistory[0][0];

            board.makeNullMove();

            int score = -search(-beta, -beta + 1, depth - R, !cutNode, stack + 1);

            board.undoNullMove();

            if (score >= beta) {
                return (abs(score) > MATE ? beta : score); /// don't trust mate scores
            }
        }

        /// probcut
        if (depth >= probcutDepth && abs(beta) < MATE) {
            int cutBeta = beta + probcutMargin;
            Movepick noisyPicker(NULLMOVE, NULLMOVE, NULLMOVE, cutBeta - staticEval);

            uint16_t move;

            while ((move = noisyPicker.nextMove(this, stack, board, true, true)) != NULLMOVE) {
                if (move == excluded)
                    continue;

                stack->move = move;
                stack->piece = board.piece_at(sqFrom(move));
                stack->continuationHist = &continuationHistory[stack->piece][sqTo(move)];

                board.makeMove(move);

                /// do we have a good sequence of captures that beats cutBeta ?
                int score = -quiesce(-cutBeta, -cutBeta + 1, stack + 1);

                if (score >= cutBeta) /// then we should try searching this capture
                    score = -search(-cutBeta, -cutBeta + 1, depth - probcutR, !cutNode, stack + 1);

                board.undoMove(move);


                if (score >= cutBeta) {
                    //TT->save(key, score, depth - probcutR, ply, LOWER, move, staticEval); // save probcut score in tt
                    return score;
                }
            }
        }
    }

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    /// also for cut nodes
    if (pvNode && !isCheck && depth >= 4 && !ttHit)
        depth--;

    if (cutNode && depth >= 4 && !ttHit)
        depth--;

    /// get counter move for move picker
    uint16_t counter = (nullSearch ? NULLMOVE : cmTable[1 ^ board.turn][(stack - 1)->piece][sqTo((stack - 1)->move)]);

    Movepick picker(ttMove, stack->killer, counter, -seeDepthCoef * depth);

    uint16_t move;

    while ((move = picker.nextMove(this, stack, board, skip, false)) != NULLMOVE) {

        if (move == excluded)
            continue;

        bool isQuiet = !isNoisyMove(board, move), refutationMove = (picker.trueStage < STAGE_QUIETS);
        int hist = 0, h, ch, fh;

        /// quiet move pruning
        if (best > -MATE && board.hasNonPawnMaterial(board.turn)) {
            if (isQuiet) {
                getHistory(this, stack, move, h, ch, fh);

                hist = h + fh + ch;

                /// approximately the new depth for the next search
                int newDepth = std::max(0, depth - lmrRed[std::min(63, depth)][std::min(63, played)]);

                /// continuation history leaf pruning
                if (newDepth <= 3 && !refutationMove && (ch < chCoef * newDepth || fh < fhCoef * newDepth))
                    continue;

                /// futility pruning
                if (newDepth <= 8 && !isCheck && staticEval + fpMargin + fpCoef * newDepth <= alpha)
                    skip = 1;

                /// late move pruning
                if (newDepth <= lmpDepth && nrQuiets >= lmrCnt[improving][newDepth])
                    skip = 1;

                if (depth <= 8 && !isCheck && !see(board, move, -seeCoefQuiet * depth))
                    continue;
            }
            else {
                if (depth <= 8 && !isCheck && picker.trueStage > STAGE_GOOD_NOISY && !see(board, move, -seeCoefNoisy * depth * depth))
                    continue;

                if (depth <= 8 && !isCheck && staticEval + fpMargin + seeVal[board.piece_type_at(sqTo(move))] + fpCoef * depth <= alpha)
                    continue;

                hist = getCapHist(this, move);
            }
        }

        int ex = 0;
        /// avoid extending too far (might cause stack overflow)
        if (ply < 2 * tDepth) {
            /// singular extension (look if the tt move is better than the rest)
            if (!excluded && move == ttMove && abs(ttValue) < MATE && depth >= 6 && entry.depth() >= depth - 3 && (bound & LOWER)) {
                int rBeta = ttValue - depth;

                int score = search(rBeta - 1, rBeta, depth / 2, cutNode, stack, move);

                if (score < rBeta)
                    ex = 1 + (!pvNode && rBeta - score > 100);
                else if (rBeta >= beta) /// multicut
                    return rBeta;
                else if (ttValue >= beta)
                    ex = -1;
            }
            else if (isCheck) {
                ex = 1;
            }
        }

        /// update stack info
        stack->move = move;
        stack->piece = board.piece_at(sqFrom(move));
        stack->continuationHist = &continuationHistory[stack->piece][sqTo(move)];

        board.makeMove(move);
        TT->prefetch(board.key);
        played++;

        /// store quiets for history

        if (isQuiet)
            stack->quiets[nrQuiets++] = move;
        else
            stack->captures[nrCaptures++] = move;

        int newDepth = depth + ex, R = 1;

        /// quiet late move reduction

        if (depth >= 3 && played > 1 + pvNode) { /// first few moves we don't reduce
            if (isQuiet) {
                R = lmrRed[std::min(63, depth)][std::min(63, played)];

                R += !pvNode + !improving; /// not on pv or not improving

                R += quietUs && !isCheck && eval - seeVal[KNIGHT] > beta; /// if the position is relatively quiet and eval is bigger than beta by a margin

                R += quietUs && !isCheck && staticEval - rootEval > 200 && ply % 2 == 0; /// the position in quiet and static eval is way bigger than root eval

                R -= 2 * refutationMove; /// reduce for refutation moves

                R -= board.checkers != 0; /// move gives check

                R -= hist / histDiv; /// reduce based on move history
            }
            else if (!pvNode) {
                R = lmrRedNoisy[std::min(63, depth)][std::min(63, played)];

                R += !improving; /// not improving

                R += quietUs && picker.trueStage == STAGE_BAD_NOISY; /// if the position is relatively quiet and the capture is "very losing"
            }

            R += cutNode;

            R = std::min(depth - 1, std::max(R, 1)); /// clamp R
        }

        int score = -INF;

        /// principal variation search
        uint64_t initNodes = nodes;

        if (R != 1) {
            score = -search(-alpha - 1, -alpha, newDepth - R, true, stack + 1);
        }

        if ((R != 1 && score > alpha) || (R == 1 && (!pvNode || played > 1))) {
            score = -search(-alpha - 1, -alpha, newDepth - 1, !cutNode, stack + 1);
        }

        if (pvNode && (played == 1 || score > alpha)) {
            score = -search(-beta, -alpha, newDepth - 1, false, stack + 1);
        }

        board.undoMove(move);

        nodesSearched[!isQuiet][sqFrom(move)][sqTo(move)] += nodes - initNodes;

        if (flag & TERMINATED_SEARCH) /// stop search
            return ABORT;

        if (score > best) {
            best = score;
            bestMove = move;

            if (score > alpha) {
                alpha = score;

                updatePv(ply, move);

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
        //cnt2 += flaggy;
        if (!isNoisyMove(board, bestMove)) {
            if (stack->killer != bestMove)
                stack->killer = bestMove;
            updateHistory(this, stack, nrQuiets, ply, getHistoryBonus(depth + pvNode));
        }
        updateCapHistory(this, stack, nrCaptures, bestMove, ply, getHistoryBonus(depth));
    }

    /// update tt only if we aren't in a singular search
    if (!excluded) {
        bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
        TT->save(key, best, depth, ply, bound, bestMove, staticEval);
    }

    return best;
}

int Search::rootSearch(int alpha, int beta, int depth, int multipv, StackEntry* stack) {
    if (checkForStop())
        return ABORT;

    if (depth <= 0)
        return quiesce(alpha, beta, stack);

    uint16_t ttMove = NULLMOVE;
    int alphaOrig = alpha;
    uint64_t key = board.key;
    uint16_t nrQuiets = 0;
    uint16_t nrCaptures = 0;
    int played = 0, bound = NONE;
    int best = -INF;
    uint16_t bestMove = NULLMOVE;
    int ttHit = 0, ttValue = 0;

    nodes++;

    TT->prefetch(key);
    pvTableLen[0] = 0;

    tt::Entry entry = {};

    /// transposition table probing
    int eval = INF;

    if (TT->probe(key, entry)) {
        int score = entry.value(0);
        ttHit = 1;
        ttValue = score;
        bound = entry.bound(), ttMove = entry.move;
        eval = entry.eval;
    }

    bool isCheck = (board.checkers != 0);

    if (isCheck) { /// when in check, don't evaluate (king safety evaluation might break)
        stack->eval = eval = INF;
    }
    else if (eval == INF) {
        stack->eval = eval = evaluate(board);
    }
    else { /// ttValue might be a better evaluation
        stack->eval = eval;
        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval))
            eval = ttValue;
    }

    (stack + 1)->killer = NULLMOVE;

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)
    if (!isCheck && depth >= 4 && !ttHit)
        depth--;

    Movepick picker(ttMove, stack->killer, NULLMOVE, -10 * depth);

    uint16_t move;

    while ((move = picker.nextMove(this, stack, board, false, false)) != NULLMOVE) {
        bool searched = false;

        for (int i = 1; i < multipv; i++) {
            if (move == bestMoves[i]) {
                searched = true;
                break;
            }
        }

        if (searched)
            continue;

        bool isQuiet = !isNoisyMove(board, move), refutationMove = (picker.stage < STAGE_QUIETS);

        /// update stack info
        stack->move = move;
        stack->piece = board.piece_at(sqFrom(move));
        stack->continuationHist = &continuationHistory[stack->piece][sqTo(move)];

        board.makeMove(move);
        played++;

        /// current root move info
        if (principalSearcher && printStats && getTime() > info->startTime + 2500 && !info->sanMode) {
            std::cout << "info depth " << depth << " currmove " << toString(move) << " currmovenumber " << played << std::endl;
        }

        /// store quiets for history
        if (isQuiet)
            stack->quiets[nrQuiets++] = move;
        else
            stack->captures[nrCaptures++] = move;

        int newDepth = depth, R = 1;

        /// quiet late move reduction
        if (depth >= 3 && played > 3) { /// first few moves we don't reduce
            if (isQuiet) {
                R = lmrRed[std::min(63, depth)][std::min(63, played)];

                R -= 2 * refutationMove; /// reduce for refutation moves
            }

            R = std::min(depth - 1, std::max(R, 1)); /// clamp R
        }

        int score = -INF;

        /// principal variation search
        uint64_t initNodes = nodes;

        if (R != 1) {
            score = -search(-alpha - 1, -alpha, newDepth - R, true, stack + 1);
        }

        if ((R != 1 && score > alpha) || (R == 1 && played > 1)) {
            score = -search(-alpha - 1, -alpha, newDepth - 1, true, stack + 1);
        }

        if (played == 1 || score > alpha) {
            score = -search(-beta, -alpha, newDepth - 1, false, stack + 1);
        }

        board.undoMove(move);

        nodesSearched[isNoisyMove(board, move)][sqFrom(move)][sqTo(move)] += nodes - initNodes;

        if (flag & TERMINATED_SEARCH) /// stop search
            return ABORT;

        if (score > best) {
            best = score;
            bestMove = move;

            if (score > alpha) {
                alpha = score;

                updatePv(0, move);

                if (alpha >= beta) {
                    break;
                }
            }
        }
    }

    TT->prefetch(key);

    if (!played) {
        return (isCheck ? -INF : 0);
    }

    /// update killer and history heuristics in case of cutoff
    if (best >= beta) {
        if (!isNoisyMove(board, bestMove)) {
            if (stack->killer != bestMove)
                stack->killer = bestMove;
            updateHistory(this, stack, nrQuiets, 0, getHistoryBonus(depth));
        }
        updateCapHistory(this, stack, nrCaptures, bestMove, 0, getHistoryBonus(depth));
    }

    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(key, best, depth, 0, bound, bestMove, stack->eval);

    return best;
}

std::pair <int, uint16_t> Search::startSearch(Info* _info) {
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

        int nrMoves = genLegal(board, moves);

        /// only 1 move legal

        if (PROBE_ROOT && nrMoves == 1) {
            if (printStats)
                std::cout << "bestmove " << toString(moves[0]) << std::endl;
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
                        std::cout << "bestmove " << toString(move) << std::endl;
                    return { 0, move };
                }
            }
        }
    }

    if (threadCount)
        startWorkerThreads(info);

    uint64_t totalNodes = 0, totalHits = 0;
    int limitDepth = (principalSearcher ? info->depth : 255); /// when limited by depth, allow helper threads to pass the fixed depth
    int mainThreadScore = 0;
    uint16_t mainThreadBestMove = NULLMOVE;

    memset(scores, 0, sizeof(scores));
    StackEntry search_stack[DEPTH + 15];
    StackEntry* stack = search_stack + 3;

    memset(search_stack, 0, sizeof(search_stack));

    rootEval = (!board.checkers ? evaluate(board) : INF);

    for (int i = 1; i <= 3; i++)
        (stack - i)->continuationHist = &continuationHistory[0][0], (stack - i)->eval = INF;

    for (tDepth = 1; tDepth <= limitDepth; tDepth++) {
        memset(bestMoves, 0, sizeof(bestMoves));

        for (int i = 1; i <= info->multipv; i++) {
            int window = 10;

            if (tDepth >= 6) {
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

                scores[i] = rootSearch(alpha, beta, depth, i, stack);

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
                            std::cout << "cp " << scores[i];
                        if (scores[i] >= beta)
                            std::cout << " lowerbound";
                        else if (scores[i] <= alpha)
                            std::cout << " upperbound";
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
                            int score = abs(scores[i]);
                            std::cout << (scores[i] >= 0 ? "+" : "-") <<
                                score / 100 << "." << (score % 100 >= 10 ? std::to_string(score % 100) : "0" + std::to_string(score % 100)) << " ";
                        }
                        std::cout << "  ";
                        printPv();
                        std::cout << std::endl;
                        //average_changes.print_mean();
                        //values[0].print_mean();
                        //std::cout << cnt << '\n';
                        //std::cout << cnt2 << " out of " << cnt << ", " << 100.0 * cnt2 / cnt << "% good\n";
                    }
                }

                if (scores[i] <= alpha) {
                    beta = (beta + alpha) / 2;
                    alpha = std::max(-INF, alpha - window);
                    depth = tDepth;
                }
                else if (beta <= scores[i]) {
                    beta = std::min(INF, beta + window);
                    depth--;

                    if (pvTableLen[0])
                        bestMoves[i] = pvTable[0][0];
                }
                else {
                    if (pvTableLen[0])
                        bestMoves[i] = pvTable[0][0];
                    break;
                }

                window += window / 2;
            }
        }

        if (principalSearcher && !(flag & TERMINATED_SEARCH)) {
            double scoreChange = 1.0, bestMoveStreak = 1.0, nodesSearchedPercentage = 1.0;

            float _tmNodesSearchedMaxPercentage = 1.0 * tmNodesSearchedMaxPercentage / 1000;
            float _tmBestMoveMax = 1.0 * tmBestMoveMax / 1000, _tmBestMoveStep = 1.0 * tmBestMoveStep / 1000;

            if (tDepth >= 9) {
                scoreChange = std::max(0.5, std::min(1.0 + 1.0 * (mainThreadScore - scores[1]) / tmScoreDiv, 1.5)); /// adjust time based on score change

                bestMoveCnt = (bestMoves[1] == mainThreadBestMove ? bestMoveCnt + 1 : 1);

                /// adjust time based on how many nodes from the total searched nodes were used for the best move
                nodesSearchedPercentage = 1.0 * nodesSearched[isNoisyMove(board, bestMoves[1])][sqFrom(bestMoves[1])][sqTo(bestMoves[1])] / nodes;
                nodesSearchedPercentage = _tmNodesSearchedMaxPercentage - nodesSearchedPercentage; 

                bestMoveStreak = _tmBestMoveMax - _tmBestMoveStep * std::min(10, bestMoveCnt); /// adjust time based on how long the best move was the same
            }

            info->stopTime = info->startTime + info->goodTimeLim * scoreChange * bestMoveStreak * nodesSearchedPercentage;

            mainThreadScore = scores[1];
            mainThreadBestMove = bestMoves[1];
        }

        if (flag & TERMINATED_SEARCH)
            break;

        if (info->timeset && getTime() > info->stopTime) {
            flag |= TERMINATED_BY_LIMITS;
            break;
        }

        if (tDepth == limitDepth) {
            flag |= TERMINATED_BY_LIMITS;
            break;
        }
    }

    if (threadCount)
        stopWorkerThreads();

    int bm = (bestMoves[1] ? bestMoves[1] : mainThreadBestMove);

    if (principalSearcher && printStats)
        std::cout << "bestmove " << toString(bm) << std::endl;

    //TT->age();

    return std::make_pair(scores[1], bm);
}

void Search::clearHistory() {
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

void Search::clearStack() {
    memset(pvTableLen, 0, sizeof(pvTableLen));
    memset(pvTable, 0, sizeof(pvTable));
    memset(nodesSearched, 0, sizeof(nodesSearched));

    for (int i = 0; i < threadCount; i++) {
        memset(params[i].pvTableLen, 0, sizeof(params[i].pvTableLen));
        memset(params[i].pvTable, 0, sizeof(params[i].pvTable));
        memset(params[i].nodesSearched, 0, sizeof(params[i].nodesSearched));
    }
}

void Search::startWorkerThreads(Info* info) {
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

void Search::flagWorkersStop() {
    for (int i = 0; i < threadCount; i++) {
        params[i].SMPThreadExit = true;
        params[i].flag |= TERMINATED_BY_LIMITS;
    }
}

void Search::stopWorkerThreads() {
    flagWorkersStop();

    for (int i = 0; i < threadCount; i++) {
        while (params[i].lazyFlag);
    }
}

void Search::isReady() {
    flagWorkersStop();
    flag |= TERMINATED_BY_USER;
    std::unique_lock <std::mutex> lk(readyMutex);
    std::cout << "readyok" << std::endl;
}

void Search::startPrincipalSearch(Info* info) {
    principalSearcher = true;

    readyMutex.lock();
    setTime(info);
    lazyFlag = 1;
    readyMutex.unlock();
    lazyCV.notify_one();

    if (!principalThread)
        principalThread.reset(new std::thread(&Search::lazySMPSearcher, this));
}

void Search::stopPrincipalSearch() {
    flag |= TERMINATED_BY_USER;
}

void Search::setThreadCount(int nrThreads) {
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

            startSearch(info);
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

void Search::_setFen(std::string fen) {
    for (int i = 0; i < threadCount; i++) {
        params[i].board.setFen(fen);
    }

    board.setFen(fen);
}

void Search::_makeMove(uint16_t move) {
    for (int i = 0; i < threadCount; i++)
        params[i].board.makeMove(move);

    board.makeMove(move);
}

void Search::clearBoard() {
    for (int i = 0; i < threadCount; i++)
        params[i].board.clear();

    board.clear();
}