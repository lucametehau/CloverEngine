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

void init(Info* info) {
    info->quit = info->stopped = 0;
    info->depth = DEPTH;
    info->multipv = 1;
    info->nodes = -1;
    info->chess960 = false;
}

void Board::set_fen(const std::string fen) {
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
            int sq = get_sq(i, j);
            if (fen[ind] < '0' || '9' < fen[ind]) {
                place_piece_at_sq(cod[int(fen[ind])], sq);
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
            castleRights |= (1 << 3), key ^= castleKey[WHITE][1], ind++;
        if ('A' <= fen[ind] && fen[ind] <= 'H' && kingSq > fen[ind] - 'A')
            castleRights |= (1 << 2), key ^= castleKey[WHITE][0], ind++;
        kingSq = king(BLACK);
        if ('a' <= fen[ind] && fen[ind] <= 'h' && 56 + fen[ind] - 'a' > kingSq)
            castleRights |= (1 << 1), key ^= castleKey[BLACK][1], ind++;
        if ('a' <= fen[ind] && fen[ind] <= 'h' && kingSq > 56 + fen[ind] - 'a')
            castleRights |= (1 << 0), key ^= castleKey[BLACK][0], ind++;
        chess960 = true;

    }
    else {
        if (fen[ind] == 'K')
            castleRights |= (1 << 3), ind++, key ^= castleKey[1][1];
        if (fen[ind] == 'Q')
            castleRights |= (1 << 2), ind++, key ^= castleKey[1][0];
        if (fen[ind] == 'k')
            castleRights |= (1 << 1), ind++, key ^= castleKey[0][1];
        if (fen[ind] == 'q')
            castleRights |= (1 << 0), ind++, key ^= castleKey[0][0];
        if (fen[ind] == '-')
            ind++;
    }
    int a = 64, b = 64;
    for (int i = 0; i < 8; i++) {
        if (piece_at(i) == WR) {
            a = b;
            b = i;
        }
    }

    for (auto& rook : { a, b }) {
        if (rook != 64) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < king(WHITE) && (castleRights & 4)) rookSq[WHITE][0] = rook;
            if (king(WHITE) < rook && (castleRights & 8)) rookSq[WHITE][1] = rook;
        }
    }
    a = 64, b = 64;
    for (int i = 56; i < 64; i++) {
        if (piece_at(i) == BR) {
            b = a;
            a = i;
        }
    }
    for (auto& rook : { a, b }) {
        if (rook != 64) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < king(BLACK) && (castleRights & 1)) rookSq[BLACK][0] = rook;
            if (king(BLACK) < rook && (castleRights & 2)) rookSq[BLACK][1] = rook;
        }
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
        enPas = get_sq(rank, file);

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

    NetInput input = to_netinput();

    checkers = getAttackers(*this, 1 ^ turn, pieces[WHITE] | pieces[BLACK], king(turn));
    pinnedPieces = getPinnedPieces(*this, turn);

    NN.calc(input, turn);
}

void Board::setFRCside(bool color, int idx) {
    int ind = (color == WHITE ? 0 : 56);

    place_piece_at_sq(get_piece(BISHOP, color), ind + 1 + (idx % 4) * 2);
    idx /= 4;
    place_piece_at_sq(get_piece(BISHOP, color), ind + 0 + (idx % 4) * 2);
    idx /= 4;
    int cnt = 0;
    for (int i = ind; i < ind + 8; i++) {
        if (!board[i]) {
            if (idx % 6 == cnt) {
                place_piece_at_sq(get_piece(QUEEN, color), i);
                break;
            }
            cnt++;
        }
    }
    idx /= 6;

    constexpr int vals[10][2] = {
        {0, 1},
        {0, 2},
        {0, 3},
        {0, 4},
        {1, 2},
        {1, 3},
        {1, 4},
        {2, 3},
        {2, 4},
        {3, 4},
    };
    cnt = 0;
    for (int i = ind; i < ind + 8; i++) {
        if (!board[i]) {
            if (cnt == vals[idx][0] || cnt == vals[idx][1]) {
                place_piece_at_sq(get_piece(KNIGHT, color), i);
            }
            cnt++;
        }
    }
    cnt = 0;
    for (int i = ind; i < ind + 8; i++) {
        if (!board[i]) {
            if (cnt == 0 || cnt == 2) {
                place_piece_at_sq(get_piece(ROOK, color), i);
            }
            else {
                place_piece_at_sq(get_piece(KING, color), i);
            }
            cnt++;
        }
    }
}

void Board::set_dfrc(int idx) {
    ply = gamePly = 0;
    captured = 0;

    //checkers = 0;

    for (int i = 0; i <= 12; i++)
        bb[i] = 0;

    for (int i = 0; i < 64; i++)
        board[i] = 0;

    pieces[BLACK] = pieces[WHITE] = 0;

    int idxw = idx / 960, idxb = idx % 960;
    setFRCside(WHITE, idxw);
    setFRCside(BLACK, idxb);

    for (int i = 8; i < 16; i++)
        place_piece_at_sq(WP, i);
    for (int i = 48; i < 56; i++)
        place_piece_at_sq(BP, i);

    turn = WHITE;
    key ^= turn;

    castleRights = 15;

    int a = 64, b = 64;
    for (int i = 0; i < 8; i++) {
        if (piece_at(i) == WR) {
            a = b;
            b = i;
        }
    }

    for (auto& rook : { a, b }) {
        if (rook != 64) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < king(WHITE) && (castleRights & 4)) rookSq[WHITE][0] = rook;
            if (king(WHITE) < rook && (castleRights & 8)) rookSq[WHITE][1] = rook;
        }
    }
    a = 64, b = 64;
    for (int i = 56; i < 64; i++) {
        if (piece_at(i) == BR) {
            b = a;
            a = i;
        }
    }
    for (auto& rook : { a, b }) {
        if (rook != 64) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < king(BLACK) && (castleRights & 1)) rookSq[BLACK][0] = rook;
            if (king(BLACK) < rook && (castleRights & 2)) rookSq[BLACK][1] = rook;
        }
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

    enPas = -1;
    halfMoves = 0;
    moveIndex = 1;

    NetInput input = to_netinput();

    checkers = getAttackers(*this, 1 ^ turn, pieces[WHITE] | pieces[BLACK], king(turn));
    pinnedPieces = getPinnedPieces(*this, turn);

    NN.calc(input, turn);
}

bool Board::is_draw(int ply) {
    if (halfMoves < 100 || !checkers)
        return isMaterialDraw() || is_repetition(ply) || halfMoves >= 100;
    int nrmoves = 0;
    uint16_t moves[256];
    nrmoves = gen_legal_moves(*this, moves);
    return nrmoves > 0;
}