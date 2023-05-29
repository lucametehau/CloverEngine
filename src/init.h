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
#include <string>
#include "board.h"
#include "defs.h"
#include "move.h"
#include "attacks.h"

const char initBoard[] = {
  'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R',
  'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
  '.', '.', '.', '.', '.', '.', '.', '.',
  '.', '.', '.', '.', '.', '.', '.', '.',
  '.', '.', '.', '.', '.', '.', '.', '.',
  '.', '.', '.', '.', '.', '.', '.', '.',
  'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
  'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r',
};

void init(Info* info) {
    info->quit = info->stopped = 0;
    info->depth = DEPTH;
    info->multipv = 1;
    info->nodes = -1;
}

void Board::setFen(const std::string fen) {
    int ind = 0;
    key = 0;

    ply = gamePly = 0;
    captured = 0;

    //checkers = 0;

    for (int i = 0; i <= 12; i++)
        bb[i] = 0;

    pieces[BLACK] = pieces[WHITE] = 0;

    for (int i = 7; i >= 0; i--) {
        int j = 0;
        while (fen[ind] != '/' && fen[ind] != ' ') {
            int sq = getSq(i, j);
            if (fen[ind] < '0' || '9' < fen[ind]) {
                board[sq] = cod[int(fen[ind])];
                key ^= hashKey[board[sq]][sq];

                pieces[(board[sq] > 6)] |= (1ULL << sq);
                bb[board[sq]] |= (1ULL << sq);
                j++;
            }
            else {
                int nr = fen[ind] - '0';
                while (nr)
                    board[sq] = 0, j++, sq++, nr--;
            }
            ind++;
        }
        ind++;
    }

    if (fen[ind] == 'w')
        turn = WHITE;
    else
        turn = BLACK;

    key ^= turn;

    //cout << "key " << key << "\n";

    castleRights = 0;
    ind += 2;
    rookSq[WHITE][0] = rookSq[WHITE][1] = rookSq[BLACK][0] = rookSq[BLACK][1] = 64;
    if (fen[ind] != 'K' && fen[ind] != 'Q' && fen[ind] != 'k' && fen[ind] != 'q' && fen[ind] != '-') { // most likely chess 960
        int kingSq = king(WHITE);
        if ('A' <= fen[ind] && fen[ind] <= 'H' && fen[ind] - 'A' > kingSq)
            castleRights |= (1 << 3), key ^= castleKey[WHITE][1], rookSq[WHITE][1] = fen[ind] - 'A', ind++;
        if ('A' <= fen[ind] && fen[ind] <= 'H' && kingSq > fen[ind] - 'A')
            castleRights |= (1 << 2), key ^= castleKey[WHITE][0], rookSq[WHITE][0] = fen[ind] - 'A', ind++;
        kingSq = king(BLACK);
        if ('a' <= fen[ind] && fen[ind] <= 'h' && 56 + fen[ind] - 'a' > kingSq)
            castleRights |= (1 << 1), key ^= castleKey[BLACK][1], rookSq[BLACK][1] = 56 + fen[ind] - 'a', ind++;
        if ('a' <= fen[ind] && fen[ind] <= 'h' && kingSq > 56 + fen[ind] - 'a')
            castleRights |= (1 << 0), key ^= castleKey[BLACK][0], rookSq[BLACK][0] = 56 + fen[ind] - 'a', ind++;

    }
    else {
        if (fen[ind] == 'K')
            castleRights |= (1 << 3), ind++, key ^= castleKey[1][1], rookSq[WHITE][1] = 7;
        if (fen[ind] == 'Q')
            castleRights |= (1 << 2), ind++, key ^= castleKey[1][0], rookSq[WHITE][0] = 0;
        if (fen[ind] == 'k')
            castleRights |= (1 << 1), ind++, key ^= castleKey[0][1], rookSq[BLACK][1] = 63;
        if (fen[ind] == 'q')
            castleRights |= (1 << 0), ind++, key ^= castleKey[0][0], rookSq[BLACK][0] = 56;
        if (fen[ind] == '-')
            ind++;
    }
    for (int i = 0; i < 64; i++) castleRightsDelta[BLACK][i] = castleRightsDelta[WHITE][i] = 15;
    if (rookSq[BLACK][0] != 64)
        castleRightsDelta[BLACK][rookSq[BLACK][0]] = 14, castleRightsDelta[BLACK][king(BLACK)] = 12;
    if (rookSq[BLACK][1] != 64)
        castleRightsDelta[BLACK][rookSq[BLACK][1]] = 13, castleRightsDelta[BLACK][king(BLACK)] = 12;
    if (rookSq[WHITE][0] != 64)
        castleRightsDelta[WHITE][rookSq[WHITE][0]] = 11, castleRightsDelta[WHITE][king(WHITE)] = 3;
    if (rookSq[WHITE][1] != 64)
        castleRightsDelta[WHITE][rookSq[WHITE][1]] = 7, castleRightsDelta[WHITE][king(WHITE)] = 3;
    //std::cout << int(rookSq[WHITE][0]) << " " << int(rookSq[WHITE][1]) << " " << int(rookSq[BLACK][0]) << " " << int(rookSq[BLACK][1]) << "\n";
    //cout << "key " << key << "\n";
    ind++;
    if (fen[ind] != '-') {
        int file = fen[ind] - 'a';
        ind++;
        int rank = fen[ind] - '1';
        ind += 2;
        enPas = getSq(rank, file);

        key ^= enPasKey[enPas];
    }
    else {
        enPas = -1;
        ind += 2;
    }
    //cout << "key " << key << "\n";

    int nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9')
        nr = nr * 10 + fen[ind] - '0', ind++;
    halfMoves = nr;

    ind++;
    nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9')
        nr = nr * 10 + fen[ind] - '0', ind++;
    moveIndex = nr;

    NetInput input = toNetInput();

    checkers = getAttackers(*this, 1 ^ turn, pieces[WHITE] | pieces[BLACK], king(turn));
    pinnedPieces = getPinnedPieces(*this, turn);

    NN.calc(input, turn);
}
