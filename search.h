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
#include <cmath>

Search::Search() : threads(nullptr), params(nullptr)
{
    nodes = tbHits = t0 = tDepth = selDepth = lazyDepth = threadCount = flag = checkCount = 0;
    principalSearcher = terminateSMP = SMPThreadExit = false;
    memset(&lmrRed, 0, sizeof(lmrRed));
    memset(&lmrCnt, 0, sizeof(lmrCnt));
    memset(&info, 0, sizeof(info));

    for (int i = 0; i < 64; i++) { /// depth
        for (int j = 0; j < 64; j++) /// moves played
            lmrRed[i][j] = 0.75 + log(i) * log(j) / 2.25;
    }
    for (int i = 1; i < 9; i++) {
        lmrCnt[0][i] = (3 + i * i) / 2;
        lmrCnt[1][i] = 3 + i * i;
    }
}

Search::~Search() {
    releaseThreads();
}

bool Search::checkForStop() {
    if (flag & TERMINATED_SEARCH)
        return 1;

    if (SMPThreadExit) {
        flag |= TERMINATED_BY_TIME;
        return 1;
    }

    checkCount++;
    checkCount &= 1023;

    if (!checkCount) {
        if (info->timeset && getTime() > info->startTime + info->hardTimeLim)
            flag |= TERMINATED_BY_TIME;
    }

    return (flag & TERMINATED_SEARCH);
}

bool printStats = true; /// default true
bool PROBE_ROOT = true; /// default true

int Search::quiesce(int alpha, int beta, bool useTT) {
    int ply = board.ply;

    pvTableLen[ply] = 0;

    selDepth = std::max(selDepth, ply);
    nodes++;

    if (isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw()) /// check for draw
        return 1 - (nodes & 2);

    if (checkForStop())
        return ABORT;

    /// check if in transposition table
    uint64_t key = board.key;
    int score = 0, best = -INF, alphaOrig = alpha;
    int bound = NONE;
    uint16_t bestMove = NULLMOVE;

    TT->prefetch(key);

    tt::Entry entry = {};

    int eval = INF, ttValue = 0;

    /// probe transposition table

    if (TT->probe(key, entry)) {
        best = eval = entry.info.eval;
        ttValue = score = entry.value(ply);
        bound = entry.bound();

        if (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))
            return score;
    }

    if (eval == INF) {
        /// if last move was null, we already know the evaluation
        Stack[ply].eval = best = eval = (!Stack[ply - 1].move ? -Stack[ply - 1].eval + 2 * TEMPO : evaluate(board));
    }
    else {
        /// ttValue might be a better evaluation

        Stack[ply].eval = eval;

        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval))
            best = ttValue;
    }

    /// stand-pat

    if (best >= beta)
        return best;

    alpha = std::max(alpha, best);

    Movepick noisyPicker(NULLMOVE, NULLMOVE, NULLMOVE, NULLMOVE, 0);

    uint16_t move;

    while ((move = noisyPicker.nextMove(this, 1, 1))) {
        /// update stack info

        Stack[ply].move = move;
        Stack[ply].piece = board.piece_at(sqFrom(move));

        makeMove(board, move);
        score = -quiesce(-beta, -alpha);
        undoMove(board, move);

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

    /// store info in transposition table (seems to work)

    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(key, best, 0, ply, bound, bestMove, Stack[ply].eval);

    return best;
}

int Search::search(int alpha, int beta, int depth, bool cutNode, uint16_t excluded) {

    if (checkForStop())
        return ABORT;

    if (depth <= 0)
        return quiesce(alpha, beta);

    int pvNode = (alpha < beta - 1), rootNode = (board.ply == 0);
    uint16_t ttMove = NULLMOVE;
    int ply = board.ply;
    int alphaOrig = alpha;
    uint64_t key = board.key;
    uint16_t quiets[256], nrQuiets = 0;
    //uint16_t captures[256], nrCaptures = 0;
    int played = 0, bound = NONE, skip = 0;
    int best = -INF;
    uint16_t bestMove = NULLMOVE;
    int ttHit = 0, ttValue = 0;

    nodes++;
    selDepth = std::max(selDepth, ply);

    TT->prefetch(key);
    pvTableLen[ply] = 0;

    if (!rootNode) {
        if (isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw())
            return 1 - (nodes & 2); /// beal effect apparently, credit to Ethereal for this

          /// mate distance pruning
        alpha = std::max(alpha, -INF + ply), beta = std::min(beta, INF - ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    tt::Entry entry = {};

    /// transposition table probing

    int eval = INF;

    if (!excluded && TT->probe(key, entry)) {
        int score = entry.value(ply);
        ttHit = 1;
        ttValue = score;
        bound = entry.bound(), ttMove = entry.info.move;
        eval = entry.info.eval;
        if (entry.depth() >= depth && !pvNode) {
            if (bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))
                return score;
        }
    }

    /// tablebase probing

    if (!rootNode && TB_LARGEST && depth >= 2 && !board.halfMoves && !board.castleRights) {
        unsigned pieces = count(board.pieces[WHITE] | board.pieces[BLACK]);

        if (pieces <= TB_LARGEST) {
            int ep = board.enPas, score = 0, type = NONE;
            if (ep == -1)
                ep = 0;

            auto probe = tb_probe_wdl(board.pieces[WHITE], board.pieces[BLACK],
                board.bb[WK] | board.bb[BK],
                board.bb[WQ] | board.bb[BQ],
                board.bb[WR] | board.bb[BR],
                board.bb[WB] | board.bb[BB],
                board.bb[WN] | board.bb[BN],
                board.bb[WP] | board.bb[BP],
                0, 0, ep, board.turn);

            if (probe != TB_RESULT_FAILED) {
                tbHits++;
                switch (probe) {
                case TB_WIN:
                    score = TB_WIN_SCORE - DEPTH - board.ply;
                    type = LOWER;
                    break;
                case TB_LOSS:
                    score = -(TB_WIN_SCORE - DEPTH - board.ply);
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
        }
    }

    bool isCheck = inCheck(board);

    if (isCheck) {
        /// when in check, don't evaluate (king safety evaluation might break)
        Stack[ply].eval = eval = INF;
    }
    else if (eval == INF) {
        /// if last move was null, we already know the evaluation
        if (excluded)
            eval = Stack[ply].eval;
        else
            Stack[ply].eval = eval = (ply >= 1 && !Stack[ply - 1].move ? -Stack[ply - 1].eval + 2 * TEMPO : evaluate(board));
    }
    else {
        /// ttValue might be a better evaluation

        Stack[ply].eval = eval;

        if (bound == EXACT || (bound == LOWER && ttValue > eval) || (bound == UPPER && ttValue < eval))
            eval = ttValue;
    }

    bool improving = (!isCheck && ply >= 2 && Stack[ply].eval > Stack[ply - 2].eval); /// (TO DO: make all pruning dependent of this variable?)

    killers[ply + 1][0] = killers[ply + 1][1] = NULLMOVE;

    /// razoring (searching 1 more ply can't change the score much, drop in quiesce)

    if (!pvNode && !isCheck && depth <= 1 && Stack[ply].eval + 325 < alpha) {
        return quiesce(alpha, beta);
    }

    /// static null move pruning (don't prune when having a mate line, again stability)

    if (!pvNode && !isCheck && depth <= 8 && eval - (SNMPCoef1 - SNMPCoef2 * improving) * depth > beta && eval < MATE)
        return eval;

    /// null move pruning (when last move wasn't null, we still have non pawn material,
    ///                    we have a good position and we don't have any idea if it's likely to fail)
    /// TO DO: tune nmp

    if (!pvNode && !isCheck && !excluded && eval >= beta && eval >= Stack[ply].eval && depth >= 2 && Stack[ply - 1].move &&
        (board.pieces[board.turn] ^ board.bb[getType(PAWN, board.turn)] ^ board.bb[getType(KING, board.turn)]) &&
        (!ttHit || !(bound & UPPER) || ttValue >= beta)) {
        int R = 4 + depth / 6 + std::min(3, (eval - beta) / 100);

        Stack[ply].move = NULLMOVE;
        Stack[ply].piece = 0;

        makeNullMove(board);

        int score = -search(-beta, -beta + 1, depth - R, !cutNode);

        /// TO DO: (verification search?)

        undoNullMove(board);

        //nmpTries++;

        if (score >= beta) /// don't trust mate scores
            return (abs(score) > MATE ? beta : score);
        else {
            //nmpFail++;
        }
    }

    /// probcut

    if (!pvNode && !isCheck && depth >= 5 && abs(beta) < MATE) {
        int cutBeta = beta + 100;
        Movepick noisyPicker(NULLMOVE, NULLMOVE, NULLMOVE, NULLMOVE, cutBeta - Stack[ply].eval);

        uint16_t move;

        while ((move = noisyPicker.nextMove(this, 1, 1)) != NULLMOVE) {
            if (move == excluded)
                continue;

            Stack[ply].move = move;
            Stack[ply].piece = board.piece_at(sqFrom(move));

            makeMove(board, move);

            /// do we have a good sequence of captures that beats cutBeta ?

            int score = -quiesce(-cutBeta, -cutBeta + 1);

            if (score >= cutBeta) /// then we should try searching this capture
                score = -search(-cutBeta, -cutBeta + 1, depth - 4, !cutNode);

            undoMove(board, move);

            if (score >= cutBeta)
                return score;
        }
    }

    /// internal iterative deepening (search at reduced depth to find a ttMove) (Rebel like)

    if (pvNode && !isCheck && depth >= 4 && !ttMove)
        depth--;

    /// get counter move for move picker

    uint16_t counter = (ply == 0 || !Stack[ply - 1].move ? NULLMOVE : cmTable[1 ^ board.turn][Stack[ply - 1].piece][sqTo(Stack[ply - 1].move)]);

    Movepick picker(ttMove, killers[ply][0], killers[ply][1], counter, 0);

    uint16_t move;
    int recapSq = (!rootNode && Stack[ply - 1].move ? sqTo(Stack[ply - 1].move) : - 1);

    while ((move = picker.nextMove(this, skip, 0)) != NULLMOVE) {

        if (move == excluded)
            continue;

        bool isQuiet = !isNoisyMove(board, move), refutationMove = (picker.stage < STAGE_QUIETS);
        Heuristics H{}; /// history values for quiet moves

        //int capH = capHist[board.piece_type_at(sqFrom(move))][sqTo(move)][board.piece_type_at(sqTo(move))];

        /// quiet move pruning
        if (!rootNode && best > -MATE) {
            if (isQuiet) {
                getHistory(this, move, ply, H);

                /// approximately the new depth for the next search
                int newDepth = std::max(0, depth - lmrRed[std::min(63, depth)][std::min(63, played)]);

                /// counter move and follow move pruning
                if (newDepth <= 3 && !refutationMove && (H.ch < chCoef * newDepth || H.fh < fhCoef * newDepth))
                    continue;

                /// futility pruning
                if (newDepth <= 8 && !isCheck && Stack[ply].eval + fpMargin + fpCoef * newDepth + (H.h + H.ch + H.fh) / fpHistDiv <= alpha)
                    skip = 1;

                /// late move pruning
                if (newDepth <= 8 && nrQuiets >= lmrCnt[improving][newDepth])
                    skip = 1;

                if (depth <= 8 && !isCheck && !see(board, move, -seeCoefQuiet * depth))
                    continue;
            }

            /// see pruning

            if (depth <= 8 && !isCheck && !isQuiet && picker.stage > STAGE_GOOD_NOISY && !see(board, move, -seeCoefNoisy * depth * depth))
                continue;
        }

        bool ex = 0;

        /// singular extension (look if the tt move is better than the rest)


        if (!rootNode && !excluded && move == ttMove && abs(ttValue) < MATE && depth >= 8 && entry.depth() >= depth - 3 && (bound & LOWER)) { /// had best instead of ttValue lol
            int rBeta = ttValue - depth;

            int score = search(rBeta - 1, rBeta, depth / 2, cutNode, move);

            if (score < rBeta)
                ex = 1;
            else if (rBeta >= beta) /// multicut
                return rBeta;
        }
        else if (isCheck) {
            ex = 1;
        }
        else if (sqTo(move) == recapSq) {
            ex = 1;
        }
        /*
        else if (isQuiet && pvNode && H.ch >= 10000 && H.fh >= 10000) { /// in check extension and moves with good history
            ex = 1;
        }
        */

        /// update stack info
        Stack[ply].move = move;
        Stack[ply].piece = board.piece_at(sqFrom(move));

        makeMove(board, move);
        played++;

        /// current root move info

        if (rootNode && principalSearcher && printStats && getTime() > info->startTime + 2500) {
            std::cout << "info depth " << depth << " currmove " << toString(move) << " currmovenumber " << played << std::endl;
        }

        /// store quiets for history

        if (isQuiet)
            quiets[nrQuiets++] = move;
        /*else
          captures[nrCaptures++] = move;*/

        int newDepth = depth + (ex && !rootNode), R = 1;

        /// quiet late move reduction

        if (isQuiet && depth >= 3 && played > 1 + 2 * rootNode) { /// first few moves we don't reduce
            R = lmrRed[std::min(63, depth)][std::min(63, played)];

            R += !pvNode + !improving; /// not on pv or not improving

            R += cutNode; // reduce more for cut nodes

            R -= 2 * refutationMove; /// reduce for refutation moves

            R -= std::max(-2, std::min(2, (H.h + H.ch + H.fh) / histDiv)); /// reduce based on move history

            R = std::min(depth - 1, std::max(R, 1)); /// clamp R
        }

        int score = -INF;

        /// principal variation search

        uint64_t initNodes = nodes;

        if (R != 1) {
            score = -search(-alpha - 1, -alpha, newDepth - R, true);
        }

        if ((R != 1 && score > alpha) || (R == 1 && !(pvNode && played == 1))) {
            score = -search(-alpha - 1, -alpha, newDepth - 1, !cutNode);
        }

        if (pvNode && (played == 1 || score > alpha)) {
            score = -search(-beta, -alpha, newDepth - 1, false);
        }

        nodesSearched[sqFrom(move)][sqTo(move)] += nodes - initNodes;

        undoMove(board, move);

        if (flag & TERMINATED_SEARCH) /// stop search
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

    TT->prefetch(key);

    if (!played) {
        return (isCheck ? -INF + ply : 0);
    }

    /// update killers and history heuristics

    if (best >= beta && !isNoisyMove(board, bestMove)) {
        if (killers[ply][0] != bestMove) {
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = bestMove;
        }

        int updateDepth = depth;
        updateHistory(this, quiets, nrQuiets, ply, updateDepth * updateDepth);
    }

    /*if (best >= beta)
        updateCapHistory(this, captures, nrCaptures, bestMove, ply, depth * depth);*/

      /// update tt only if we aren't in a singular search

    if (!excluded) {
        bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
        TT->save(key, best, depth, ply, bound, bestMove, Stack[ply].eval);
    }

    return best;
}

std::pair <int, uint16_t> Search::startSearch(Info* _info) {
    int alpha, beta, score = 0;
    int bestMove = NULLMOVE;

    nodes = selDepth = tbHits = 0;
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

        if (PROBE_ROOT && printStats && nrMoves == 1) {
            waitUntilDone();
            std::cout << "bestmove " << toString(moves[0]) << std::endl;
            return { 0, moves[0] };
        }

        /// position is in tablebase

        if (PROBE_ROOT && count(board.pieces[WHITE] | board.pieces[BLACK]) <= (int)TB_LARGEST) {
            int move = NULLMOVE;
            auto probe = tb_probe_root(board.pieces[WHITE], board.pieces[BLACK],
                board.bb[WK] | board.bb[BK],
                board.bb[WQ] | board.bb[BQ],
                board.bb[WR] | board.bb[BR],
                board.bb[WB] | board.bb[BB],
                board.bb[WN] | board.bb[BN],
                board.bb[WP] | board.bb[BP],
                board.halfMoves,
                (board.castleRights & (3 << board.turn)) > 0,
                0,
                board.turn,
                nullptr);
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

            for (auto& mv : moves) {
                if (mv == move && printStats) {
                    waitUntilDone();
                    std::cout << "bestmove " << toString(move) << std::endl;
                    return { 0, move };
                }
            }
        }
    }

    if (threadCount)
        startWorkerThreads(info);

    uint64_t totalNodes = 0, totalHits = 0;
    int lastScore = 0, lastBestMove = NULLMOVE;
    int limitDepth = (principalSearcher ? info->depth : DEPTH); /// when limited by depth, allow helper threads to pass the fixed depth

    //contempt = 0;

    for (tDepth = 1; tDepth <= limitDepth; tDepth++) {

        int window = 10;

        if (tDepth >= 6) {
            alpha = std::max(-INF, lastScore - window);
            beta = std::min(INF, lastScore + window);
        }
        else {
            alpha = -INF;
            beta = INF;
        }

        //contempt = (board.turn == WHITE ? lastScore / 20 : -lastScore / 20);
        //std::cout << "CONTEMPT = " << contempt << "\n";

        int depth = tDepth;
        while (true) {

            depth = std::max(depth, 1);

            selDepth = 0;

            score = search(alpha, beta, depth, false);

            if (flag & TERMINATED_SEARCH)
                break;

            if (principalSearcher && printStats && ((alpha < score && score < beta) || getTime() > t0 + 3000)) {
                if (principalSearcher) {
                    totalNodes = nodes;
                    totalHits = tbHits;
                    for (int i = 0; i < threadCount; i++) {
                        totalNodes += params[i].nodes;
                        totalHits += params[i].tbHits;
                    }
                }
                uint64_t t = (uint64_t)getTime() - t0;
                std::cout << "info score ";
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
                std::cout << " depth " << depth << " seldepth " << selDepth << " nodes " << totalNodes;
                if (t)
                    std::cout << " nps " << totalNodes * 1000 / t;
                std::cout << " time " << t << " ";
                std::cout << "tbhits " << totalHits << " hashfull " << TT->tableFull() << " ";
                std::cout << "pv ";
                printPv();
                std::cout << std::endl;

                //std::cout << "NMP Fail rate: " << 100.0 * nmpFail / nmpTries << "\n";
            }

            if (score <= alpha) {
                beta = (beta + alpha) / 2;
                alpha = std::max(-INF, alpha - window);
                depth = tDepth;
            }
            else if (beta <= score) {
                beta = std::min(INF, beta + window);
                /// reduce depth if failing high
                /// don't reduce when finding tb wins / mate scores
                depth -= (abs(score) < TB_WIN_SCORE / 2);

                if (pvTableLen[0])
                    bestMove = pvTable[0][0];
            }
            else {
                if (pvTableLen[0])
                    bestMove = pvTable[0][0];
                break;
            }

            window += window / 2;
        }

        if (principalSearcher) {
            /// adjust time if score is dropping (and maybe if it's jumping)
            double scoreChange = 1.0, bestMoveStreak = 1.0;
            if (tDepth >= 9) {
                scoreChange = std::max(0.5, std::min(1.0 + (lastScore - score) / 100, 1.5));

                bestMoveCnt = (bestMove == lastBestMove ? bestMoveCnt + 1 : 1);

                bestMoveStreak = 1.5 - 0.1 * std::min(10, bestMoveCnt);
            }

            info->stopTime = info->startTime + info->goodTimeLim * scoreChange * bestMoveStreak;
        }

        lastScore = score;
        lastBestMove = bestMove;

        if (flag & TERMINATED_SEARCH)
            break;

        if (info->timeset && getTime() > info->stopTime) {
            flag |= TERMINATED_BY_TIME;
            break;
        }

        if (tDepth == limitDepth) {
            flag |= TERMINATED_BY_TIME;
            break;
        }
    }

    if (threadCount)
        stopWorkerThreads();

    waitUntilDone();

    if (principalSearcher && printStats)
        std::cout << "bestmove " << toString(bestMove) << std::endl;

    //TT->age();

    return std::make_pair(score, bestMove);
}

void Search::clearHistory() {
    memset(hist, 0, sizeof(hist));
    memset(capHist, 0, sizeof(capHist));
    memset(cmTable, 0, sizeof(cmTable));
    memset(follow, 0, sizeof(follow));
    memset(nodesSearched, 0, sizeof(nodesSearched));

    for (int i = 0; i < threadCount; i++) {
        memset(params[i].hist, 0, sizeof(params[i].hist));
        memset(params[i].capHist, 0, sizeof(params[i].capHist));
        memset(params[i].cmTable, 0, sizeof(params[i].cmTable));
        memset(params[i].follow, 0, sizeof(params[i].follow));
        memset(params[i].nodesSearched, 0, sizeof(params[i].nodesSearched));
    }
}

void Search::clearKillers() {
    memset(killers, 0, sizeof(killers));

    for (int i = 0; i < threadCount; i++)
        memset(params[i].killers, 0, sizeof(params[i].killers));
}

void Search::clearStack() {
    memset(Stack, 0, sizeof(Stack));
    memset(pvTableLen, 0, sizeof(pvTableLen));
    memset(pvTable, 0, sizeof(pvTable));

    for (int i = 0; i < threadCount; i++) {
        memset(params[i].Stack, 0, sizeof(params[i].Stack));
        memset(params[i].pvTableLen, 0, sizeof(params[i].pvTableLen));
        memset(params[i].pvTable, 0, sizeof(params[i].pvTable));
    }
}

void Search::printPv() {
    for (int i = 0; i < pvTableLen[0]; i++) {
        std::cout << toString(pvTable[0][i]) << " ";
    }
}

void Search::updatePv(int ply, int move) {
    pvTable[ply][0] = move;
    for (int i = 0; i < pvTableLen[ply + 1]; i++)
        pvTable[ply][i + 1] = pvTable[ply + 1][i];
    pvTableLen[ply] = 1 + pvTableLen[ply + 1];
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

        params[i].lazyDepth = 1;
        params[i].flag = flag;

        params[i].readyMutex.unlock();

        params[i].lazyCV.notify_one();
    }
}

void Search::flagWorkersStop() {
    for (int i = 0; i < threadCount; i++) {
        params[i].SMPThreadExit = true;
        params[i].flag |= TERMINATED_BY_TIME;
    }
}

void Search::stopWorkerThreads() {
    flagWorkersStop();

    for (int i = 0; i < threadCount; i++) {
        while (params[i].lazyDepth);
    }
}

void Search::waitUntilDone() {
    if (!principalSearcher)
        return;
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
    lazyDepth = 1;
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

int Search::getThreadCount() { /// for debugging (i got a crash related to threadCount)
    return threadCount + 1;
}

void Search::lazySMPSearcher() {
    while (!terminateSMP) {

        {
            std::unique_lock <std::mutex> lk(readyMutex);

            lazyCV.wait(lk, std::bind(&Search::isLazySMP, this));

            if (terminateSMP)
                return;

            tDepth = lazyDepth;

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
                params[i].lazyDepth = 1;
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
        makeMove(params[i].board, move);

    makeMove(board, move);
}

void Search::clearBoard() {
    for (int i = 0; i < threadCount; i++)
        params[i].board.clear();

    board.clear();
}