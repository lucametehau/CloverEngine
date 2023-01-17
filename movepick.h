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
    STAGE_COUNTER, STAGE_KILLER_1,
    STAGE_GEN_QUIETS, STAGE_QUIETS, 
    STAGE_PRE_BAD_NOISY, STAGE_BAD_NOISY,
}; /// move picker stages

bool see(Board& board, uint16_t move, int threshold);

class Movepick {
public:
    int stage, trueStage;
    //int mp_type;

    uint16_t hashMove, killer1, counter, possibleCounter;
    int recapSq;
    int nrNoisy, nrQuiets, nrBadNoisy;
    int index;

    int threshold;

    uint16_t noisy[256], quiets[256], badNoisy[256];
    int scores[256];
    long long v[256];

    Movepick(const uint16_t HashMove, const uint16_t Killer1, const uint16_t Counter, const int Threshold, int RecaptureSq = -1) {
        stage = STAGE_HASHMOVE;

        hashMove = HashMove;
        killer1 = (Killer1 != hashMove ? Killer1 : NULLMOVE);
        counter = (Counter != hashMove && Counter != killer1 ? Counter : NULLMOVE);

        nrNoisy = nrQuiets = nrBadNoisy = 0;
        threshold = Threshold;
        recapSq = RecaptureSq;
    }

    long long codify(uint16_t move, int score) {
        return ((-1LL * score) << 16) | move;
    }

    void sortMoves(int nrMoves, uint16_t moves[], int scores[]) {
        for (int i = 0; i < nrMoves; i++)
            v[i] = codify(moves[i], scores[i]);

        std::sort(v, v + nrMoves);

        for (int i = 0; i < nrMoves; i++) {
            moves[i] = v[i] & 65535;
        }
    }

    uint16_t nextMove(Search* searcher, bool skip, bool noisyPicker) {
        switch (stage) {
        case STAGE_HASHMOVE:
            trueStage = STAGE_HASHMOVE;
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

                if (move == hashMove || move == killer1 || move == counter)
                    continue;

                noisy[m] = move;

                int p = searcher->board.piece_at(sqFrom(move)), cap = searcher->board.piece_type_at(sqTo(move)), to = sqTo(move);
                int score = 0; // so that move score isn't negative

                if (type(move) == ENPASSANT)
                    cap = PAWN;

                score = 10 * seeVal[cap];
                if (promoted(move) + KNIGHT == QUEEN)
                    score += 10000;

                if (to == recapSq)
                    score += 4096;

                score += searcher->capHist[p][to][cap] + 1000000;

                //score += searcher->nodesSearched[sqFrom(move)][sqTo(move)] / 10000;

                scores[m++] = score;
            }

            nrNoisy = m;
            sortMoves(nrNoisy, noisy, scores);
            index = 0;
            stage++;
        }
        case STAGE_GOOD_NOISY:
            trueStage = STAGE_GOOD_NOISY;
            while (index < nrNoisy) {
                if (see(searcher->board, noisy[index], threshold))
                    return noisy[index++];
                else {
                    badNoisy[nrBadNoisy++] = noisy[index++];
                }
            }
            if (skip) { /// no need to go through quiets
                stage = STAGE_PRE_BAD_NOISY;
                return nextMove(searcher, skip, noisyPicker);
            }
            stage++;
        case STAGE_COUNTER:
            trueStage = STAGE_COUNTER;
            stage++;

            if (!skip && counter && isLegalMove(searcher->board, counter))
                return counter;
        case STAGE_KILLER_1:
            trueStage = STAGE_KILLER_1;
            stage++;

            if (!skip && killer1 && isLegalMove(searcher->board, killer1))
                return killer1;
        case STAGE_GEN_QUIETS:
        {
            if (!skip) {
                nrQuiets = genLegalQuiets(searcher->board, quiets);

                int ply = searcher->board.ply;

                bool turn = searcher->board.turn, enemy = 1 ^ turn;
                uint64_t enemyPawns = searcher->board.bb[getType(PAWN, turn ^ 1)], allPieces = searcher->board.pieces[WHITE] | searcher->board.pieces[BLACK];
                uint64_t pawnAttacks = getPawnAttacks(enemy, enemyPawns);
                uint64_t enemyKingRing = kingRingMask[searcher->board.king(enemy)] & ~(shift(enemy, NORTHEAST, enemyPawns & ~fileMask[(enemy == WHITE ? 7 : 0)]) & shift(enemy, NORTHWEST, enemyPawns & ~fileMask[(enemy == WHITE ? 0 : 7)]));
                uint16_t counterMove = (ply >= 1 ? searcher->Stack[ply - 1].move : NULLMOVE), followMove = (ply >= 2 ? searcher->Stack[ply - 2].move : NULLMOVE);
                int counterPiece = (ply >= 1 ? searcher->Stack[ply - 1].piece : 0), followPiece = ply >= 2 ? searcher->Stack[ply - 2].piece : 0;
                int counterTo = sqTo(counterMove), followTo = sqTo(followMove);
                int m = 0;

                for (int i = 0; i < nrQuiets; i++) {
                    uint16_t move = quiets[i];

                    if (move == hashMove || move == killer1 || move == counter)
                        continue;

                    quiets[m] = move;
                    int score = 0;
                    int from = sqFrom(move), to = sqTo(move), piece = searcher->board.piece_at(from), pt = piece_type(piece);

                    score = searcher->hist[searcher->board.turn][from][to];

                    if (counterMove)
                        score += searcher->follow[0][counterPiece][counterTo][piece][to];

                    if (followMove)
                        score += searcher->follow[1][followPiece][followTo][piece][to];

                    if (pt != PAWN && (pawnAttacks & (1ULL << to)))
                        score -= 10 * seeVal[pt];

                    if (pt == PAWN) // pawn push, generally good?
                        score += 10000;

                    if(pt != KING && pt != PAWN)
                        score += 4096 * count(genAttacksSq(allPieces, to, pt) & enemyKingRing);

                    score += searcher->nodesSearched[from][to] / nodesSearchedDiv + 1000000; // the longer it takes a move to be refuted, the higher its chance to become the best move
                    scores[m++] = score;
                }

                nrQuiets = m;
                sortMoves(nrQuiets, quiets, scores);
                index = 0;
            }

            stage++;
        }
        case STAGE_QUIETS:
            trueStage = STAGE_QUIETS;
            if (!skip && index < nrQuiets) {
                return quiets[index++];
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

        occ ^= lsb(myAtt & board.bb[getType(nextVictim, col)]);

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