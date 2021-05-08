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
  memset(&info  , 0, sizeof(info));

  for(int i = 0; i < 64; i++) { /// depth
    for(int j = 0; j < 64; j++) /// moves played
      lmrRed[i][j] = 0.75 + log(i) * log(j) / 2.25;
  }
  for(int i = 1; i < 9; i++) {
    lmrCnt[0][i] = 2.5 + 2 * i * i / 4.5;
    lmrCnt[1][i] = 4.0 + 4 * i * i / 4.5;
  }
}

Search::~Search() {
  releaseThreads();
}

bool Search :: checkForStop() {
  if(flag & TERMINATED_SEARCH)
    return 1;

  if(SMPThreadExit) {
    flag |= TERMINATED_BY_TIME;
    return 1;
  }

  checkCount++;
  checkCount &= 1023;

  if(!checkCount) {
    if(info->timeset && getTime() > info->stopTime)
      flag |= TERMINATED_BY_TIME;
  }

  return (flag & TERMINATED_SEARCH);
}

bool printStats = true; /// default true
bool PROBE_ROOT = true; /// default true

int Search :: quiesce(int alpha, int beta) {
  int ply = board.ply;

  //std::cout << "quiesce " << alpha << " " << beta << "\n";
  //board.print();

  pvTableLen[ply] = 0;

  selDepth = std::max(selDepth, ply);
  nodes++;

  if(isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw()) /// check for draw
    return 0;

  if(checkForStop())
    return ABORT;

  /// check if in transposition table

  uint64_t key = board.key;
  int score = 0, best = -INF;
  int bound = NONE;

  tt :: Entry entry = {};

  int eval = INF;

  if(TT->probe(key, entry)) {
    eval = entry.info.eval;
    score = entry.value(ply);
    bound = entry.bound();
    if(bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))
      return score;
  }

  if(eval == INF)
    eval = (Stack[ply - 1].move == NULLMOVE ? -Stack[ply - 1].eval + 2 * TEMPO : evaluate(board));

  Stack[ply].eval = eval;

  //std::cout << "eval = " << eval << "\n";

  //uint16_t mv = getMove(C5, D4, 0, NEUT);

  //std::cout << see(board, mv, 0) << " " << see(board, mv, 1) << "\n";

  /// stand-pat

  if(eval >= beta)
    return eval;

  alpha = std::max(alpha, eval);
  best = eval;

  Movepick noisyPicker(NULLMOVE, NULLMOVE, NULLMOVE, NULLMOVE, 0); /// delta pruning -> TO DO: find better constant (edit: removed)

  uint16_t move;

  while((move = noisyPicker.nextMove(this, 1, 1))) {

    //cout << "in quiesce, ply = " << ply << ", move = " << toString(move) << "\n";

    Stack[ply].move = move;
    Stack[ply].piece = board.piece_at(sqFrom(move));

    makeMove(board, move);
    score = -quiesce(-beta, -alpha);
    undoMove(board, move);

    if(score > best) {
      best = score;

      if(score > alpha) {
        alpha = score;
        updatePv(ply, move);

        if(alpha >= beta)
          break;
      }
    }
  }

  return best;
}

int Search :: search(int alpha, int beta, int depth, uint16_t excluded) {
  int pvNode = (alpha < beta - 1), rootNode = (board.ply == 0);
  uint16_t hashMove = NULLMOVE;
  int ply = board.ply;
  int alphaOrig = alpha;
  uint64_t key = board.key;
  uint16_t quiets[256], nrQuiets = 0;

  //cout << alpha << " " << beta << " " << depth << "\n";

  if(checkForStop())
    return ABORT;

  int played = 0, bound = NONE, skip = 0;
  int best = -INF;
  uint16_t bestMove = NULLMOVE;
  int ttHit = 0, ttValue = 0;

  if(depth <= 0)
    return quiesce(alpha, beta);

  nodes++;
  selDepth = std::max(selDepth, ply);

  TT->prefetch(key);
  pvTableLen[ply] = 0;

  if(!rootNode) {
    if(isRepetition(board, ply) || board.halfMoves >= 100 || board.isMaterialDraw())
      return 0;
    int rAlpha = std::max(alpha, -INF + ply), rBeta = std::min(beta, INF - ply - 1);
    if(rAlpha >= rBeta)
      return rAlpha;
  }

  tt :: Entry entry = {};

  /// transposition table probing

  int eval = INF;

  if(excluded == NULLMOVE && TT->probe(key, entry)) {
    int score = entry.value(ply);
    ttHit = 1;
    ttValue = score;
    bound = entry.bound(), hashMove = entry.info.move;
    eval = entry.info.eval;
    if(entry.depth() >= depth && (depth == 0 || !pvNode)) {
      if(bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))
        return score;
    }
  }

  /// tablebase probing

  if(!rootNode && TB_LARGEST && depth >= 2 && !board.halfMoves && !board.castleRights) {
    unsigned pieces = count(board.pieces[WHITE] | board.pieces[BLACK]);

    if(pieces <= TB_LARGEST) {
      int ep = board.enPas, score = 0, type = NONE;
      if(ep == -1)
        ep = 0;

      auto probe = tb_probe_wdl(board.pieces[WHITE], board.pieces[BLACK],
                                board.bb[WK] | board.bb[BK],
                                board.bb[WQ] | board.bb[BQ],
                                board.bb[WR] | board.bb[BR],
                                board.bb[WB] | board.bb[BB],
                                board.bb[WN] | board.bb[BN],
                                board.bb[WP] | board.bb[BP],
                                0, 0, ep, board.turn);

      if(probe != TB_RESULT_FAILED) {
        tbHits++;
        switch(probe) {
          case TB_WIN:
            score = TB_BASE_SCORE - DEPTH - board.ply;
            type = LOWER;
            break;
          case TB_LOSS:
            score = -(TB_BASE_SCORE - DEPTH - board.ply);
            type = UPPER;
            break;
          default:
            score = 0;
            type = EXACT;
            break;
        }

        if(type == EXACT || (type == UPPER && score <= alpha) || (type == LOWER && score >= beta)) {
          //TT->save()
          //board.print();
          TT->save(key, score, DEPTH, 0, type, NULLMOVE, 0);
          return score;
        }
      }
    }
  }

  /// no need to evaluate if last move was null
  if(eval == INF)
    eval = (ply >= 1 && Stack[ply - 1].move == NULLMOVE ? -Stack[ply - 1].eval + 2 * TEMPO : evaluate(board));

  bool isCheck = inCheck(board), improving = (ply >= 2 && eval > Stack[ply - 2].eval);

  killers[ply + 1][0] = killers[ply + 1][1] = NULLMOVE;
  Stack[ply].eval = eval;

  /// razoring

  if(!pvNode && !isCheck && depth <= 1 && eval + RazorCoef < alpha)
    return quiesce(alpha, beta);

  /// static null move pruning

  if(!pvNode && !isCheck && depth <= 8 && eval - StaticNullCoef * depth > beta)
    return eval;

  /// null move pruning

  if(!pvNode && !isCheck && eval >= beta && depth >= 2 &&
     Stack[ply - 1].move &&
     (board.pieces[board.turn] ^ board.bb[getType(PAWN, board.turn)] ^ board.bb[getType(KING, board.turn)]) &&
     (!ttHit || !(bound & UPPER) || ttValue >= beta)) {
    int R = 4 + depth / 6 + std::min(3, (eval - beta) / 200);

    Stack[ply].move = NULLMOVE;
    Stack[ply].piece = 0;

    makeNullMove(board);

    int score = -search(-beta, -beta + 1, depth - R);

    undoNullMove(board);

    if(score >= beta)
      return (abs(score) > MATE ? beta : score);
  }

  /// probcut

  if(!pvNode && !isCheck && depth >= 5 && abs(beta) < MATE) {
    int cutBeta = beta + 100;
    Movepick noisyPicker(NULLMOVE, NULLMOVE, NULLMOVE, NULLMOVE, cutBeta - eval);

    uint16_t move;

    while((move = noisyPicker.nextMove(this, 1, 1)) != NULLMOVE) {
      if(move == excluded)
        continue;

      Stack[ply].move = move;
      Stack[ply].piece = board.piece_at(sqFrom(move));

      makeMove(board, move);


      int score = -quiesce(-cutBeta, -cutBeta + 1);

      if(score >= cutBeta)
        score = -search(-cutBeta, -cutBeta + 1, depth - 4);

      undoMove(board, move);

      if(score >= cutBeta)
        return score;
    }
  }

  /*/// IID

  if(depth >= 4 && !hashMove)
    depth--;*/

  /// get counter move for move picker

  uint16_t counter = (ply == 0 || Stack[ply - 1].move == NULLMOVE ? NULLMOVE :
                                                                    cmTable[1 ^ board.turn][Stack[ply - 1].piece][sqTo(Stack[ply - 1].move)]);

  Movepick picker(hashMove, killers[ply][0], killers[ply][1], counter, 0);

  uint16_t move;

  while((move = picker.nextMove(this, skip, 0)) != NULLMOVE) {

    if(move == excluded)
      continue;

    bool isQuiet = !isNoisyMove(board, move);
    History :: Heuristics H{}; /// history values for quiet moves

    /// quiet move pruning
    if(!rootNode && best > -MATE) {
      if(isQuiet) {
        History :: getHistory(this, move, ply, H);

        //cout << h << " " << ch << " " << fh << "\n";

        /// cm and fm pruning

        if(depth <= cmpDepth[improving] && H.ch < cmpHistoryLimit[improving])
          continue;

        if(depth <= fmpDepth[improving] && H.fh < fmpHistoryLimit[improving])
          continue;

        /// futility pruning
        if(depth <= 8 && eval + 90 * depth <= alpha && H.h + H.ch + H.fh < fpHistoryLimit[improving])
          skip = 1;

        /// late move pruning
        if(depth <= 8 && nrQuiets >= lmrCnt[improving][depth])
          skip = 1;
      }

      /// see pruning

      if(depth <= 8 && !isCheck) {
        int seeMargin[2];

        seeMargin[1] = -80 * depth;
        seeMargin[0] = -18 * depth * depth;

        if(!see(board, move, seeMargin[isQuiet]))
          continue;
      }
    }

    bool ex = 0;

    /// singular extension (we look for other moves, for move diversity)

    if(!rootNode && !excluded && move == hashMove && abs(ttValue) < MATE && depth >= 8 && entry.depth() >= depth - 3 && (bound & LOWER)) { /// had best instead of ttValue lol
      int rBeta = ttValue - depth;
      //cout << "Entering singular extension with ";
      //cout << "depth = " << depth << ", alpha = " << alpha << ", beta = " << beta << "\n";
      //board.print();
      int score = search(rBeta - 1, rBeta, depth / 2, move);

      if(score < rBeta)
        ex = 1;
      else if(rBeta >= beta)
        return rBeta;
    } else {
      ex = isCheck | (isQuiet && pvNode && H.ch >= 10000 && H.fh >= 10000); /// in check extension and moves with good history
    }

    Stack[ply].move = move;
    Stack[ply].piece = board.piece_at(sqFrom(move));

    makeMove(board, move);
    played++;

    /// current root move info

    if(rootNode && principalSearcher && getTime() > info->startTime + 2500) {
      std::cout << "info depth " << depth << " currmove " << toString(move) << " currmovenumber " << played << std::endl;
      /*if(move == hashMove)
        std::cout << "hashMove\n";
      else if(move == killers[ply][0])
        std::cout << "killer1\n";
      else if(move == killers[ply][1])
        std::cout << "killer2\n";
      else if(move == counter)
        std::cout << "counter\n";
      else
        std::cout << "normal\n";*/
    }

    /// store quiets for history

    if(isQuiet)
      quiets[nrQuiets++] = move;

    int newDepth = depth + (ex && !rootNode), R = 1;

    /// quiet late move reduction

    if(isQuiet && depth >= 3 && played > 1 + 2 * rootNode) { /// first few moves we don't reduce
      R = lmrRed[std::min(63, depth)][std::min(63, played)];

      R += !pvNode + !improving; /// not on pv or not improving

      R += isCheck && piece_type(board.board[sqTo(move)]) == KING; /// check evasions

      R -= picker.stage < STAGE_QUIETS; /// refutation moves

      R -= std::max(-2, std::min(2, (H.h + H.ch + H.fh) / 5000)); /// reduce based on move history

      R = std::min(depth - 1, std::max(R, 1));
    }

    int score = -INF;

    /// principal variation search

    if(R != 1) {
      score = -search(-alpha - 1, -alpha, newDepth - R);
    }

    if((R != 1 && score > alpha) || (R == 1 && !(pvNode && played == 1))) {
      score = -search(-alpha - 1, -alpha, newDepth - 1);
    }

    if(pvNode && (played == 1 || score > alpha)) {
      score = -search(-beta, -alpha, newDepth - 1);
    }

    undoMove(board, move);

    if(score > best) {
      best = score;
      bestMove = move;

      if(score > alpha) {
        alpha = score;
        updatePv(ply, move);

        if(alpha >= beta)
          break;
      }
    }
  }

  TT->prefetch(key);

  if(!played)
    return (isCheck ? -INF + ply : 0);

  /// update killers and history heuristics

  if(best >= beta && !isNoisyMove(board, bestMove)) {
    if(killers[ply][0] != bestMove) {
      killers[ply][1] = killers[ply][0];
      killers[ply][0] = bestMove;
    }
    History :: updateHistory(this, quiets, nrQuiets, ply, depth * depth);
  }

  /// update tt only if we aren't in a singular search

  if(excluded == NULLMOVE) {
    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(key, best, depth, ply, bound, bestMove, eval);
  }

  return best;
}

void Search :: startSearch(Info *_info) {
  int alpha, beta, score = 0;
  int bestMove = NULLMOVE;

  nodes = selDepth = tbHits = 0;
  t0 = getTime();
  flag = 0;
  checkCount = 0;

  memset(pvTable, 0, sizeof(pvTable));
  info = _info;

  if(principalSearcher) {

      /// corner cases

      uint16_t moves[256];

      int nrMoves = genLegal(board, moves);

      //board.print();

      /*for(int i = 0; i < nrMoves; i++)
        cout << toString(moves[i]) << " ";
      cout << endl;*/

      /// only 1 move legal

      if(PROBE_ROOT && nrMoves == 1) {
        waitUntilDone();
        std::cout << "bestmove " << toString(moves[0]) << std::endl;
        return;
      }

      /// position is in tablebase

      if(PROBE_ROOT && count(board.pieces[WHITE] | board.pieces[BLACK]) <= (int)TB_LARGEST) {
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
        if(probe != TB_RESULT_CHECKMATE && probe != TB_RESULT_FAILED && probe != TB_RESULT_STALEMATE) {
          int to = int(TB_GET_TO(probe)), from = int(TB_GET_FROM(probe)), promote = TB_GET_PROMOTES(probe), ep = TB_GET_EP(probe);

          //cout << int(TB_GET_TO(probe)) << " " << int(TB_GET_FROM(probe)) << " " << from << " " << to << " " << prom << " " << ep << "\n";

          if(!promote && !ep) {
            move = getMove(from, to, 0, NEUT);
          } else {
            int prom = 0;
            switch(promote) {
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

        for(auto &mv : moves) {
          if(mv == move) {
            waitUntilDone();
            std::cout << "bestmove " << toString(move) << std::endl;
            return;
          }
        }
      }
  }

  if(threadCount)
    startWorkerThreads(info);

  //cout << params[0].nodes << "\n";

  uint64_t totalNodes = 0, totalHits = 0;
  int lastScore = 0;

  //cout << info->depth << endl;

  for(tDepth = 1; tDepth <= info->depth; tDepth++) {

    //cout << "depth = " << depth << "\n";
    int window = 10;

    if(tDepth >= 6) {
      alpha = std::max(-INF, lastScore - window);
      beta = std::min(INF, lastScore + window);
    } else {
      alpha = -INF;
      beta = INF;
    }

    int initDepth = tDepth;
    while(true) {

      score = search(alpha, beta, tDepth);

      if(principalSearcher && printStats && ((alpha < score && score < beta) || getTime() > t0 + 3000)) {
        if(principalSearcher) {
          totalNodes = nodes;
          totalHits = tbHits;
          //cout << "principal thread has analyzed " << nodes << " nodes" << endl;
          for(int i = 0; i < threadCount; i++) {
            totalNodes += params[i].nodes;
            //cout << "thread " << i << " has analyzed " << params[i].nodes << " nodes" << endl;
            totalHits += params[i].tbHits;
          }
        }
        uint64_t t = (uint64_t)getTime() - t0;
        std::cout << "info score ";
        if(score > MATE)
          std::cout << "mate " << (INF - score + 1) / 2;
        else if(score < -MATE)
          std::cout << "mate -" << (INF + score + 1) / 2;
        else
          std::cout << "cp " << score;
        if(score >= beta)
          std::cout << " lowerbound";
        else if(score <= alpha)
          std::cout << " upperbound";
        std::cout << " depth " << initDepth << " seldepth " << selDepth << " nodes " << totalNodes;
        if(t)
          std::cout << " nps " << totalNodes * 1000 / t;
        std::cout << " time " << t << " ";
        std::cout << "tbhits " << totalHits << " hashfull " << TT->tableFull() << " ";
        std::cout << "pv ";
        printPv();
        std::cout << std::endl;
      }

      if(flag & TERMINATED_SEARCH)
        break;

      if(-INF < score && score <= alpha) {
        beta = (beta + alpha) / 2;
        alpha = std::max(-INF, alpha - window);
      } else if(beta <= score && score < INF) {
        beta = std::min(INF, beta + window);
      } else {
        if(pvTableLen[0])
          bestMove = pvTable[0][0];
        lastScore = score;
        break;
      }

      window += window / 2;

    }

    if(flag & TERMINATED_SEARCH)
      break;

    if(info->timeset && getTime() > info->stopTime) {
      flag |= TERMINATED_BY_TIME;
      break;
    }

    if(tDepth == info->depth) {
      flag |= TERMINATED_BY_TIME;
      break;
    }
  }

  if(threadCount)
    stopWorkerThreads();

  waitUntilDone();

  if(principalSearcher && printStats)
    std::cout << "bestmove " << toString(bestMove) << std::endl;

  //TT->age();
}

void Search :: clearHistory() {
  memset(hist, 0, sizeof(hist));
  memset(cmTable, 0, sizeof(cmTable));

  for(int i = 0; i < threadCount; i++) {
    memset(params[i].hist, 0, sizeof(params[i].hist));
    memset(params[i].cmTable, 0, sizeof(params[i].cmTable));
  }
}

void Search :: clearKillers() {
  memset(killers, 0, sizeof(killers));

  for(int i = 0; i < threadCount; i++)
    memset(params[i].killers, 0, sizeof(params[i].killers));
}

void Search :: clearStack() {
  memset(Stack, 0, sizeof(Stack));
  memset(follow, 0, sizeof(follow));
  memset(pvTableLen, 0, sizeof(pvTableLen));

  for(int i = 0; i < threadCount; i++) {
    memset(params[i].Stack, 0, sizeof(params[i].Stack));
    memset(params[i].follow, 0, sizeof(params[i].follow));
    memset(params[i].pvTableLen, 0, sizeof(params[i].pvTableLen));
  }
}

void Search :: printPv() {
  for(int i = 0; i < pvTableLen[0]; i++) {
    std::cout << toString(pvTable[0][i]) << " ";
  }
}

void Search :: updatePv(int ply, int move) {
  pvTable[ply][0] = move;
  for(int i = 0; i < pvTableLen[ply + 1]; i++)
    pvTable[ply][i + 1] = pvTable[ply + 1][i];
  pvTableLen[ply] = 1 + pvTableLen[ply + 1];
}

void Search :: startWorkerThreads(Info *info) {
  for(int i = 0; i < threadCount; i++) {
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

void Search :: flagWorkersStop() {
  for(int i = 0; i < threadCount; i++) {
    params[i].SMPThreadExit = true;
    params[i].flag |= TERMINATED_BY_TIME;
  }
}

void Search :: stopWorkerThreads() {
  flagWorkersStop();

  for(int i = 0; i < threadCount; i++) {
    while(params[i].lazyDepth);
  }
}

void Search :: waitUntilDone() {
  if(!principalSearcher)
    return;
}

void Search :: isReady() {
  flagWorkersStop();
  flag |= TERMINATED_BY_USER;
  std::unique_lock <std::mutex> lk(readyMutex);
  std::cout << "readyok" << std::endl;
}

void Search :: startPrincipalSearch(Info *info) {
  principalSearcher = true;

  readyMutex.lock();
  setTime(info);
  lazyDepth = 1;
  readyMutex.unlock();
  lazyCV.notify_one();

  if(!principalThread)
    principalThread.reset(new std::thread(&Search :: lazySMPSearcher, this));
}

void Search :: stopPrincipalSearch() {
  flag |= TERMINATED_BY_USER;
}

void Search :: setThreadCount(int nrThreads) {
  if(nrThreads == threadCount)
    return;

  releaseThreads();

  threadCount = nrThreads;
  threads.reset(new std::thread[nrThreads]);
  params.reset(new Search[nrThreads]);

  for(int i = 0; i < nrThreads; i++)
    threads[i] = std::thread(&Search :: lazySMPSearcher, &params[i]);
}

int Search :: getThreadCount() { /// for debugging (i got a crash related to threadCount)
  return threadCount + 1;
}

void Search :: lazySMPSearcher() {
  while(!terminateSMP) {

    {
      std::unique_lock <std::mutex> lk(readyMutex);

      lazyCV.wait(lk, std::bind(&Search :: isLazySMP, this));

      if(terminateSMP)
        return;

      tDepth = lazyDepth;

      startSearch(info);
      resetLazySMP();
    }
  }
}

void Search :: releaseThreads() {
  for(int i = 0; i < threadCount; i++) {
    if(threads[i].joinable()) {
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

void Search :: _setFen(std::string fen) {
  for(int i = 0; i < threadCount; i++) {
    params[i].board.setFen(fen);
  }

  board.setFen(fen);
}

void Search :: _makeMove(uint16_t move) {
  for(int i = 0; i < threadCount; i++)
    makeMove(params[i].board, move);

  makeMove(board, move);
}

void Search :: clearBoard() {
  for(int i = 0; i < threadCount; i++)
    params[i].board.clear();

  board.clear();
}
