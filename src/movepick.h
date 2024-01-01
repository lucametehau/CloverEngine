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

enum {
    STAGE_NONE = 0,
    STAGE_HASHMOVE,
    STAGE_GEN_NOISY, STAGE_GOOD_NOISY,
    STAGE_COUNTER, STAGE_KILLER,
    STAGE_GEN_QUIETS, STAGE_QUIETS,
    STAGE_PRE_BAD_NOISY, STAGE_BAD_NOISY,
}; /// move picker stages

bool see(Board& board, uint16_t move, int threshold);

class Movepick {
public:
    int stage, trueStage;
    //int mp_type;

    uint16_t hashMove, killer, counter, possibleCounter;
    int nrNoisy, nrQuiets, nrBadNoisy;
    int index;

    uint64_t threatsEnemy;

    int threshold;

    uint16_t moves[256], badNoisy[256];
    int scores[256];

    Movepick(const uint16_t HashMove, const uint16_t Killer, const uint16_t Counter, const int Threshold, const uint64_t ThreatsEnemy) {
        stage = STAGE_HASHMOVE;

        hashMove = HashMove;
        killer = (Killer != hashMove ? Killer : NULLMOVE);
        counter = (Counter != hashMove && Counter != killer ? Counter : NULLMOVE);

        nrNoisy = nrQuiets = nrBadNoisy = 0;
        threshold = Threshold;
        threatsEnemy = ThreatsEnemy;
    }

    void getBestMove(int offset, int nrMoves, uint16_t moves[], int scores[]) {
        int ind = offset;
        for (int i = offset + 1; i < nrMoves; i++) {
            if (scores[ind] < scores[i])
                ind = i;
        }
        std::swap(scores[ind], scores[offset]);
        std::swap(moves[ind], moves[offset]);
    }

    uint16_t nextMove(Search* searcher, StackEntry* stack, Board& board, bool skip, bool noisyPicker) {
        switch (stage) {
        case STAGE_HASHMOVE:
            trueStage = STAGE_HASHMOVE;
            stage++;

            if (hashMove && isLegalMove(board, hashMove)) {
                return hashMove;
            }
        case STAGE_GEN_NOISY:
        {
            nrNoisy = genLegalNoisy(board, moves);

            int m = 0;

            for (int i = 0; i < nrNoisy; i++) {
                const uint16_t move = moves[i];

                if (move == hashMove || move == killer || move == counter)
                    continue;

                if (type(move) == PROMOTION && promoted(move) + KNIGHT != QUEEN) {
                    badNoisy[nrBadNoisy++] = move;
                    continue;
                }

                moves[m] = move;

                const int p = board.piece_at(sqFrom(move)), cap = (type(move) == ENPASSANT ? PAWN : board.piece_type_at(sqTo(move))), to = sqTo(move);
                int score = 0;

                score = 10 * seeVal[cap];
                if (type(move) == PROMOTION)
                    score += 10000;

                score += searcher->capHist[p][to][cap];

                scores[m++] = score;
            }

            nrNoisy = m;
            index = 0;
            stage++;
        }
        case STAGE_GOOD_NOISY:
            trueStage = STAGE_GOOD_NOISY;
            while (index < nrNoisy) {
                getBestMove(index, nrNoisy, moves, scores);
                if (see(board, moves[index], threshold))
                    return moves[index++];
                else {
                    badNoisy[nrBadNoisy++] = moves[index++];
                }
            }
            if (skip) { /// no need to go through quiets
                stage = STAGE_PRE_BAD_NOISY;
                return nextMove(searcher, stack, board, skip, noisyPicker);
            }
            stage++;
        case STAGE_COUNTER:
            trueStage = STAGE_COUNTER;
            stage++;

            if (!skip && counter && isLegalMove(board, counter))
                return counter;
        case STAGE_KILLER:
            trueStage = STAGE_KILLER;
            stage++;

            if (!skip && killer && isLegalMove(board, killer))
                return killer;
        case STAGE_GEN_QUIETS:
        {
            if (!skip) {
                nrQuiets = genLegalQuiets(board, moves);
                const bool turn = board.turn, enemy = 1 ^ turn;
                const uint64_t enemyPawns = board.bb[getType(PAWN, turn ^ 1)], allPieces = board.pieces[WHITE] | board.pieces[BLACK];
                const uint64_t pawnAttacks = getPawnAttacks(enemy, enemyPawns);
                const uint64_t enemyKingRing = kingRingMask[board.king(enemy)] & ~(shift(enemy, NORTHEAST, enemyPawns & ~fileMask[(enemy == WHITE ? 7 : 0)]) & shift(enemy, NORTHWEST, enemyPawns & ~fileMask[(enemy == WHITE ? 0 : 7)]));
                int m = 0;

                for (int i = 0; i < nrQuiets; i++) {
                    const uint16_t move = moves[i];

                    if (move == hashMove || move == killer || move == counter)
                        continue;

                    moves[m] = move;
                    int score = 0;
                    const int from = sqFrom(move), to = sqTo(move), piece = board.piece_at(from), pt = piece_type(piece);

                    score = searcher->hist[board.turn][!!(threatsEnemy & (1ULL << from))][!!(threatsEnemy & (1ULL << to))][fromTo(move)];
                    score += (*(stack - 1)->continuationHist)[piece][to];
                    score += (*(stack - 2)->continuationHist)[piece][to];
                    score += (*(stack - 4)->continuationHist)[piece][to];

                    if (pt != PAWN && (pawnAttacks & (1ULL << to)))
                        score -= pawnAttackedCoef * seeVal[pt];

                    if (pt == PAWN) // pawn push, generally good?
                        score += pawnPushBonus;

                    if (pt != KING && pt != PAWN)
                        score += kingAttackBonus * count(genAttacksSq(allPieces, to, pt) & enemyKingRing);

                    scores[m++] = score;
                }

                nrQuiets = m;
                index = 0;
            }

            stage++;
        }
        case STAGE_QUIETS:
            trueStage = STAGE_QUIETS;
            if (!skip && index < nrQuiets) {
                getBestMove(index, nrQuiets, moves, scores);
                return moves[index++];
            }
            else {
                stage++;
            }
        case STAGE_PRE_BAD_NOISY:
            if (noisyPicker) {
                return NULLMOVE;
            }
            index = 0;
            stage++;
        case STAGE_BAD_NOISY:
            trueStage = STAGE_BAD_NOISY;
            if (index < nrBadNoisy) { /// bad captures can't improve the current position in quiesce search
                return badNoisy[index++];
            }
            return NULLMOVE;
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

    att = (getAttackers(board, WHITE, occ, to) | getAttackers(board, BLACK, occ, to));

    col = 1 ^ board.turn;

    while (true) {
        att &= occ;
        myAtt = att & board.pieces[col];

        if (!myAtt)
            break;

        if (myAtt & board.bb[getType(PAWN, col)]) {
            occ ^= lsb(myAtt & board.bb[getType(PAWN, col)]);
            att |= genAttacksBishop(occ, to) & diag;
            score = -score - 1 - seeVal[PAWN];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.bb[getType(KNIGHT, col)]) {
            occ ^= lsb(myAtt & board.bb[getType(KNIGHT, col)]);
            score = -score - 1 - seeVal[KNIGHT];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.bb[getType(BISHOP, col)]) {
            occ ^= lsb(myAtt & board.bb[getType(BISHOP, col)]);
            att |= genAttacksBishop(occ, to) & diag;
            score = -score - 1 - seeVal[BISHOP];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.bb[getType(ROOK, col)]) {
            occ ^= lsb(myAtt & board.bb[getType(ROOK, col)]);
            att |= genAttacksRook(occ, to) & orth;
            score = -score - 1 - seeVal[ROOK];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.bb[getType(QUEEN, col)]) {
            occ ^= lsb(myAtt & board.bb[getType(QUEEN, col)]);
            att |= genAttacksBishop(occ, to) & diag;
            att |= genAttacksRook(occ, to) & orth;
            score = -score - 1 - seeVal[QUEEN];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else {
            assert(myAtt & board.bb[getType(KING, col)]);
            occ ^= board.bb[getType(KING, col)];
            col ^= 1;
            score = -score - 1 - seeVal[KING];
            if (score >= 0) {
                if ((att & occ & board.pieces[col]))
                    col ^= 1;
                break;
            }
        }
    }

    return board.turn != col;
}