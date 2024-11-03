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

enum Stages : int {
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

    Bitboard all_threats, threats_p, threats_bn, threats_r;

    int threshold;

    MoveList moves, badNoisy;
    std::array<int, MAX_MOVES> scores;

public:
    Movepick(const Move _ttMove, const Move _killer, const int _threshold, const Threats threats) {
        stage = STAGE_TTMOVE;
        ttMove = _ttMove;
        killer = (_killer != ttMove ? _killer : NULLMOVE);
        nrNoisy = nrQuiets = nrBadNoisy = 0;
        threshold = _threshold;
        all_threats = threats.all_threats;
        threats_p = threats.threats_pieces[PieceTypes::PAWN];
        threats_bn = threats.threats_pieces[PieceTypes::KNIGHT] | threats.threats_pieces[PieceTypes::BISHOP] | threats_p;
        threats_r = threats.threats_pieces[PieceTypes::ROOK] | threats_bn;
    }

    void get_best_move(int offset, int nrMoves, MoveList &moves, std::array<int, MAX_MOVES> &scores) {
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
        case Stages::STAGE_TTMOVE:
            trueStage = Stages::STAGE_TTMOVE;
            stage++;

            if (ttMove && is_legal(board, ttMove)) {
                return ttMove;
            }
        case Stages::STAGE_GEN_NOISY:
        {
            nrNoisy = board.gen_legal_noisy_moves(moves);
            int m = 0;
            for (int i = 0; i < nrNoisy; i++) {
                const Move move = moves[i];
                if (move == ttMove || move == killer)
                    continue;

                if (type(move) == PROMOTION && promoted(move) + PieceTypes::KNIGHT != PieceTypes::QUEEN) {
                    badNoisy[nrBadNoisy++] = move;
                    continue;
                }
                moves[m] = move;

                const Piece piece = board.piece_at(sq_from(move)), cap = board.get_captured_type(move);
                const Square to = sq_to(move);
                int score = GoodNoisyValueCoef * seeVal[cap];
                score += histories.get_cap_hist(piece, to, cap);
                scores[m++] = score;
            }

            nrNoisy = m;
            index = 0;
            stage++;
        }
        case Stages::STAGE_GOOD_NOISY:
            trueStage = Stages::STAGE_GOOD_NOISY;
            while (index < nrNoisy) {
                get_best_move(index, nrNoisy, moves, scores);
                if (see(board, moves[index], threshold))
                    return moves[index++];
                else {
                    badNoisy[nrBadNoisy++] = moves[index++];
                }
            }
            if (skip) { /// no need to go through quiets
                stage = Stages::STAGE_PRE_BAD_NOISY;
                return get_next_move(histories, stack, board, skip, noisyPicker);
            }
            stage++;
        case Stages::STAGE_KILLER:
            trueStage = Stages::STAGE_KILLER;
            stage++;

            if (!skip && killer && is_legal(board, killer))
                return killer;
        case Stages::STAGE_GEN_QUIETS:
        {
            if (!skip) {
                nrQuiets = board.gen_legal_quiet_moves(moves);
                const bool turn = board.turn, enemy = 1 ^ turn;
                const Bitboard allPieces = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
                const Bitboard enemyKingRing = attacks::kingRingMask[board.get_king(enemy)];
                
                int m = 0;
                for (int i = 0; i < nrQuiets; i++) {
                    const Move move = moves[i];
                    if (move == ttMove || move == killer)
                        continue;

                    moves[m] = move;
                    const Square from = sq_from(move), to = sq_to(move);
                    const Piece piece = board.piece_at(from), pt = piece.type();
                    int score = histories.get_history_movepick(move, piece, all_threats, turn, stack);

                    if (pt == PieceTypes::PAWN) // pawn push, generally good?
                        score += QuietPawnPushBonus;

                    if (pt != PieceTypes::KING && pt != PieceTypes::PAWN) {
                        score += QuietKingRingAttackBonus * (attacks::genAttacksSq(allPieces, to, pt) & enemyKingRing).count();

                        auto score_threats = [&](
                            Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r, 
                            Piece pt, Square to
                        ) {
                            if (threats_p.has_square(to)) return QuietPawnAttackedCoef * seeVal[pt];
                            if (pt >= PieceTypes::ROOK && threats_bn.has_square(to)) return 16384;
                            if (pt == PieceTypes::QUEEN && threats_r.has_square(to)) return 16384;
                            return 0;
                        };

                        auto score_threats_dodged = [&](
                            Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r,
                            Piece pt, Square from
                        ) {
                            if (threats_p.has_square(from)) return QuietPawnAttackedDodgeCoef * seeVal[pt];
                            if (pt >= PieceTypes::ROOK && threats_bn.has_square(from)) return 16384;
                            if (pt == PieceTypes::QUEEN && threats_r.has_square(from)) return 16384;
                            return 0;
                        };

                        score += score_threats_dodged(threats_p, threats_bn, threats_r, pt, from) -
                                 score_threats(threats_p, threats_bn, threats_r, pt, to);
                    }

                    scores[m++] = score;
                }

                nrQuiets = m;
                index = 0;
            }

            stage++;
        }
        case Stages::STAGE_QUIETS:
            trueStage = Stages::STAGE_QUIETS;
            if (!skip && index < nrQuiets) {
                get_best_move(index, nrQuiets, moves, scores);
                return moves[index++];
            }
            else {
                stage++;
            }
        case Stages::STAGE_PRE_BAD_NOISY:
            if (noisyPicker) {
                return NULLMOVE;
            }
            index = 0;
            stage++;
        case Stages::STAGE_BAD_NOISY:
            trueStage = Stages::STAGE_BAD_NOISY;
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
    Square from = sq_from(move), to = sq_to(move);
    int score = -threshold + 1;
    Bitboard diag, orth, occ, att, myAtt, b;
    bool stm, result = 1;

    score += seeVal[board.get_captured_type(move)];

    if (score <= 0)
        return 0;

    score = seeVal[board.piece_type_at(from)] - score + 1;

    if (score <= 0)
        return 1;

    diag = board.diagonal_sliders(WHITE) | board.diagonal_sliders(BLACK);
    orth = board.orthogonal_sliders(WHITE) | board.orthogonal_sliders(BLACK);

    occ = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
    occ = (occ ^ Bitboard(from)) | Bitboard(to);

    if (type(move) == ENPASSANT && board.enpas() != NO_SQUARE) occ ^= Bitboard(board.enpas());

    att = board.get_attackers(WHITE, occ, to) | board.get_attackers(BLACK, occ, to);

    stm = board.turn;

    while (true) {
        stm ^= 1;
        att &= occ;
        myAtt = att & board.get_bb_color(stm);

        if (!myAtt)
            break;

        result ^= 1;

        if ((b = (myAtt & board.get_bb_piece(PieceTypes::PAWN, stm)))) {
            score = seeVal[PieceTypes::PAWN] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::KNIGHT, stm)))) {
            score = seeVal[PieceTypes::KNIGHT] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::BISHOP, stm)))) {
            score = seeVal[PieceTypes::BISHOP] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::ROOK, stm)))) {
            score = seeVal[PieceTypes::ROOK] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksRook(occ, to) & orth;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::QUEEN, stm)))) {
            score = seeVal[PieceTypes::QUEEN] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
            att |= attacks::genAttacksRook(occ, to) & orth;
        }
        else {
            assert(myAtt & board.get_bb_piece(PieceTypes::KING, stm));
            if (att & board.get_bb_color(1 ^ stm)) result ^= 1;
            break;
        }
    }

    return result;
}