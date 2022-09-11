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
#include "defs.h"
#include "board.h"
#include "move.h"
#include "evaluate.h"
#include "history.h"
#include <cassert>

const int BAD_CAP_SCORE = (int)-1e9;

int totalMoves;
long long totalOp;
int totalMP;

enum {
    STAGE_NONE = 0, STAGE_HASHMOVE, STAGE_GEN_NOISY, STAGE_GOOD_NOISY,
    STAGE_KILLER_1, STAGE_KILLER_2, STAGE_COUNTER,
    STAGE_GEN_QUIETS, STAGE_QUIETS, STAGE_BAD_NOISY,
}; /// move picker stages

bool see(Board& board, uint16_t move, int threshold);

const int seeVal[] = { 0, 100, 310, 330, 500, 1000, 20000 };

enum {
    NORMAL_MP = 0, NOISY_MP
};

class Movepick {
public:
    int stage;
    //int mp_type;

    uint16_t hashMove, killer1, killer2, counter, possibleCounter;
    int nrNoisy, nrQuiets, nrBadNoisy;
    int index;

    int threshold;

    uint16_t noisy[256], quiets[256], badNoisy[256];
    int scores[256];
    long long v[256];

    Movepick(const uint16_t HashMove, const uint16_t Killer1, const uint16_t Killer2, const uint16_t Counter, const int Threshold) {
        stage = STAGE_HASHMOVE;

        hashMove = HashMove;
        killer1 = (Killer1 != hashMove ? Killer1 : NULLMOVE);
        killer2 = (Killer2 != hashMove ? Killer2 : NULLMOVE);
        counter = (Counter != hashMove && Counter != killer1 && Counter != killer2 ? Counter : NULLMOVE);

        nrNoisy = nrQuiets = nrBadNoisy = 0;
        threshold = Threshold;

        totalMP++;
    }

    long long codify(int move_ind, int score) {
        return ((1LL * score) << 16) | move_ind;
    }

    void sortMoves(int nrMoves, uint16_t moves[], int scores[]) {
        for (int i = 0; i < nrMoves; i++)
            v[i] = codify(moves[i], scores[i]);
        for (int i = 1; i < nrMoves; i++) {
            long long sc = v[i];
            int j;
            for (j = i - 1; v[j] < sc && j >= 0; j--) {
                std::swap(v[j], v[j + 1]);
            }
            v[j + 1] = sc;
        }
        for (int i = 0; i < nrMoves; i++)
            moves[i] = v[i] & 65535;
    }

    uint16_t nextMove(Search* searcher, bool skip, bool noisyPicker) {
        switch (stage) {
        case STAGE_HASHMOVE:
            stage++;

            if (hashMove && isLegalMove(searcher->board, hashMove)) {
                return hashMove;
            }
        case STAGE_GEN_NOISY:
        {
            nrNoisy = genLegalNoisy(searcher->board, noisy);

            int m = 0;

            for (int i = 0; i < nrNoisy; i++) {
                uint16_t move = noisy[i];

                if (move == hashMove || move == killer1 || move == killer2 || move == counter)
                    continue;

                noisy[m] = move;

                int p = searcher->board.piece_at(sqFrom(move)), cap = searcher->board.piece_type_at(sqTo(move));
                int score = 0; // so that move score isn't negative

                if (type(move) == ENPASSANT)
                    cap = PAWN;

                score = 10 * seeVal[cap];
                if (promoted(move) == QUEEN)
                    score += 10000;

                score += searcher->capHist[p][sqTo(move)][cap] + 100000;

                //score += searcher->nodesSearched[sqFrom(move)][sqTo(move)] / 10000;

                scores[m++] = score;
            }

            nrNoisy = m;

            sortMoves(nrNoisy, noisy, scores);

            index = 0;
            stage++;
        }
        case STAGE_GOOD_NOISY:
            if (index < nrNoisy) {
                while (index < nrNoisy) {
                    if (see(searcher->board, noisy[index], threshold))
                        return noisy[index++];
                    else {
                        badNoisy[nrBadNoisy++] = noisy[index++];
                    }
                }
            }
            if (skip) { /// no need to go through quiets
                stage = STAGE_BAD_NOISY;
                return nextMove(searcher, skip, noisyPicker);
            }
            stage++;

        case STAGE_KILLER_1:
            stage++;

            if (!skip && killer1 && isLegalMove(searcher->board, killer1))
                return killer1;
        case STAGE_KILLER_2:
            stage++;

            if (!skip && killer2 && isLegalMove(searcher->board, killer2))
                return killer2;
        case STAGE_COUNTER:
            stage++;

            if (!skip && counter && isLegalMove(searcher->board, counter))
                return counter;
        case STAGE_GEN_QUIETS:
        {
            nrQuiets = genLegalQuiets(searcher->board, quiets);

            int ply = searcher->board.ply;

            uint16_t counterMove = (ply >= 1 ? searcher->Stack[ply - 1].move : NULLMOVE), followMove = (ply >= 2 ? searcher->Stack[ply - 2].move : NULLMOVE);
            int counterPiece = (ply >= 1 ? searcher->Stack[ply - 1].piece : 0), followPiece = ply >= 2 ? searcher->Stack[ply - 2].piece : 0;
            int counterTo = sqTo(counterMove), followTo = sqTo(followMove);
            bool turn = searcher->board.turn;
            uint64_t pawnAttacks = getPawnAttacks(turn ^ 1, searcher->board.bb[getType(PAWN, turn ^ 1)]);
            int m = 0;

            for (int i = 0; i < nrQuiets; i++) {
                uint16_t move = quiets[i];

                if (move == hashMove || move == killer1 || move == killer2 || move == counter)
                    continue;

                quiets[m] = move;
                int score = 0;
                int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from);

                score = searcher->hist[searcher->board.turn][from][to];

                if (counterMove)
                    score += searcher->follow[0][counterPiece][counterTo][piece][to];

                if (followMove)
                    score += searcher->follow[1][followPiece][followTo][piece][to];

                if ((pawnAttacks & (1ULL << to)) && piece_type(piece) != PAWN)
                    score -= 10 * seeVal[piece_type(piece)];

                score += searcher->nodesSearched[from][to] / nodesSearchedDiv + 1000000; // the longer it takes a move to be refuted, the higher its chance to become the best move
                scores[m++] = score;
            }

            nrQuiets = m;

            sortMoves(nrQuiets, quiets, scores);

            index = 0;
            stage++;
        }
        case STAGE_QUIETS:
            if (!skip && index < nrQuiets) {
                return quiets[index++];
            }
            else {
                stage++;
            }
        case STAGE_BAD_NOISY:
            if (nrBadNoisy && !noisyPicker) { /// bad captures can't improve the current position in quiesce search
                nrBadNoisy--;
                uint16_t move = badNoisy[nrBadNoisy];

                return move;
            }
            else {
                return NULLMOVE;
            }
        default:
            assert(0);
        }

        assert(0);

        return NULLMOVE;

    }
};

bool see(Board& board, uint16_t move, int threshold) {
    int from = sqFrom(move), to = sqTo(move), t = type(move), col, nextVictim, score = -threshold;
    uint64_t diag, orth, occ, att, myAtt;

    nextVictim = (t != PROMOTION ? board.piece_type_at(from) : promoted(move) + KNIGHT);

    score += seeVal[board.piece_type_at(to)];

    if (t == PROMOTION)
        score += seeVal[promoted(move) + KNIGHT] - seeVal[PAWN];
    else if (t == ENPASSANT)
        score = seeVal[PAWN];

    if (score < 0)
        return 0;

    score -= seeVal[nextVictim];

    if (score >= 0)
        return 1;

    diag = board.diagSliders(WHITE) | board.diagSliders(BLACK);
    orth = board.orthSliders(WHITE) | board.orthSliders(BLACK);

    occ = board.pieces[WHITE] | board.pieces[BLACK];
    occ = (occ ^ (1ULL << from)) | (1ULL << to);

    if (t == ENPASSANT)
        occ ^= (1ULL << board.enPas);

    att = (getAttackers(board, WHITE, occ, to) | getAttackers(board, BLACK, occ, to)) & occ;

    col = 1 ^ board.turn;

    while (true) {
        myAtt = att & board.pieces[col];

        if (!myAtt)
            break;

        for (nextVictim = PAWN; nextVictim <= QUEEN; nextVictim++) {
            if (myAtt & board.bb[getType(nextVictim, col)])
                break;
        }

        occ ^= (1ULL << Sq(lsb(myAtt & board.bb[getType(nextVictim, col)])));

        if (nextVictim == PAWN || nextVictim == BISHOP || nextVictim == QUEEN)
            att |= genAttacksBishop(occ, to) & diag;

        if (nextVictim == ROOK || nextVictim == QUEEN)
            att |= genAttacksRook(occ, to) & orth;

        att &= occ;
        col ^= 1;

        score = -score - 1 - seeVal[nextVictim];

        if (score >= 0) {
            if (nextVictim == KING && (att & board.pieces[col]))
                col ^= 1;

            break;
        }
    }

    return board.turn != col;
}