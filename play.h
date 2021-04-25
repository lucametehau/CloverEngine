#include "move.h"
#include "search.h"
#pragma once

/*void playHuman(Board *board) {
  while(!isGameOver(board)) {
    vector <Move> moves = genLegalMoves(board);
    bool ok = 1;
    board->print();
    Move humanMove;
    while(ok) {
      string s;
      cout << "Make your move:\n";
      cin >> s;
      ok = 1;
      for(auto &mv : moves) {
        if(mv.move == s)
          ok = 0, humanMove = mv;
      }
      system("cls");
    }
    makeMove(board, humanMove);
    ap[board->key]++;
    board->print();
    clock_t time1 = clock();
    int score = iterativeDeepening(board, DEPTH);
    clock_t time2 = clock();
    Move move = pv[board->key];
    cout << "Score is = " << score << " and the best move is " << move.move << "\n";
    cout << "Time taken: " << (time2 - time1) / CLOCKS_PER_SEC << "\n";
    makeMove(board, move);
    ap[board->key]++;
  }
}

void playSelf(Board *board, Info *info) {
  vector <string> positions;
  ofstream out("positions2.txt");
  for(int i = 1; i <= 10; i++) {
      init(board);
      init(info);
      board->print();
      initSearch();
      info->timeset = 1;
      while(true) {
        vector <uint16_t> moves = genLegal(board);
        if(moves.empty() || isRepetition(board) || board->halfMoves >= 100 || board->isMaterialDraw())
          break;
        clock_t time1 = clock();
        info->startTime = getTime();
        info->stopTime = info->startTime + 105;
        iterativeDeepening(board, info);
        clock_t time2 = clock();
        //system("cls");
        uint16_t move = pvTable[0][0];
        cout << "Best move is " << toString(move) << "\n";
        cout << "Time taken: " << (time2 - time1) / CLOCKS_PER_SEC << "\n";

        makeMove(board, move);
        tt :: Entry entry = {};
        TT->probe(board->key, entry);
        if(abs(entry.score) < MATE)
          positions.push_back(board->fen());
        board->print();
      }
      int result;
      if(isRepetition(board))
        result = 1;
      else {
        if(inCheck(board))
          result = (board->turn == WHITE ? 0 : 2);
        else
          result = 1;
      }
      string ans = (result == 0 ? "[0.0]" : (result == 1 ? "[0.5]" : "[1.0]"));
      for(auto &pos : positions) {
        out << pos << " " << ans << "\n";
      }
  }
}*/
