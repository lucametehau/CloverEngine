#pragma once

#include "attacks.h"
#include "board.h"
#include "incbin.h"
#include "net.h"
#include <algorithm>
#include <cmath>

constexpr int POLICY_INPUT_NEURONS = 768;
constexpr int POLICY_SIDE_NEURONS = 128;
constexpr int POLICY_OUTPUT_NEURONS = 1880;
constexpr int POLICY_HIDDEN_NEURONS = POLICY_SIDE_NEURONS * 2;
constexpr int POLICY_NUM_REGS = POLICY_SIDE_NEURONS / REG_LENGTH;
constexpr int POLICY_UNROLL_LENGTH = BUCKET_UNROLL / REG_LENGTH;

constexpr int POLICY_Q_IN = 255;
constexpr int POLICY_Q_OUT = 64;

constexpr std::array<int, 65> OFFSETS = {0,    23,   47,   72,   97,   122,  147,  171,  194,  218,  245,  274,  303,
                                         332,  361,  388,  412,  437,  466,  499,  532,  565,  598,  627,  652,  677,
                                         706,  739,  774,  809,  842,  871,  896,  921,  950,  983,  1018, 1053, 1086,
                                         1115, 1140, 1165, 1194, 1227, 1260, 1293, 1326, 1355, 1380, 1404, 1431, 1460,
                                         1489, 1518, 1547, 1574, 1598, 1621, 1645, 1670, 1695, 1720, 1745, 1769, 1792};

constexpr std::array<Bitboard, 64> ALL_DESTINATIONS = {
    9313761861428512766ull,  180779649147539453ull,   289501704257216507ull,   578721933554499319ull,
    1157442771892337903ull,  2314886639001336031ull,  4630054752962539711ull,  9332167099946164351ull,
    4693051017167109639ull,  9386102034350996751ull,  325459995009219359ull,   578862400275412798ull,
    1157444425085677436ull,  2315169225636372472ull,  4702396040998862832ull,  9404792077685915616ull,
    2382695603659212551ull,  4765391211613458191ull,  9530782427521949471ull,  614821880829263422ull,
    1157867642580597884ull,  2387511404205701368ull,  4775021704588030192ull,  9550042305352687840ull,
    1227520104343209737ull,  2455041308214890258ull,  4910083715958251300ull,  9820448902615023177ull,
    1266211321280232594ull,  2460365044244346916ull,  4920447509705322568ull,  9840612440627273872ull,
    650497458799315217ull,   1301276396887151138ull,  2602834273062822980ull,  5277725044946913672ull,
    10555448994677166609ull, 2664152820428055586ull,  5255965472313067588ull,  10439590776083091592ull,
    506652789238731041ull,   1085364276338762306ull,  2242787250538824836ull,  4485294125612632072ull,
    8970307875760115984ull,  17940335376038371873ull, 17361587605057586242ull, 16204092063096014980ull,
    575905529148285249ull,   1152093637079876226ull,  2304469852943057924ull,  4537163586841610248ull,
    9002551054605291536ull,  17933325985854398752ull, 17347849204449690177ull, 16176895641640273026ull,
    18304606945994162561ull, 18234809986805039618ull, 18095216068426728452ull, 17815745661460023304ull,
    17256804838970232848ull, 16138922098757279776ull, 13830818648828297536ull, 9214611748970332801ull};

INCBIN(Policy, POLICY_FILE);

alignas(ALIGN) int16_t policy_input_weights[POLICY_SIDE_NEURONS * POLICY_INPUT_NEURONS];
alignas(ALIGN) int16_t policy_input_biases[POLICY_SIDE_NEURONS];
alignas(ALIGN) int16_t policy_output_weights[POLICY_HIDDEN_NEURONS * POLICY_OUTPUT_NEURONS];
alignas(ALIGN) int16_t policy_output_biases[POLICY_OUTPUT_NEURONS];

int policy_move_index(bool color, Move move)
{
    if (move.is_promo())
    {
        int id = 2 * (move.get_from() & 7) + (move.get_to() & 7);
        return OFFSETS[64] + 22 * move.get_prom() + id;
    }

    return OFFSETS[move.get_from().mirror(color)] +
           (ALL_DESTINATIONS[move.get_from().mirror(color)] & ((1ull << move.get_to().mirror(color)) - 1)).count();
}

int policy_input_index(Piece piece, Square sq, bool side)
{
    return 64 * (piece + side * (piece >= 6 ? -6 : +6)) + sq.mirror(side);
}

void init_policy_network()
{
    int16_t *int_data = (int16_t *)gPolicyData;

    for (int i = 0; i < POLICY_SIDE_NEURONS * POLICY_INPUT_NEURONS; i++)
    {
        policy_input_weights[i] = *int_data;
        int_data++;
    }
    for (int j = 0; j < POLICY_SIDE_NEURONS; j++)
    {
        policy_input_biases[j] = *int_data;
        int_data++;
    }
    for (int j = 0; j < POLICY_HIDDEN_NEURONS * POLICY_OUTPUT_NEURONS; j++)
    {
        policy_output_weights[j] = *int_data;
        int_data++;
    }
    for (int j = 0; j < POLICY_OUTPUT_NEURONS; j++)
    {
        policy_output_biases[j] = *int_data;
        int_data++;
    }
}

class PolicyNetwork
{
  public:
    PolicyNetwork() : hist_size(1)
    {
    }

    void init(Board &board)
    {
        hist_size = 1;
        for (int i = 0; i < POLICY_SIDE_NEURONS; i++)
        {
            for (auto color : {WHITE, BLACK})
            {
                output_history[0][i + POLICY_SIDE_NEURONS * color] = policy_input_biases[i];
                for (auto c : {WHITE, BLACK})
                {
                    for (Piece pt = PieceTypes::PAWN; pt <= PieceTypes::KING; pt++)
                    {
                        Bitboard bb = board.get_bb_piece(pt, c);
                        Piece piece = Piece(pt, c);
                        while (bb)
                        {
                            int input_index = policy_input_index(piece, bb.get_lsb_square(), color);
                            output_history[0][i + POLICY_SIDE_NEURONS * color] +=
                                policy_input_weights[input_index * POLICY_SIDE_NEURONS + i];
                            bb ^= bb.lsb();
                        }
                    }
                }
            }
        }
        hist[0].calc[0] = hist[0].calc[1] = 1;
    }

    void apply_sub_add(int16_t *output, int16_t *input, int ind1, int ind2)
    {
        reg_type regs[POLICY_UNROLL_LENGTH];
        const int16_t *inputWeights1 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind1 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights2 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind2 * POLICY_SIDE_NEURONS]);

        for (int b = 0; b < POLICY_SIDE_NEURONS / BUCKET_UNROLL; b++)
        {
            const int offset = b * BUCKET_UNROLL;
            const reg_type *reg_in = reinterpret_cast<const reg_type *>(&input[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type *reg1 = reinterpret_cast<const reg_type *>(&inputWeights1[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type *reg2 = reinterpret_cast<const reg_type *>(&inputWeights2[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);

            reg_type *reg_out = (reg_type *)&output[offset];
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void apply_sub_add_sub(int16_t *output, int16_t *input, int ind1, int ind2, int ind3)
    {
        reg_type regs[POLICY_UNROLL_LENGTH];
        const int16_t *inputWeights1 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind1 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights2 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind2 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights3 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind3 * POLICY_SIDE_NEURONS]);

        for (int b = 0; b < POLICY_SIDE_NEURONS / BUCKET_UNROLL; b++)
        {
            const int offset = b * BUCKET_UNROLL;
            const reg_type *reg_in = reinterpret_cast<const reg_type *>(&input[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type *reg1 = reinterpret_cast<const reg_type *>(&inputWeights1[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type *reg2 = reinterpret_cast<const reg_type *>(&inputWeights2[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);
            const reg_type *reg3 = reinterpret_cast<const reg_type *>(&inputWeights3[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg3[i]);

            reg_type *reg_out = (reg_type *)&output[offset];
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void apply_sub_add_sub_add(int16_t *output, int16_t *input, int ind1, int ind2, int ind3, int ind4)
    {
        reg_type regs[POLICY_UNROLL_LENGTH];
        const int16_t *inputWeights1 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind1 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights2 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind2 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights3 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind3 * POLICY_SIDE_NEURONS]);
        const int16_t *inputWeights4 =
            reinterpret_cast<const int16_t *>(&policy_input_weights[ind4 * POLICY_SIDE_NEURONS]);

        for (int b = 0; b < POLICY_SIDE_NEURONS / BUCKET_UNROLL; b++)
        {
            const int offset = b * BUCKET_UNROLL;
            const reg_type *reg_in = reinterpret_cast<const reg_type *>(&input[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_load(&reg_in[i]);

            const reg_type *reg1 = reinterpret_cast<const reg_type *>(&inputWeights1[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg1[i]);
            const reg_type *reg2 = reinterpret_cast<const reg_type *>(&inputWeights2[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg2[i]);
            const reg_type *reg3 = reinterpret_cast<const reg_type *>(&inputWeights3[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg3[i]);
            const reg_type *reg4 = reinterpret_cast<const reg_type *>(&inputWeights4[offset]);
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg4[i]);

            reg_type *reg_out = (reg_type *)&output[offset];
            for (int i = 0; i < POLICY_UNROLL_LENGTH; i++)
                reg_save(&reg_out[i], regs[i]);
        }
    }

    void process_move(Move move, Piece piece, Piece captured, bool side, int16_t *a, int16_t *b)
    {
        Square from = move.get_from(), to = move.get_to();
        const bool turn = piece.color();
        switch (move.get_type())
        {
        case NO_TYPE: {
            if (captured == NO_PIECE)
                apply_sub_add(a, b, policy_input_index(piece, from, side), policy_input_index(piece, to, side));
            else
                apply_sub_add_sub(a, b, policy_input_index(piece, from, side), policy_input_index(piece, to, side),
                                  policy_input_index(captured, to, side));
        }
        break;
        case MoveTypes::CASTLE: {
            Square rFrom = to, rTo;
            Piece rPiece(PieceTypes::ROOK, turn);
            if (to > from)
            { // king side castle
                to = Squares::G1.mirror(turn);
                rTo = Squares::F1.mirror(turn);
            }
            else
            { // queen side castle
                to = Squares::C1.mirror(turn);
                rTo = Squares::D1.mirror(turn);
            }
            apply_sub_add_sub_add(a, b, policy_input_index(piece, from, side), policy_input_index(piece, to, side),
                                  policy_input_index(rPiece, rFrom, side), policy_input_index(rPiece, rTo, side));
        }
        break;
        case MoveTypes::ENPASSANT: {
            const Square pos = shift_square<SOUTH>(turn, to);
            const Piece pieceCap(PieceTypes::PAWN, 1 ^ turn);
            apply_sub_add_sub(a, b, policy_input_index(piece, from, side), policy_input_index(piece, to, side),
                              policy_input_index(pieceCap, pos, side));
        }
        break;
        default: {
            const Piece promPiece(move.get_prom() + PieceTypes::KNIGHT, turn);
            if (captured == NO_PIECE)
                apply_sub_add(a, b, policy_input_index(piece, from, side), policy_input_index(promPiece, to, side));
            else
                apply_sub_add_sub(a, b, policy_input_index(piece, from, side), policy_input_index(promPiece, to, side),
                                  policy_input_index(captured, to, side));
        }
        break;
        }
    }

    void process_historic_update(const int index, const bool side)
    {
        hist[index].calc[side] = 1;
        process_move(hist[index].move, hist[index].piece, hist[index].captured, side,
                     &output_history[index][side * POLICY_SIDE_NEURONS],
                     &output_history[index - 1][side * POLICY_SIDE_NEURONS]);
    }

    void add_move_to_history(Move move, Piece piece, Piece captured)
    {
        hist[hist_size] = {move, piece, captured, {0, 0}};
        // std::cout << "add " << move.to_string() << "\n";
        hist_size++;
    }

    void revert_move()
    {
        hist_size--;
        // std::cout << "revert\n";
    }

    int get_computed_parent(const bool c)
    {
        int i = hist_size - 1;
        while (!hist[i].calc[c])
            i--;
        return i;
    }

    void bring_up_to_date(Board &board)
    {
        for (auto side : {BLACK, WHITE})
        {
            if (!hist[hist_size - 1].calc[side])
            {
                int last_computed_pos = get_computed_parent(side) + 1;
                // std::cerr << last_computed_pos << " " << hist_size << " haha\n";
                if (last_computed_pos)
                { // no full refresh required
                    while (last_computed_pos < hist_size)
                    {
                        process_historic_update(last_computed_pos, side);
                        last_computed_pos++;
                    }
                }
            }
        }
    }

    float score_move(bool stm, Move move)
    {
        int move_index = policy_move_index(stm, move);
        reg_type_s acc{};
        const reg_type *w =
            reinterpret_cast<const reg_type *>(&output_history[hist_size - 1][stm * POLICY_SIDE_NEURONS]);
        const reg_type *w2 =
            reinterpret_cast<const reg_type *>(&output_history[hist_size - 1][(stm ^ 1) * POLICY_SIDE_NEURONS]);
        const reg_type *v =
            reinterpret_cast<const reg_type *>(&policy_output_weights[move_index * POLICY_HIDDEN_NEURONS]);
        const reg_type *v2 = reinterpret_cast<const reg_type *>(
            &policy_output_weights[move_index * POLICY_HIDDEN_NEURONS + POLICY_SIDE_NEURONS]);
        reg_type clamped;

        for (int j = 0; j < POLICY_NUM_REGS; j++)
        {
            clamped = reg_clamp(w[j]);
            acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v[j]), clamped));
            clamped = reg_clamp(w2[j]);
            acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v2[j]), clamped));
        }

        return (policy_output_biases[move_index] + 1.0 * get_sum(acc) / POLICY_Q_IN) / (POLICY_Q_IN * POLICY_Q_OUT);
    }

    int score_move_movepicker(bool stm, Move move)
    {
        float raw_score = score_move(stm, move);
        return 20000 * exp(raw_score);
    }

    // used for debugging
    std::array<float, 256> score_moves(Board &board)
    {
        init(board);
        MoveList moves;
        std::array<float, 256> scores;
        int nr_moves = board.gen_legal_moves<MOVEGEN_ALL>(moves);
        float sum_exp = 0;

        for (int i = 0; i < nr_moves; i++)
        {
            scores[i] = score_move(board.turn, moves[i]);
            std::cout << moves[i].to_string() << " has score " << scores[i] << " -> " << exp(scores[i]) * 20000
                      << " OR " << score_move_movepicker(board.turn, moves[i]) << "\n";
            sum_exp += exp(scores[i]);
        }
        for (int i = 0; i < nr_moves; i++)
        {
            // scores[i] = exp(scores[i]) / sum_exp;
        }
        return scores;
    }

  private:
    struct PolicyHist
    {
        Move move;
        Piece piece, captured;
        bool calc[2];
    };

    alignas(ALIGN) int16_t output_history[STACK_SIZE][POLICY_HIDDEN_NEURONS];
    PolicyHist hist[STACK_SIZE];
    int hist_size;
};