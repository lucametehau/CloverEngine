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
#include "board.h"
#include "defs.h"
#include "evaluate.h"
#include "history.h"
#include "movegen.h"
#include <cassert>

enum Stages : int
{
    STAGE_NONE = 0,
    STAGE_TTMOVE,
    STAGE_GEN_NOISY,
    STAGE_GOOD_NOISY,
    STAGE_KILLER,
    STAGE_GEN_QUIETS,
    STAGE_QUIETS,
    STAGE_PRE_BAD_NOISY,
    STAGE_BAD_NOISY,

    // QS stages
    STAGE_QS_TTMOVE,
    STAGE_QS_GEN_NOISY,
    STAGE_QS_NOISY,
    STAGE_QS_GEN_QUIETS,
    STAGE_QS_QUIETS,

    // Probcut stages
    STAGE_PC_TTMOVE,
    STAGE_PC_GEN_NOISY,
    STAGE_PC_NOISY,
}; /// move picker stages

bool see(Board &board, Move move, int threshold);

class Movepick
{
  public:
    int stage, trueStage;

  private:
    Move tt_move, killer, kp_move;
    int nrNoisy, nrQuiets, nrBadNoisy;
    int index;

    Bitboard all_threats, threats_p, threats_bn, threats_r;
    int threshold;

    MoveList moves, badNoisy;
    std::array<int, MAX_MOVES> scores;

    bool skip_quiets_flag;

#ifdef GENERATE
    MoveList all_moves;
    int nr_all;
#endif

  public:
    // Normal Movepicker, used in PVS search.
    Movepick(const Move tt_move, const Move killer, const Move kp_move, const int threshold, const Threats threats)
        : tt_move(tt_move), killer(killer != tt_move ? killer : NULLMOVE),
          kp_move(kp_move != killer && kp_move != tt_move ? kp_move : NULLMOVE), threshold(threshold)
    {
        stage = STAGE_TTMOVE;
        nrNoisy = nrQuiets = nrBadNoisy = 0;
        all_threats = threats.all_threats;
        threats_p = threats.threats_pieces[PieceTypes::PAWN];
        threats_bn =
            threats.threats_pieces[PieceTypes::KNIGHT] | threats.threats_pieces[PieceTypes::BISHOP] | threats_p;
        threats_r = threats.threats_pieces[PieceTypes::ROOK] | threats_bn;
        skip_quiets_flag = false;
    }

    // QS Movepicker
    Movepick(const Move tt_move, const Threats threats, const bool in_check) : tt_move(tt_move)
    {
        stage = STAGE_QS_TTMOVE + !tt_move;
        nrNoisy = nrQuiets = 0;
        all_threats = threats.all_threats;
        threats_p = threats.threats_pieces[PieceTypes::PAWN];
        threats_bn =
            threats.threats_pieces[PieceTypes::KNIGHT] | threats.threats_pieces[PieceTypes::BISHOP] | threats_p;
        threats_r = threats.threats_pieces[PieceTypes::ROOK] | threats_bn;
        skip_quiets_flag = !in_check;
    }

    // Probcut Movepicker
    Movepick(const Move tt_move, const int threshold, const Threats threats) : tt_move(tt_move), threshold(threshold)
    {
        stage = STAGE_PC_TTMOVE;
        nrNoisy = 0;
        all_threats = threats.all_threats;
        threats_p = threats.threats_pieces[PieceTypes::PAWN];
        threats_bn =
            threats.threats_pieces[PieceTypes::KNIGHT] | threats.threats_pieces[PieceTypes::BISHOP] | threats_p;
        threats_r = threats.threats_pieces[PieceTypes::ROOK] | threats_bn;
        skip_quiets_flag = true;
    }

    void skip_quiets()
    {
        skip_quiets_flag = true;
    }

    void get_best_move(int offset, int nrMoves, MoveList &moves, std::array<int, MAX_MOVES> &scores)
    {
        int ind = offset;
        for (int i = offset + 1; i < nrMoves; i++)
        {
            if (scores[ind] < scores[i])
                ind = i;
        }
        std::swap(scores[ind], scores[offset]);
        std::swap(moves[ind], moves[offset]);
    }

    Move get_next_move(Histories &histories, StackEntry *stack, Board &board)
    {
#ifdef GENERATE
        auto is_move_legal = [&](Move move) {
            for (int i = 0; i < nr_all; i++)
            {
                if (all_moves[i] == move)
                    return true;
            }
            return false;
        };
#endif

        switch (stage)
        {
        case Stages::STAGE_TTMOVE: {
            trueStage = Stages::STAGE_TTMOVE;
            stage++;
#ifdef GENERATE
            nr_all = board.gen_legal_moves<MOVEGEN_ALL>(all_moves);

            bool legal1 = is_legal(board, tt_move), legal2 = is_move_legal(tt_move);

            if (legal1 != legal2)
            {
                std::cout << legal1 << " " << legal2 << "\n";
                std::cout << tt_move.to_string(board.chess960) << "\n";
                board.print();
                for (int i = 0; i < 12; i++)
                    board.bb[i].print();
                board.pieces[WHITE].print();
                board.pieces[BLACK].print();
                for (int i = 0; i < nr_all; i++)
                    std::cout << all_moves[i].to_string(board.chess960) << " ";
                std::cout << "\n";
                exit(0);
            }

            if (legal1)
                return tt_move;
#else
            if (is_legal(board, tt_move))
                return tt_move;
#endif
        }
        case Stages::STAGE_GEN_NOISY: {
            nrNoisy = board.gen_legal_moves<MOVEGEN_NOISY>(moves);
            int m = 0;
            for (int i = 0; i < nrNoisy; i++)
            {
                const Move move = moves[i];
                if (move == tt_move || move == killer)
                    continue;

                if (move.is_promo() && move.get_prom() + PieceTypes::KNIGHT != PieceTypes::QUEEN)
                {
                    badNoisy[nrBadNoisy++] = move;
                    continue;
                }
                moves[m] = move;

                const Piece piece = board.piece_at(move.get_from()), cap = board.get_captured_type(move);
                const Square to = move.get_to();
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
            while (index < nrNoisy)
            {
                get_best_move(index, nrNoisy, moves, scores);
                if (see(board, moves[index], threshold))
                    return moves[index++];
                else
                {
                    badNoisy[nrBadNoisy++] = moves[index++];
                }
            }
            if (skip_quiets_flag)
            {
                stage = Stages::STAGE_PRE_BAD_NOISY;
                return get_next_move(histories, stack, board);
            }
            stage++;
        case Stages::STAGE_KILLER: {
            trueStage = Stages::STAGE_KILLER;
            stage++;

#ifdef GENERATE
            bool legal1 = is_legal(board, killer), legal2 = is_move_legal(killer);

            if (legal1 != legal2)
            {
                std::cout << legal1 << " " << legal2 << "\n";
                std::cout << killer.to_string(board.chess960) << "\n";
                board.print();
                for (int i = 0; i < 12; i++)
                    board.bb[i].print();
                board.pieces[WHITE].print();
                board.pieces[BLACK].print();
                for (int i = 0; i < nr_all; i++)
                    std::cout << all_moves[i].to_string(board.chess960) << " ";
                std::cout << "\n";
                exit(0);
            }

            if (!skip_quiets_flag && legal1)
                return killer;
#else
            if (!skip_quiets_flag && is_legal(board, killer))
                return killer;
#endif
        }
        case Stages::STAGE_GEN_QUIETS: {
            if (!skip_quiets_flag)
            {
                nrQuiets = board.gen_legal_moves<MOVEGEN_QUIET>(moves);
                const bool turn = board.turn, enemy = 1 ^ turn;
                const Bitboard allPieces = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
                const Bitboard enemyKingRing = attacks::kingRingMask[board.get_king(enemy)];
                const Key pawn_key = board.pawn_key();

                int m = 0;
                for (int i = 0; i < nrQuiets; i++)
                {
                    Move move = moves[i];
                    if (move == tt_move || move == killer)
                        continue;

                    moves[m] = move;
                    const Square from = move.get_from(), to = move.get_to();
                    const Piece piece = board.piece_at(from), pt = piece.type();
                    int score = histories.get_history_movepick(move, piece, all_threats, turn, stack, pawn_key);

                    score += QuietPawnPushBonus * (pt == PieceTypes::PAWN); // pawn push, generally good

                    if (pt != PieceTypes::KING && pt != PieceTypes::PAWN)
                    {
                        [[unlikely]] if (pt < 0 || pt > PieceTypes::QUEEN)
                        {
                            board.print();
                            for (int i = 0; i < 12; i++)
                                board.bb[i].print();
                            board.pieces[WHITE].print();
                            board.pieces[BLACK].print();
                            for (int i = 0; i <= board.ply; i++)
                                std::cerr << "i: " << i << " " << (stack - i)->move.to_string(board.chess960) << "\n";
                            std::cerr << "tried move " << move.to_string(board.chess960) << "\n";
                            assert(0);
                        }
                        score += QuietKingRingAttackBonus *
                                 (attacks::genAttacksSq(allPieces, to, pt) & enemyKingRing).count();

                        auto score_threats = [&](Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r, Piece pt,
                                                 Square to) -> int {
                            if (threats_p.has_square(to))
                                return QuietPawnAttackedCoef * seeVal[pt];
                            if (pt >= PieceTypes::ROOK && threats_bn.has_square(to))
                                return ThreatCoef1;
                            if (pt == PieceTypes::QUEEN && threats_r.has_square(to))
                                return ThreatCoef2;
                            return 0;
                        };

                        auto score_threats_dodged = [&](Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r,
                                                        Piece pt, Square from) -> int {
                            if (threats_p.has_square(from))
                                return QuietPawnAttackedDodgeCoef * seeVal[pt];
                            if (pt >= PieceTypes::ROOK && threats_bn.has_square(from))
                                return ThreatCoef3;
                            if (pt == PieceTypes::QUEEN && threats_r.has_square(from))
                                return ThreatCoef4;
                            return 0;
                        };

                        score += score_threats_dodged(threats_p, threats_bn, threats_r, pt, from) -
                                 score_threats(threats_p, threats_bn, threats_r, pt, to);
                    }

                    score += KPMoveBonus * (move == kp_move); // KP move bonus
                    scores[m++] = score;
                }

                nrQuiets = m;
                index = 0;
            }

            stage++;
        }
        case Stages::STAGE_QUIETS: {
            trueStage = Stages::STAGE_QUIETS;
            if (!skip_quiets_flag && index < nrQuiets)
            {
                get_best_move(index, nrQuiets, moves, scores);
                return moves[index++];
            }
            else
            {
                stage++;
            }
        }
        case Stages::STAGE_PRE_BAD_NOISY: {
            index = 0;
            stage++;
        }
        case Stages::STAGE_BAD_NOISY: {
            trueStage = Stages::STAGE_BAD_NOISY;
            // don't sort bad noisies
            [[likely]] if (index < nrBadNoisy)
                return badNoisy[index++];
            return NULLMOVE;
        }

        // Here begin the QS stages.
        case Stages::STAGE_QS_TTMOVE: {
            stage++;
#ifdef GENERATE
            nr_all = board.gen_legal_moves<MOVEGEN_ALL>(all_moves);

            bool legal1 = is_legal(board, tt_move), legal2 = is_move_legal(tt_move);

            if (legal1 != legal2)
            {
                std::cout << legal1 << " " << legal2 << "\n";
                std::cout << tt_move.to_string(board.chess960) << "\n";
                board.print();
                for (int i = 0; i < 12; i++)
                    board.bb[i].print();
                board.pieces[WHITE].print();
                board.pieces[BLACK].print();
                for (int i = 0; i < nr_all; i++)
                    std::cout << all_moves[i].to_string(board.chess960) << " ";
                std::cout << "\n";
                exit(0);
            }

            if (legal1)
                return tt_move;
#else
            if (is_legal(board, tt_move))
                return tt_move;
#endif
        }
        case Stages::STAGE_QS_GEN_NOISY: {
            nrNoisy = board.gen_legal_moves<MOVEGEN_NOISY>(moves);
            int m = 0;
            for (int i = 0; i < nrNoisy; i++)
            {
                const Move move = moves[i];
                if (move == tt_move)
                    continue;

                if (move.is_promo() && move.get_prom() + PieceTypes::KNIGHT != PieceTypes::QUEEN)
                    continue;

                moves[m] = move;

                const Piece piece = board.piece_at(move.get_from()), cap = board.get_captured_type(move);
                const Square to = move.get_to();
                int score = GoodNoisyValueCoef * seeVal[cap];
                score += histories.get_cap_hist(piece, to, cap);
                scores[m++] = score;
            }

            nrNoisy = m;
            index = 0;
            stage++;
        }
        case Stages::STAGE_QS_NOISY: {
            if (index < nrNoisy)
            {
                get_best_move(index, nrNoisy, moves, scores);
                return moves[index++];
            }
            // we are done with noisies
            if (skip_quiets_flag)
                return NULLMOVE;
            stage++;
        }
        case Stages::STAGE_QS_GEN_QUIETS: {
            nrQuiets = board.gen_legal_moves<MOVEGEN_QUIET>(moves);
            const bool turn = board.turn, enemy = 1 ^ turn;
            const Bitboard allPieces = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
            const Bitboard enemyKingRing = attacks::kingRingMask[board.get_king(enemy)];
            const Key pawn_key = board.pawn_key();

            int m = 0;
            for (int i = 0; i < nrQuiets; i++)
            {
                Move move = moves[i];
                if (move == tt_move)
                    continue;

                moves[m] = move;
                const Square from = move.get_from(), to = move.get_to();
                const Piece piece = board.piece_at(from), pt = piece.type();
                int score = histories.get_history_movepick(move, piece, all_threats, turn, stack, pawn_key);

                score += QuietPawnPushBonus * (pt == PieceTypes::PAWN); // pawn push, generally good

                if (pt != PieceTypes::KING && pt != PieceTypes::PAWN)
                {
                    [[unlikely]] if (pt < 0 || pt > PieceTypes::QUEEN)
                    {
                        board.print();
                        for (int i = 0; i < 12; i++)
                            board.bb[i].print();
                        board.pieces[WHITE].print();
                        board.pieces[BLACK].print();
                        for (int i = 0; i <= board.ply; i++)
                            std::cerr << "i: " << i << " " << (stack - i)->move.to_string(board.chess960) << "\n";
                        std::cerr << "tried move " << move.to_string(board.chess960) << "\n";
                        assert(0);
                    }
                    score +=
                        QuietKingRingAttackBonus * (attacks::genAttacksSq(allPieces, to, pt) & enemyKingRing).count();

                    auto score_threats = [&](Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r, Piece pt,
                                             Square to) -> int {
                        if (threats_p.has_square(to))
                            return QuietPawnAttackedCoef * seeVal[pt];
                        if (pt >= PieceTypes::ROOK && threats_bn.has_square(to))
                            return ThreatCoef1;
                        if (pt == PieceTypes::QUEEN && threats_r.has_square(to))
                            return ThreatCoef2;
                        return 0;
                    };

                    auto score_threats_dodged = [&](Bitboard threats_p, Bitboard threats_bn, Bitboard threats_r,
                                                    Piece pt, Square from) -> int {
                        if (threats_p.has_square(from))
                            return QuietPawnAttackedDodgeCoef * seeVal[pt];
                        if (pt >= PieceTypes::ROOK && threats_bn.has_square(from))
                            return ThreatCoef3;
                        if (pt == PieceTypes::QUEEN && threats_r.has_square(from))
                            return ThreatCoef4;
                        return 0;
                    };

                    score += score_threats_dodged(threats_p, threats_bn, threats_r, pt, from) -
                             score_threats(threats_p, threats_bn, threats_r, pt, to);
                }

                scores[m++] = score;
            }

            nrQuiets = m;
            index = 0;

            stage++;
        }
        case Stages::STAGE_QS_QUIETS: {
            [[likely]] if (index < nrQuiets)
            {
                get_best_move(index, nrQuiets, moves, scores);
                return moves[index++];
            }

            return NULLMOVE;
        }

        // Here begin the Probcut stages.
        case Stages::STAGE_PC_TTMOVE: {
            stage++;
#ifdef GENERATE
            nr_all = board.gen_legal_moves<MOVEGEN_ALL>(all_moves);

            bool legal1 = is_legal(board, tt_move), legal2 = is_move_legal(tt_move);

            if (legal1 != legal2)
            {
                std::cout << legal1 << " " << legal2 << "\n";
                std::cout << tt_move.to_string(board.chess960) << "\n";
                board.print();
                for (int i = 0; i < 12; i++)
                    board.bb[i].print();
                board.pieces[WHITE].print();
                board.pieces[BLACK].print();
                for (int i = 0; i < nr_all; i++)
                    std::cout << all_moves[i].to_string(board.chess960) << " ";
                std::cout << "\n";
                exit(0);
            }

            if (legal1)
                return tt_move;
#else
            if (is_legal(board, tt_move))
                return tt_move;
#endif
        }
        case Stages::STAGE_PC_GEN_NOISY: {
            nrNoisy = board.gen_legal_moves<MOVEGEN_NOISY>(moves);
            int m = 0;
            for (int i = 0; i < nrNoisy; i++)
            {
                const Move move = moves[i];
                if (move == tt_move)
                    continue;

                if (move.is_promo() && move.get_prom() + PieceTypes::KNIGHT != PieceTypes::QUEEN)
                    continue;

                moves[m] = move;

                const Piece piece = board.piece_at(move.get_from()), cap = board.get_captured_type(move);
                const Square to = move.get_to();
                int score = GoodNoisyValueCoef * seeVal[cap];
                score += histories.get_cap_hist(piece, to, cap);
                scores[m++] = score;
            }

            nrNoisy = m;
            index = 0;
            stage++;
        }
        case Stages::STAGE_PC_NOISY: {
            while (index < nrNoisy)
            {
                get_best_move(index, nrNoisy, moves, scores);
                if (see(board, moves[index], threshold))
                    return moves[index++];
                else
                    index++;
            }

            return NULLMOVE;
        }
        default:
            assert(0);
        }

        return NULLMOVE;
    }
};

bool see(Board &board, Move move, int threshold)
{
    Square from = move.get_from(), to = move.get_to();
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

    if (move.get_type() == MoveTypes::ENPASSANT && board.enpas() != NO_SQUARE)
        occ ^= Bitboard(board.enpas());

    att = board.get_attackers(WHITE, occ, to) | board.get_attackers(BLACK, occ, to);

    stm = board.turn;

    while (true)
    {
        stm ^= 1;
        att &= occ;
        myAtt = att & board.get_bb_color(stm);

        if (!myAtt)
            break;

        result ^= 1;

        if ((b = (myAtt & board.get_bb_piece(PieceTypes::PAWN, stm))))
        {
            score = seeVal[PieceTypes::PAWN] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::KNIGHT, stm))))
        {
            score = seeVal[PieceTypes::KNIGHT] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::BISHOP, stm))))
        {
            score = seeVal[PieceTypes::BISHOP] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::ROOK, stm))))
        {
            score = seeVal[PieceTypes::ROOK] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksRook(occ, to) & orth;
        }
        else if ((b = (myAtt & board.get_bb_piece(PieceTypes::QUEEN, stm))))
        {
            score = seeVal[PieceTypes::QUEEN] + 1 - score;
            if (score <= 0)
                break;
            occ ^= b.lsb();
            att |= attacks::genAttacksBishop(occ, to) & diag;
            att |= attacks::genAttacksRook(occ, to) & orth;
        }
        else
        {
            assert(myAtt & board.get_bb_piece(PieceTypes::KING, stm));
            if (att & board.get_bb_color(1 ^ stm))
                result ^= 1;
            break;
        }
    }

    return result;
}