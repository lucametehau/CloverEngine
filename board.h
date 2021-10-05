#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include "defs.h"
#include "net.h"

struct StackEntry { /// info to keep in the stack
  uint16_t move, piece;
  int eval;
};

class Undo {
public:
  int8_t enPas;
  uint8_t castleRights;
  uint8_t captured;
  uint16_t halfMoves, moveIndex;
  //uint64_t checkers;
  uint64_t key;
};

class Board {
public:
  bool turn;

  uint8_t captured; /// keeping track of last captured piece so i reduce the size of move
  uint8_t castleRights; /// 1 - bq, 2 - bk, 4 - wq, 8 - wk
  int8_t enPas;
  uint8_t board[64];

  uint16_t ply, gamePly;
  uint16_t halfMoves, moveIndex;

  uint64_t bb[13];
  uint64_t pieces[2];
  uint64_t key;
  Undo history[1000]; /// fuck it

  Network NN;

  Board() {
    halfMoves = moveIndex = key = 0;
    turn = 0;
    ply = gamePly = 0;
    for(int i = 0; i <= 12; i++)
      bb[i] = 0;
    for(int i = 0; i < 64; i++)
      board[i] = 0;
    castleRights = 0;
    captured = 0;
  }

  Board(const Board &other) {
    halfMoves = other.halfMoves;
    moveIndex = other.moveIndex;
    turn = other.turn;
    key = other.key;
    gamePly = other.gamePly;
    ply = other.ply;
    //checkers = other.checkers;
    for(int i = 0; i <= 12; i++)
      bb[i] = other.bb[i];
    for(int i = BLACK; i <= WHITE; i++)
      pieces[i] = other.pieces[i];
    for(int i = 0; i < 64; i++)
      board[i] = other.board[i];
    castleRights = other.castleRights;
    captured = other.captured;
  }

  uint64_t diagSliders(int color) {
    return bb[getType(BISHOP, color)] | bb[getType(QUEEN, color)];
  }

  uint64_t orthSliders(int color) {
    return bb[getType(ROOK, color)] | bb[getType(QUEEN, color)];
  }

  int piece_type_at(int sq) {
    return piece_type(board[sq]);
  }

  int piece_at(int sq) {
    return board[sq];
  }

  /*int color_at(int sq) {
    if(board[sq] == 0) /// empty square
      return -1;
    return board[sq] / 7;
  }*/

  int king(int color) {
    return Sq(bb[getType(KING, color)]);
  }

  bool isCapture(int move) {
    return (board[sqTo(move)] > 0);
  }

  void clear() {
    ply = 0;

    NetInput input = toNetInput();

    NN.calc(input);
  }

  void print() {
    for(int i = 7; i >= 0; i--) {
      for(int j = 0; j <= 7; j++)
        std::cout << piece[board[8 * i + j]] << " ";
      std::cout << "\n";
    }
  }

  NetInput toNetInput() {
    NetInput ans;

    for(int i = 1; i <= 12; i++) {
      uint64_t b = bb[i];
      while(b) {
        uint64_t b2 = lsb(b);
        ans.ind.push_back(netInd(i, Sq(b2)));
        b ^= b2;
      }
    }

    return ans;
  }

  void setFen(const std::string fen) {
    int ind = 0;
    key = 0;

    ply = gamePly = 0;
    captured = 0;

    //checkers = 0;

    for(int i = 0; i <= 12; i++)
      bb[i] = 0;

    pieces[BLACK] = pieces[WHITE] = 0;

    for(int i = 7; i >= 0; i--) {
      int j = 0;
      while(fen[ind] != '/' && fen[ind] != ' ') {
        int sq = getSq(i, j);
        if(fen[ind] < '0' || '9' < fen[ind]) {
          board[sq] = cod[int(fen[ind])];
          key ^= hashKey[board[sq]][sq];

          pieces[(board[sq] > 6)] |= (1ULL << sq);
          bb[board[sq]] |= (1ULL << sq);
          j++;
        } else {
          int nr = fen[ind] - '0';
          while(nr)
            board[sq] = 0, j++, sq++, nr--;
        }
        ind++;
      }
      ind++;
    }

    if(fen[ind] == 'w')
      turn = WHITE;
    else
      turn = BLACK;

    key ^= turn;

    //cout << "key " << key << "\n";

    castleRights = 0;
    ind += 2;
    if(fen[ind] == 'K')
      castleRights |= (1 << 3), ind++, key ^= castleKey[1][1];
    if(fen[ind] == 'Q')
      castleRights |= (1 << 2), ind++, key ^= castleKey[1][0];
    if(fen[ind] == 'k')
      castleRights |= (1 << 1), ind++, key ^= castleKey[0][1];
    if(fen[ind] == 'q')
      castleRights |= (1 << 0), ind++, key ^= castleKey[0][0];
    if(fen[ind] == '-')
      ind++;


    //cout << "key " << key << "\n";
    ind++;
    if(fen[ind] != '-') {
      int file = fen[ind] - 'a';
      ind++;
      int rank = fen[ind] - '1';
      ind += 2;
      enPas = getSq(rank, file);

      key ^= enPasKey[enPas];
    } else {
      enPas = -1;
      ind += 2;
    }
    //cout << "key " << key << "\n";

    int nr = 0;
    while('0' <= fen[ind] && fen[ind] <= '9')
      nr = nr * 10 + fen[ind] - '0', ind++;
    halfMoves = nr;

    ind++;
    nr = 0;
    while('0' <= fen[ind] && fen[ind] <= '9')
      nr = nr * 10 + fen[ind] - '0', ind++;
    moveIndex = nr;

    NetInput input = toNetInput();

    NN.calc(input);
  }

  std::string fen() {
    std::string fen = "";
    for(int i = 7; i >= 0; i--) {
      int cnt = 0;
      for(int j = 0, sq = i * 8; j < 8; j++, sq++) {
        if(board[sq] == 0)
          cnt++;
        else {
          if(cnt)
            fen += char(cnt + '0');
          cnt = 0;
          fen += piece[board[sq]];
        }
      }
      if(cnt)
        fen += char(cnt + '0');
      if(i)
        fen += "/";
    }
    fen += " ";
    fen += (turn == WHITE ? "w" : "b");
    fen += " ";
    if(castleRights & 8)
      fen += "K";
    if(castleRights & 4)
      fen += "Q";
    if(castleRights & 2)
      fen += "k";
    if(castleRights & 1)
      fen += "q";
    if(!castleRights)
      fen += "-";
    fen += " ";
    if(enPas >= 0) {
      fen += char('a' + enPas % 8);
      fen += char('1' + enPas / 8);
    } else
      fen += "-";
    fen += " ";
    std::string s = "";
    int nr = halfMoves;
    while(nr)
      s += char('0' + nr % 10), nr /= 10;
    std::reverse(s.begin(), s.end());
    if(halfMoves)
      fen += s;
    else
      fen += "0";
    fen += " ";
    s = "", nr = moveIndex;
    while(nr)
      s += char('0' + nr % 10), nr /= 10;
    std::reverse(s.begin(), s.end());
    fen += s;
    return fen;
  }

  uint64_t hash() {
    uint64_t h = 0;
    h ^= turn;
    for(int i = 0; i < 64; i++) {
      if(board[i])
        h ^= hashKey[board[i]][i];
    }
    //cout << h << "\n";
    for(int i = 0; i < 4; i++)
      h ^= castleKey[i / 2][i % 2] * ((castleRights >> i) & 1);
    //cout << h << "\n";

    h ^= (enPas >= 0 ? enPasKey[enPas] : 0);
    //cout << h << "\n";
    return h;
  }

  bool isMaterialDraw() {
    /// KvK, KBvK, KNvK, KNNvK
    int num = count(pieces[WHITE]) + count(pieces[BLACK]);
    return (num == 2 || (num == 3 && (bb[WN] || bb[BN] || bb[WB] || bb[BB])) ||
           (num == 4 && (count(bb[WN]) == 2 || count(bb[BN]) == 2)));
  }
};

class Info {
public:
  long double startTime, stopTime;
  long double goodTimeLim, hardTimeLim;
  int depth, sel_depth, multipv;
  int timeset;
  int movestogo;
  long long nodes;

  bool quit, stopped;
};
