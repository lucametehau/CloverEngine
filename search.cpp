#include <bits/stdc++.h>
#include "search.h"
#include "evaluate.h"
#include "movepick.h"
#pragma once

using namespace std;

int quiesce(Board* board, Info* info, int alpha, int beta, int color);
int search(Board* board, Info* info, int alpha, int beta, int depth, int color, int excluded = NULLMOVE);
bool extend(Board* board, Info *info, int beta, int depth, int color, int hashMove, int value);
void iterativeDeepening(Board *board, Info *info);
void clearForSearch();
void initSearch();

extern const bool printStats = 1;

struct Node {
  int move, reduced_depth, depth;
};

struct StackEntry { /// info to keep in the stack
  int move, piece, eval;
};

extern int nodes, ttHits, sel_depth;
extern bool debug;
extern Board board2[1];
extern tt :: HashTable *TT;

extern StackEntry Stack[105];
extern int pvTable[205][205], pvTableLen[205];
//int killers[205][2], hist[2][13][64];
extern vector <int> currPv;

int lmrCnt[2][9];
int lmrRed[100][200];

void reset(Board *board, Info *info) {
  board->clear();
  //table.clear();
  //pvMove.clear();
  //info->timeset = 1;
  info->stopped = info->quit = info->nodes = 0;
  info->startTime = getTime();
  info->sel_depth = 0;
  currPv.clear();
  for(int i = 0; i < 200; i++)
    pvTableLen[i] = 0;
  /*for(int i = 0; i <= 12; i++) {
    for(int j = 0; j < 64; j++)
      hist[0][i][j] = hist[1][i][j] = 0;
  }
  for(int i = 0; i < 100; i++)
    killers[i][0] = killers[i][1] = NULLMOVE;*/
}

void update(Info *info) {
  if(info->timeset && getTime() > info->stopTime)
    info->stopped = 1;
}

void updatePv(int ply, int move) {
  pvTable[ply][ply] = move;
  for(int i = ply + 1; i < pvTableLen[ply + 1]; i++)
    pvTable[ply][i] = pvTable[ply + 1][i];
  pvTableLen[ply] = pvTableLen[ply + 1];
}

vector <int> getPv() {
  return vector <int>(pvTable[0], pvTable[0] + pvTableLen[0]);
}

void printPv(Board *board, int bound) {
  vector <int> pv;
  if(bound != EXACT)
    pv = currPv;
  else
    pv = getPv();
  for(auto &move : pv) {
    //makeMove(board, move);
    cout << toString(move) << " ";
  }
  //cout << "To move " << (board->turn == 1 ? "WHITE" : "BLACK") << " , evaluation = " << evaluate(board) << "\n";

  //for(int i = pv.size() - 1; i >= 0; i--)
  //  undoMove(board, pv[i]);
}

void findBestMove(vector <Move> &moves, int index) {
  int ind = index;
  for(int i = index + 1; i < (int)moves.size(); i++) {
    if(moves[i].score > moves[ind].score) {
      ind = i;
    }
  }
  swap(moves[index], moves[ind]);
}

int quiesce(Board *board, Info *info, int alpha, int beta, int color) {
  info->nodes++;

  int ply = board->ply;

  pvTableLen[ply] = ply;

  info->sel_depth = max(info->sel_depth, board->ply);

  if(!(info->nodes & 2047))
    update(info);

  /*if((isRepetition(board) || board->halfMoves >= 100) && board->ply) /// check for draw
    return 0;*/

  if(board->isMaterialDraw())
    return 0;

  if(info->stopped)
    return ABORT;

    /// check if in transposition table

  uint64_t key = board->key;
  int score = 0, best = -INF;
  int alphaOrig = alpha;
  int hashMove = NULLMOVE, bound = NONE;

  tt :: Entry entry = {};

  if(TT->probe(key, entry)) {
    ttHits++;
    Stack[ply].eval = entry.eval;
    score = entry.value(ply);
    bound = entry.bound();
    if(bound == EXACT)
      return score;
    else if(bound == LOWER && score >= beta)
      return score;
    else if(bound == UPPER && score <= alpha)
      return score;
  } else {
    Stack[ply].eval = color * evaluate(board);
  }

  int eval = Stack[ply].eval;

  if(eval >= beta)
    return eval;

  alpha = max(alpha, eval);
  best = eval;

  vector <int> moves = genLegalNoisy(board);
  vector <Move> sortedMoves;

  for(auto &i : moves) {
    sortedMoves.push_back({i, captureValue[board->piece_type_at(sqFrom(i))][board->piece_type_at(sqTo(i))]});
  }

  for(int i = 0; i < sortedMoves.size(); i++) {
    findBestMove(sortedMoves, i);
    int move = sortedMoves[i].move;

    int move_score = see(board, move);

    if(type(move) != PROMOTION) {

      if(move_score < max(1, alpha - eval - 110))
        continue;
    }

    makeMove(board, move);
    Stack[ply].move = move;
    score = -quiesce(board, info, -beta, -alpha, -color);
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

bool extend(Board *board, Info *info, int beta, int depth, int color, int hashMove, int value) {
  int score = -INF, beta2 = max(value - depth, -INF);
  int move, quiets = 0, tacticals = 0;
  bool skip = 0;

  undoMove(board, hashMove);

  Movepick picker(hashMove);
  while((move = picker.nextMove(board, skip)) != NULLMOVE) {
    if(move == hashMove)
      continue;

    makeMove(board, move);
    score = -search(board, info, -beta2 - 1, -beta2, depth / 2 - 1, -color);
    undoMove(board, move);

    if(score > beta2)
      break;

    if(!isNoisyMove(board, move))
      quiets++;
    else
      tacticals++;

    skip = (quiets >= 6);

    if(skip && tacticals >= 3)
      break;
  }

  makeMove(board, hashMove);

  return (score <= beta2);
}

int search(Board *board, Info *info, int alpha, int beta, int depth, int color, int excluded) {
  int pvNode = (alpha != beta - 1), rootNode = (board->ply == 0);
  int hashMove = NULLMOVE, ply = board->ply;
  int alphaOrig = alpha;
  uint64_t key = board->key;
  vector <int> prevQuiets;

  info->nodes++;

  if(info->nodes & 2047)
    update(info);

  if(info->stopped)
    return ABORT;

  info->sel_depth = max(info->sel_depth, ply);

  int played = 0, searched = 0, bound = NONE, skip = 0;
  int best = -INF, bestMove = NULLMOVE;

  if(depth <= 0)
    return quiesce(board, info, alpha, beta, color);

  TT->prefetch(key);
  pvTableLen[ply] = ply;

  if(!rootNode) {
    if(isRepetition(board) || board->halfMoves >= 100 || board->isMaterialDraw())
      return 0;
    alpha = max(alpha, -INF + ply);
    beta = min(beta, INF - ply - 1);
    if(alpha >= beta)
      return alpha;
  }

  tt :: Entry entry = {};

  if(TT->probe(key, entry)) {
    int score = entry.value(ply);
    bound = entry.bound(), hashMove = entry.move;
    Stack[ply].eval = entry.eval;
    if(entry.depth() >= depth && (depth == 0 || !pvNode)) {
      if(bound == EXACT || (bound == LOWER && score >= beta) || (bound == UPPER && score <= alpha))
        return score;
    }
  } else {
    Stack[ply].eval = color * evaluate(board);
  }

  int eval = Stack[ply].eval;
  bool isCheck = inCheck(board), improving = (ply >= 2 && eval > Stack[ply - 2].eval);

  killers[ply + 1][0] = killers[ply + 1][1] = NULLMOVE;

  /// razoring

  if(!pvNode && !isCheck && depth <= 1 && eval + 325 < alpha)
    return quiesce(board, info, alpha, beta, color);

  /// static null move pruning

  if(!pvNode && !isCheck && depth <= 8 && eval - 85 * depth > beta)
    return eval;

  /// null move pruning

  if(!pvNode && !isCheck && eval >= beta && depth >= 2 && ply >= 2 &&
     Stack[ply - 1].move != NULLMOVE && Stack[ply - 2].move != NULLMOVE &&
     count(board->pieces[board->turn] ^ board->bb[getType(PAWN, board->turn)] ^ board->bb[getType(KING, board->turn)]) >= 2) {
    int R = 3 + depth / 5 + min(3, (eval - beta) / 150);
    makeNullMove(board);

    int score = -search(board, info, -beta, -beta + 1, depth - R, -color);

    undoNullMove(board);

    if(score >= beta)
      return beta;
  }

  Movepick picker(hashMove);

  int move;

  while((move = picker.nextMove(board, skip)) != NULLMOVE) {

    if(move == excluded)
      continue;

    bool isQuiet = !isNoisyMove(board, move);

    /// quiet move pruning (to add things...)

    if(!rootNode && best > -MATE) {
      if(isQuiet) {
        /// futility pruning
        if(depth <= 8 && eval + 95 * depth <= alpha)
          skip = 1;
        /// late move pruning
        if(depth <= 8 && prevQuiets.size() >= lmrCnt[improving][depth])
          skip = 1;
      }

      /// see pruning

      if(depth <= 8 && !isCheck) {
        int seeMargin[2];

        seeMargin[1] = -80 * depth;
        seeMargin[0] = -18 * depth * depth;

        if(see(board, move) < seeMargin[isQuiet])
          continue;
      }
    }

    Stack[ply].move = move;
    Stack[ply].piece = board->piece_at(sqFrom(move));
    makeMove(board, move);
    played++;

    if(isQuiet)
      prevQuiets.push_back(move);

    bool ex = isCheck;

    /// singular extension

    if(!rootNode && !excluded && best > -MATE && depth >= 8 && move == hashMove && entry.depth() >= depth - 3 && (bound & LOWER)) {
      int beta2 = entry.value(ply) - depth;
      int score = search(board, info, beta2 - 1, beta2, depth / 2, -color, move);

      if(score < beta2)
        ex |= 1;
    }

    int newDepth = depth + (ex && !rootNode), R = 1;

    /// quiet late move reduction

    if(isQuiet && depth >= 3 && played > 1 + 2 * rootNode) {
      R += lmrRed[depth][min(63, played)];

      R += !improving;

      R -= picker.stage < STAGE_QUIETS;

      R -= 2 * pvNode;

      R = min(depth - 1, max(R, 1));
    }

    int score = -INF;
    bool yes = 0;

    if(R != 1)
      score = -search(board, info, -alpha - 1, -alpha, newDepth - R, -color);

    if((R != 1 && score > alpha) || (R == 1 && !(pvNode && played == 1)))
      score = -search(board, info, -alpha - 1, -alpha, newDepth - 1, -color);

    if(pvNode && (played == 1 || score > alpha))
      score = -search(board, info, -beta, -alpha, newDepth - 1, -color);

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

  if(best >= beta && !isNoisyMove(board, bestMove)) {
    if(killers[ply][0] != bestMove) {
      killers[ply][1] = killers[ply][0];
      killers[ply][0] = bestMove;
    }
    updateHistory(board, prevQuiets, ply, depth * depth);
  }

  if(excluded == NULLMOVE) {
    bound = (best >= beta ? LOWER : (best > alphaOrig ? EXACT : UPPER));
    TT->save(key, best, depth, ply, bound, bestMove, eval);
  }

  return best;
}

void iterativeDeepening(Board *board, Info *info) {
  int color = (board->turn == WHITE ? 1 : -1), depth = 1, alpha, beta, score = color * evaluate(board);
  reset(board, info);

  //cout << info->stopTime << " " << info->startTime << "\n";

  long double time1 = clock();
  bool isCheck = inCheck(board);
  int depthLimit = DEPTH;

  sel_depth = 0;

  //depthLimit = 1;

  //return;

  while(depth <= depthLimit) {

    int window = 10;

    if(depth >= 6) {
      alpha = max(-INF, score - window);
      beta = min(INF, score + window);
    } else {
      alpha = -INF;
      beta = INF;
    }
    if(info->stopped)
      break;

    int initDepth = depth;
    while(true) {
      int bound = NONE;

      score = search(board, info, alpha, beta, max(1, depth), color);

      if(info->stopped)
        break;
      //cout << "alpha = " << alpha << ", beta = " << beta << ", score = " << score << "\n";
      //cout << alpha << " " << beta << " intermediate\n";

      if(-INF < score && score <= alpha) {
        beta = (beta + alpha) / 2;
        bound = UPPER;
        alpha = max(-INF, alpha - window);
        depth = initDepth;
      } else if(beta <= score && score < INF) {
        currPv = getPv();
        bound = LOWER;
        beta = min(INF, beta + window);
        depth -= (abs(score) <= INF / 2);
      } else {
        currPv = getPv();
        break;
      }

      window += window / 2;

      if(bound != NONE && printStats && score != ABORT) {
          cout << "info score ";
          if(score > MATE)
            cout << "mate " << (INF - score + 1) / 2;
          else if(score < -MATE)
            cout << "mate -" << (INF + score + 1) / 2;
          else
            cout << "cp " << score;
          cout << " depth " << initDepth << " seldepth " << info->sel_depth << " nodes " << info->nodes;
          cout << " nps " << info->nodes / (getTime() - info->startTime) * 1000 << " time " << getTime() - info->startTime << " ";
          cout << "hashfull " << TT->tableFull() << " ";
          cout << "pv ";
          printPv(board, bound);
          cout << endl;
      }
    }
    //score = search(board, info, -INF, INF, color, depth, 1);
    //cout << score << "\n";

    //cout << ttHits << " " << sizeof(TT) << "\n";
    //cout << alpha << " " << beta << " INFO\n";
    /*tt :: Entry entry = {};
    TT->probe(board->key, entry);
    cout << entry.score << " " << entry.eval << " " << entry.depth() << " " << entry.bound() << " " << entry.generation() << "\n";*/

    if(pvTableLen[0] && printStats && score != ABORT) {

      cout << "info score ";
      if(score > MATE)
        cout << "mate " << (INF - score + 1) / 2;
      else if(score < -MATE)
        cout << "mate -" << (INF + score + 1) / 2;
      else
        cout << "cp " << score;
      cout << " depth " << initDepth << " seldepth " << info->sel_depth << " nodes " << info->nodes;
      cout << " nps " << info->nodes / (getTime() - info->startTime) * 1000 <<  " time " << getTime() - info->startTime << " ";
      cout << "hashfull " << TT->tableFull() << " ";
      cout << "pv ";
      printPv(board, EXACT);
      cout << endl;
    }

    depth = initDepth + 1;
  }
  long double time2 = clock();
  if(printStats) {
    cout << (time2 - time1) / 1000.0 << "s" << endl;
    cout << "bestmove " << toString(currPv[0]) << endl;
  }
  TT->age();
}

void initSearch() {
  for(int i = 0; i < 100; i++) { /// depth
    for(int j = 0; j < 200; j++) /// moves played
      lmrRed[i][j] = int(0.75 + 0.5 * log(i) * log(j));
  }
  for(int i = 1; i < 9; i++) {
    lmrCnt[0][i] = 2.5 + 2 * i * i / 4.5;
    lmrCnt[1][i] = 4.0 + 4 * i * i / 4.5;
  }
  delete TT;
  TT = new tt :: HashTable(256 * 1048576);
}

void clearForSearch() {
  for(int i = 0; i < 200; i++)
    killers[i][0] = killers[i][1] = NULLMOVE;
  clearHistory();
  for(int i = 0; i < TT->entries; i++)
    TT->table[i] = {0ULL, NULLMOVE, 0, 0, 0};
}
