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
    STAGE_TTMOVE,
    STAGE_GEN_NOISY, STAGE_GOOD_NOISY,
    STAGE_KILLER,
    STAGE_GEN_QUIETS, STAGE_QUIETS,
    STAGE_PRE_BAD_NOISY, STAGE_BAD_NOISY,
}; /// move picker stages

bool see(Board& board, Move move, int threshold);

class Movepick {
public:
    int stage, trueStage;

private:
    Move ttMove, killer;
    int nrNoisy, nrQuiets, nrBadNoisy;
    int index;

    uint64_t threats_enemy;

    int threshold;

    std::array<Move, MAX_MOVES> moves, badNoisy;
    std::array<int, MAX_MOVES> scores;

public:
    Movepick(const Move _ttMove, const Move _killer, const int _threshold, const uint64_t _threats_enemy) {
        stage = STAGE_TTMOVE;

        ttMove = _ttMove;
        killer = (_killer != ttMove ? _killer : NULLMOVE);

        nrNoisy = nrQuiets = nrBadNoisy = 0;
        threshold = _threshold;
        threats_enemy = _threats_enemy;
    }

    void get_best_move(int offset, int nrMoves, std::array<Move, MAX_MOVES> &moves, std::array<int, MAX_MOVES> &scores) {
        int ind = offset;
        for (int i = offset + 1; i < nrMoves; i++) {
            if (scores[ind] < scores[i])
                ind = i;
        }
        std::swap(scores[ind], scores[offset]);
        std::swap(moves[ind], moves[offset]);
    }

    Move get_next_move(Histories &histories, StackEntry* stack, Board &board, bool skip, bool noisyPicker) {
        switch (stage) {
        case STAGE_TTMOVE:
            trueStage = STAGE_TTMOVE;
            stage++;

            if (ttMove && is_legal(board, ttMove)) {
                return ttMove;
            }
        case STAGE_GEN_NOISY:
        {
            nrNoisy = gen_legal_noisy_moves(board, moves);
            int m = 0;
            for (int i = 0; i < nrNoisy; i++) {
                const Move move = moves[i];
                if (move == ttMove || move == killer)
                    continue;

                if (type(move) == PROMOTION && promoted(move) + KNIGHT != QUEEN) {
                    badNoisy[nrBadNoisy++] = move;
                    continue;
                }
                moves[m] = move;

                const int piece = board.piece_at(sq_from(move));
                const int to = sq_to(move), cap = board.get_captured_type(move);
                int score = GoodNoisyValueCoef * seeVal[cap];
                if (type(move) == PROMOTION && piece_type(piece) >= ROOK)
                    score += GoodNoisyPromotionBonus;
                score += histories.get_cap_hist(piece, to, cap);
                scores[m++] = score;
            }

            nrNoisy = m;
            index = 0;
            stage++;
        }
        case STAGE_GOOD_NOISY:
            trueStage = STAGE_GOOD_NOISY;
            while (index < nrNoisy) {
                get_best_move(index, nrNoisy, moves, scores);
                if (see(board, moves[index], threshold))
                    return moves[index++];
                else {
                    badNoisy[nrBadNoisy++] = moves[index++];
                }
            }
            if (skip) { /// no need to go through quiets
                stage = STAGE_PRE_BAD_NOISY;
                return get_next_move(histories, stack, board, skip, noisyPicker);
            }
            stage++;
        case STAGE_KILLER:
            trueStage = STAGE_KILLER;
            stage++;

            if (!skip && killer && is_legal(board, killer))
                return killer;
        case STAGE_GEN_QUIETS:
        {
            if (!skip) {
                nrQuiets = gen_legal_quiet_moves(board, moves);
                const bool turn = board.turn, enemy = 1 ^ turn;
                const uint64_t enemyPawns = board.get_bb_piece(PAWN, enemy);
                const uint64_t allPieces = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
                const uint64_t pawnAttacks = getPawnAttacks(enemy, enemyPawns);
                const uint64_t enemyKingRing = kingRingMask[board.king(enemy)] & ~(shift(enemy, NORTHEAST, enemyPawns & ~file_mask[(enemy == WHITE ? 7 : 0)]) & shift(enemy, NORTHWEST, enemyPawns & ~file_mask[(enemy == WHITE ? 0 : 7)]));
                
                int m = 0;
                for (int i = 0; i < nrQuiets; i++) {
                    const Move move = moves[i];
                    if (move == ttMove || move == killer)
                        continue;

                    moves[m] = move;
                    const int to = sq_to(move), piece = board.piece_at(sq_from(move)), pt = piece_type(piece);
                    int score = histories.get_history_movepick(move, piece, threats_enemy, turn, stack);

                    if (pt != PAWN && (pawnAttacks & (1ULL << to)))
                        score -= QuietPawnAttackedCoef * seeVal[pt];

                    if (pt == PAWN) // pawn push, generally good?
                        score += QuietPawnPushBonus;

                    if (pt != KING && pt != PAWN)
                        score += QuietKingRingAttackBonus * count(genAttacksSq(allPieces, to, pt) & enemyKingRing);

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
                get_best_move(index, nrQuiets, moves, scores);
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
            if (index < nrBadNoisy) {
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

bool see(Board& board, Move move, int threshold) {
    int from = sq_from(move), to = sq_to(move), t = type(move), col, nextVictim, score = -threshold;
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

    diag = board.diagonal_sliders(WHITE) | board.diagonal_sliders(BLACK);
    orth = board.orthogonal_sliders(WHITE) | board.orthogonal_sliders(BLACK);

    occ = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
    occ = (occ ^ (1ULL << from)) | (1ULL << to);

    if (t == ENPASSANT)
        occ ^= (1ULL << board.enPas);

    att = board.get_attackers(WHITE, occ, to) | board.get_attackers(BLACK, occ, to);

    col = 1 ^ board.turn;

    while (true) {
        att &= occ;
        myAtt = att & board.get_bb_color(col);

        if (!myAtt)
            break;

        if (myAtt & board.get_bb_piece(PAWN, col)) {
            occ ^= lsb(myAtt & board.get_bb_piece(PAWN, col));
            att |= genAttacksBishop(occ, to) & diag;
            score = -score - 1 - seeVal[PAWN];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.get_bb_piece(KNIGHT, col)) {
            occ ^= lsb(myAtt & board.get_bb_piece(KNIGHT, col));
            score = -score - 1 - seeVal[KNIGHT];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.get_bb_piece(BISHOP, col)) {
            occ ^= lsb(myAtt & board.get_bb_piece(BISHOP, col));
            att |= genAttacksBishop(occ, to) & diag;
            score = -score - 1 - seeVal[BISHOP];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.get_bb_piece(ROOK, col)) {
            occ ^= lsb(myAtt & board.get_bb_piece(ROOK, col));
            att |= genAttacksRook(occ, to) & orth;
            score = -score - 1 - seeVal[ROOK];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else if (myAtt & board.get_bb_piece(QUEEN, col)) {
            occ ^= lsb(myAtt & board.get_bb_piece(QUEEN, col));
            att |= genAttacksBishop(occ, to) & diag;
            att |= genAttacksRook(occ, to) & orth;
            score = -score - 1 - seeVal[QUEEN];
            col ^= 1;
            if (score >= 0)
                break;
        }
        else {
            assert(myAtt & board.get_bb_piece(KING, col));
            occ ^= board.get_bb_piece(KING, col);
            col ^= 1;
            score = -score - 1 - seeVal[KING];
            if (score >= 0) {
                if (att & occ & board.get_bb_color(col))
                    col ^= 1;
                break;
            }
        }
    }

    return board.turn != col;
}