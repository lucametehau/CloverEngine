#pragma once

#include "attacks.h"
#include "board.h"
#include <algorithm>
#include <cmath>

constexpr int POLICY_INPUT_NEURONS = 768;
constexpr int POLICY_SIDE_NEURONS = 128;
constexpr int POLICY_OUTPUT_NEURONS = 1880;
constexpr int POLICY_HIDDEN_NEURONS = POLICY_SIDE_NEURONS * 2;

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

int16_t policy_input_weights[POLICY_SIDE_NEURONS * POLICY_INPUT_NEURONS];
int16_t policy_input_biases[POLICY_SIDE_NEURONS];
int16_t policy_output_weights[POLICY_HIDDEN_NEURONS * POLICY_OUTPUT_NEURONS];
int16_t policy_output_biases[POLICY_OUTPUT_NEURONS];

int policy_move_index(bool color, Move move)
{
    if (move.is_promo())
    {
        int id = 2 * (move.get_from() & 7) + (move.get_to() & 7);
        return OFFSETS[64] + 22 * (move.get_prom() - PieceTypes::KNIGHT) + id;
    }

    return OFFSETS[move.get_from().mirror(color)] +
           (ALL_DESTINATIONS[move.get_from().mirror(color)] & ((1ull << move.get_to().mirror(color)) - 1)).count();
}

void init_policy_network()
{
    FILE *net = fopen("policy_net_montydata.bin", "rb");

    assert(fread(policy_input_weights, sizeof(int16_t), POLICY_SIDE_NEURONS * POLICY_INPUT_NEURONS, net) ==
           POLICY_SIDE_NEURONS * POLICY_INPUT_NEURONS);
    assert(fread(policy_input_biases, sizeof(int16_t), POLICY_SIDE_NEURONS, net) == POLICY_SIDE_NEURONS);
    assert(fread(policy_output_weights, sizeof(int16_t), POLICY_HIDDEN_NEURONS * POLICY_OUTPUT_NEURONS, net) ==
           POLICY_HIDDEN_NEURONS * POLICY_OUTPUT_NEURONS);
    assert(fread(policy_output_biases, sizeof(int16_t), POLICY_OUTPUT_NEURONS, net) == POLICY_OUTPUT_NEURONS);

    fclose(net);
}

class PolicyNetwork
{
  public:
    PolicyNetwork()
    {
    }

    void init(Board &board)
    {
        for (int i = 0; i < POLICY_SIDE_NEURONS; i++)
        {
            for (auto color : {WHITE, BLACK})
            {
                output[i + POLICY_SIDE_NEURONS * color] = policy_input_biases[i];
                for (auto c : {WHITE, BLACK})
                {
                    for (Piece pt = PieceTypes::PAWN; pt <= PieceTypes::KING; pt++)
                    {
                        Bitboard bb = board.get_bb_piece(pt, c);
                        Piece piece = Piece(pt, c);
                        while (bb)
                        {
                            int input_index =
                                64 * (piece + color * (piece >= 6 ? -6 : +6)) + bb.get_lsb_square().mirror(color);
                            output[i + POLICY_SIDE_NEURONS * color] +=
                                policy_input_weights[input_index * POLICY_SIDE_NEURONS + i];
                            bb ^= bb.lsb();
                        }
                    }
                }
            }
        }
    }

    void init_and_compute(Board &board)
    {
        init(board);

        MoveList moves;
        std::array<float, 256> scores;
        int nr_moves = board.gen_legal_moves<MOVEGEN_ALL>(moves);

        sum_exp = 0;
        for (int i = 0; i < nr_moves; i++)
        {
            scores[i] = score_move(board.turn, moves[i]);
            sum_exp += exp(scores[i]);
        }
    }

    int32_t screlu(int32_t x)
    {
        x = std::clamp(x, 0, POLICY_Q_IN);
        return x * x;
    }

    float softmax(float x)
    {
        return exp(x) / sum_exp;
    }

    float score_move(bool stm, Move move)
    {
        int move_index = policy_move_index(stm, move);
        float sum = 0;
        for (int i = 0; i < POLICY_SIDE_NEURONS; i++)
        {
            sum += screlu(output[stm * POLICY_SIDE_NEURONS + i]) *
                   policy_output_weights[POLICY_HIDDEN_NEURONS * move_index + i];
            sum += screlu(output[(1 - stm) * POLICY_SIDE_NEURONS + i]) *
                   policy_output_weights[POLICY_HIDDEN_NEURONS * move_index + POLICY_SIDE_NEURONS + i];
        }

        return (policy_output_biases[move_index] + sum / POLICY_Q_IN) / (POLICY_Q_IN * POLICY_Q_OUT);
    }

    int score_move_movepicker(bool stm, Move move)
    {
        float raw_score = softmax(score_move(stm, move));
        return 20000 * log(1 + raw_score * 19) / log(20);
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
            sum_exp += exp(scores[i]);
        }
        for (int i = 0; i < nr_moves; i++)
        {
            scores[i] = exp(scores[i]) / sum_exp;
        }
        return scores;
    }

  private:
    int16_t output[POLICY_HIDDEN_NEURONS];
    float sum_exp;
};