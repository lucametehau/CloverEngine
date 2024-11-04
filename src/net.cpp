#include "net.h"

INCBIN(Net, EVALFILE);

NetworkWeights network;

void load_nnue_weights() {
    int16_t* intData = (int16_t*)gNetData;

    for (int i = 0; i < SIDE_NEURONS * INPUT_NEURONS; i++) {
        network.inputWeights[(i / SIDE_NEURONS) * SIDE_NEURONS + (i % SIDE_NEURONS)] = *intData;
        intData++;
    }
    for (int j = 0; j < SIDE_NEURONS; j++) {
        network.inputBiases[j] = *intData;
        intData++;
    }
    for (int j = 0; j < HIDDEN_NEURONS; j++) {
        network.outputWeights[j] = *intData;
        intData++;
    }
    network.outputBias = *intData;
}

inline reg_type reg_clamp(reg_type reg) {
    static const reg_type zero{};
    static const reg_type one = reg_set1(Q_IN);
    return reg_min16(reg_max16(reg, zero), one);
}

inline int32_t get_sum(reg_type_s& x) {
#if   defined(__AVX512F__)
    __m256i reg_256 = _mm256_add_epi32(_mm512_castsi512_si256(x), _mm512_extracti32x8_epi32(x, 1));
    __m128i a = _mm_add_epi32(_mm256_castsi256_si128(reg_256), _mm256_extractf128_si256(reg_256, 1));
#elif defined(__AVX2__)
    __m128i a = _mm_add_epi32(_mm256_castsi256_si128(x), _mm256_extractf128_si256(x, 1));
#elif defined(__SSE2__) || defined(__AVX__)
    __m128i a = x;
#endif

#if   defined(__ARM_NEON)
    return x;
#else
    __m128i b = _mm_add_epi32(a, _mm_srli_si128(a, 8));
    __m128i c = _mm_add_epi32(b, _mm_srli_si128(b, 4));
    return _mm_cvtsi128_si32(c);
#endif
}

Network::Network() {
    hist_size = 0;
    for (auto c : { BLACK, WHITE }) {
        for (int i = 0; i < 2 * KING_BUCKETS; i++) {
            memcpy(cached_states[c][i].output, network.inputBiases, sizeof(network.inputBiases));
            cached_states[c][i].bb.fill(Bitboard());
        }
    }
}

void Network::add_input(int ind) { add_ind[add_size++] = ind; }
void Network::remove_input(int ind) { sub_ind[sub_size++] = ind; }
void Network::clear_updates() { add_size = sub_size = 0; }

int32_t Network::calc(NetInput& input, bool stm) {
    int32_t sum;

    for (int n = 0; n < SIDE_NEURONS; n++) {
        sum = network.inputBiases[n];
        for (auto& prevN : input.ind[WHITE]) {
            sum += network.inputWeights[prevN * SIDE_NEURONS + n];
        }
        assert(-32768 <= sum && sum <= 32767);
        output_history[0][SIDE_NEURONS + n] = sum;

        sum = network.inputBiases[n];
        for (auto& prevN : input.ind[BLACK]) {
            sum += network.inputWeights[prevN * SIDE_NEURONS + n];
        }
        assert(-32768 <= sum && sum <= 32767);
        output_history[0][n] = sum;
    }

    hist_size = 1;
    hist[0].calc[0] = hist[0].calc[1] = 1;

    return get_output(stm);
}

void Network::apply_updates(int16_t* output, int16_t* input) {
    reg_type regs[UNROLL_LENGTH];

    for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
        const int offset = b * BUCKET_UNROLL;
        const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_load(&reg_in[i]);
        for (int idx = 0; idx < add_size; idx++) {
            reg_type* reg = reinterpret_cast<reg_type*>(&network.inputWeights[add_ind[idx] * SIDE_NEURONS + offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_add16(regs[i], reg[i]);
        }

        for (int idx = 0; idx < sub_size; idx++) {
            reg_type* reg = reinterpret_cast<reg_type*>(&network.inputWeights[sub_ind[idx] * SIDE_NEURONS + offset]);
            for (int i = 0; i < UNROLL_LENGTH; i++)
                regs[i] = reg_sub16(regs[i], reg[i]);
        }

        reg_type* reg_out = (reg_type*)&output[offset];
        for (int i = 0; i < UNROLL_LENGTH; i++)
            reg_save(&reg_out[i], regs[i]);
    }
}

void Network::apply_sub_add(int16_t* output, int16_t* input, int ind1, int ind2) {
    reg_type regs[UNROLL_LENGTH];
    const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind1 * SIDE_NEURONS]);
    const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind2 * SIDE_NEURONS]);

    for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
        const int offset = b * BUCKET_UNROLL;
        const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_load(&reg_in[i]);

        const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_sub16(regs[i], reg1[i]);
        const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_add16(regs[i], reg2[i]);

        reg_type* reg_out = (reg_type*)&output[offset];
        for (int i = 0; i < UNROLL_LENGTH; i++)
            reg_save(&reg_out[i], regs[i]);
    }
}

void Network::apply_sub_add_sub(int16_t* output, int16_t* input, int ind1, int ind2, int ind3) {
    reg_type regs[UNROLL_LENGTH];
    const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind1 * SIDE_NEURONS]);
    const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind2 * SIDE_NEURONS]);
    const int16_t* inputWeights3 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind3 * SIDE_NEURONS]);

    for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
        const int offset = b * BUCKET_UNROLL;
        const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_load(&reg_in[i]);

        const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_sub16(regs[i], reg1[i]);
        const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_add16(regs[i], reg2[i]);
        const reg_type* reg3 = reinterpret_cast<const reg_type*>(&inputWeights3[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_sub16(regs[i], reg3[i]);

        reg_type* reg_out = (reg_type*)&output[offset];
        for (int i = 0; i < UNROLL_LENGTH; i++)
            reg_save(&reg_out[i], regs[i]);
    }
}

void Network::apply_sub_add_sub_add(int16_t* output, int16_t* input, int ind1, int ind2, int ind3, int ind4) {
    reg_type regs[UNROLL_LENGTH];
    const int16_t* inputWeights1 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind1 * SIDE_NEURONS]);
    const int16_t* inputWeights2 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind2 * SIDE_NEURONS]);
    const int16_t* inputWeights3 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind3 * SIDE_NEURONS]);
    const int16_t* inputWeights4 = reinterpret_cast<const int16_t*>(&network.inputWeights[ind4 * SIDE_NEURONS]);

    for (int b = 0; b < SIDE_NEURONS / BUCKET_UNROLL; b++) {
        const int offset = b * BUCKET_UNROLL;
        const reg_type* reg_in = reinterpret_cast<const reg_type*>(&input[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_load(&reg_in[i]);

        const reg_type* reg1 = reinterpret_cast<const reg_type*>(&inputWeights1[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_sub16(regs[i], reg1[i]);
        const reg_type* reg2 = reinterpret_cast<const reg_type*>(&inputWeights2[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_add16(regs[i], reg2[i]);
        const reg_type* reg3 = reinterpret_cast<const reg_type*>(&inputWeights3[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_sub16(regs[i], reg3[i]);
        const reg_type* reg4 = reinterpret_cast<const reg_type*>(&inputWeights4[offset]);
        for (int i = 0; i < UNROLL_LENGTH; i++)
            regs[i] = reg_add16(regs[i], reg4[i]);

        reg_type* reg_out = (reg_type*)&output[offset];
        for (int i = 0; i < UNROLL_LENGTH; i++)
            reg_save(&reg_out[i], regs[i]);
    }
}

void Network::process_move(Move move, Piece piece, Piece captured, Square king, bool side, int16_t* a, int16_t* b) {
    Square from = move.get_from(), to = move.get_to();
    const bool turn = piece.color();
    switch (move.get_type()) {
    case NO_TYPE: {
        if (captured == NO_PIECE)
            apply_sub_add(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side));
        else
            apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(captured, to, king, side));
    }
    break;
    case MoveTypes::CASTLE: {
        Square rFrom = to, rTo;
        Piece rPiece = Piece(PieceTypes::ROOK, turn);
        if (to > from) { // king side castle
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else { // queen side castle
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }
        apply_sub_add_sub_add(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(rPiece, rFrom, king, side), net_index(rPiece, rTo, king, side));
    }
    break;
    case MoveTypes::ENPASSANT: {
        const Square pos = shift_square<SOUTH>(turn, to);
        const Piece pieceCap = Piece(PieceTypes::PAWN, 1 ^ turn);
        apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(piece, to, king, side), net_index(pieceCap, pos, king, side));
    }
    break;
    default: {
        const int promPiece = Piece(move.get_prom() + PieceTypes::KNIGHT, turn);
        if (captured == NO_PIECE)
            apply_sub_add(a, b, net_index(piece, from, king, side), net_index(promPiece, to, king, side));
        else
            apply_sub_add_sub(a, b, net_index(piece, from, king, side), net_index(promPiece, to, king, side), net_index(captured, to, king, side));
    }
    break;
    }
}

void Network::process_historic_update(const int index, const Square king_sq, const bool side) {
    hist[index].calc[side] = 1;
    process_move(hist[index].move, hist[index].piece, hist[index].cap, king_sq, side,
        &output_history[index][side * SIDE_NEURONS], &output_history[index - 1][side * SIDE_NEURONS]);
}

void Network::add_move_to_history(Move move, Piece piece, Piece captured) {
    hist[hist_size] = { move, piece, captured, piece.type() == PieceTypes::KING && recalc(move.get_from(), move.get_special_to() , piece.color()), { 0, 0 } };
    hist_size++;
}

void Network::revert_move() { hist_size--; }

void Network::bring_up_to_date(Board &board) {
    for (auto side : { BLACK, WHITE }) {
        if (!hist[hist_size - 1].calc[side]) {
            int last_computed_pos = get_computed_parent(side) + 1;
            const int king_sq = board.get_king(side);
            if (last_computed_pos) { // no full refresh required
                while (last_computed_pos < hist_size) {
                    process_historic_update(last_computed_pos, king_sq, side);
                    last_computed_pos++;
                }
            }
            else {
                KingBucketState* state = &cached_states[side][get_king_bucket_cache_index(king_sq, side)];
                clear_updates();
                for (Piece i = Pieces::BlackPawn; i <= Pieces::WhiteKing; i++) {
                    Bitboard prev = state->bb[i];
                    Bitboard curr = board.bb[i];

                    Bitboard b = curr & ~prev; // additions
                    while (b) add_input(net_index(i, b.get_square_pop(), king_sq, side));

                    b = prev & ~curr; // removals
                    while (b) remove_input(net_index(i, b.get_square_pop(), king_sq, side));

                    state->bb[i] = curr;
                }
                apply_updates(state->output, state->output);
                memcpy(&output_history[hist_size - 1][side * SIDE_NEURONS], state->output, SIDE_NEURONS * sizeof(int16_t));
                hist[hist_size - 1].calc[side] = 1;
            }
        }
    }
}

int Network::get_computed_parent(const bool c) {
    int i = hist_size - 1;
    while (!hist[i].calc[c]) {
        if (hist[i].piece.color() == c && hist[i].recalc)
            return -1;
        i--;
    }
    return i;
}

int32_t Network::get_output(bool stm) {
    reg_type_s acc{};

    const reg_type* w = reinterpret_cast<const reg_type*>(&output_history[hist_size - 1][stm * SIDE_NEURONS]);
    const reg_type* w2 = reinterpret_cast<const reg_type*>(&output_history[hist_size - 1][(stm ^ 1) * SIDE_NEURONS]);
    const reg_type* v = reinterpret_cast<const reg_type*>(network.outputWeights);
    const reg_type* v2 = reinterpret_cast<const reg_type*>(&network.outputWeights[SIDE_NEURONS]);
    reg_type clamped;

    for (int j = 0; j < NUM_REGS; j++) {
        clamped = reg_clamp(w[j]);
        acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v[j]), clamped));
        clamped = reg_clamp(w2[j]);
        acc = reg_add32(acc, reg_madd16(reg_mullo(clamped, v2[j]), clamped));
    }

    return (network.outputBias + get_sum(acc) / Q_IN) * 225 / Q_IN_HIDDEN;
}