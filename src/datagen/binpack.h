#pragma once
#include "../board.h"
#include <fstream>
#include <deque>

class PackedBoard {
    Bitboard occ;
    std::array<uint32_t, 4> pieces;
    uint8_t state;
    uint8_t halfmove;
    uint16_t fullmove;
    int16_t score;
    uint8_t result;
    uint8_t unused;

public:
    void push_piece(uint32_t piece, int cnt) {
        pieces[cnt >> 3] = pieces[cnt >> 3] | (piece << (4 * (cnt & 7)));
    }

    constexpr PackedBoard() : occ(0), pieces{0, 0, 0, 0}, state(0), halfmove(0), fullmove(0), score(0), result(1), unused(0) {}

    PackedBoard(Board& board) {
        pieces = {0, 0, 0, 0};
        occ = board.get_bb_color(WHITE) | board.get_bb_color(BLACK);
        int cnt = 0;
        for (Square sq = 0; sq < 64; sq++) {
            if (occ.has_square(sq)) {
                Piece p = board.piece_at(sq);
                uint8_t type = p.type();
                if (type == PieceTypes::ROOK) {
                    for (auto color : {WHITE, BLACK}) {
                        if (board.rook_sq(color, 0) == sq || board.rook_sq(color, 1) == sq) {
                            type = 6;
                            break;
                        }
                    }
                }
                uint8_t piece = type | (p.color() == WHITE ? 0 : 0x8);
                push_piece(piece, cnt);
                cnt++;
            }
        }
        state = (board.turn == BLACK ? 0x80 : 0) | board.enpas();
        halfmove = board.half_moves();
        fullmove = board.move_index();
        score = 0;
        result = 1;  // Default to draw
        unused = 0;
    }

    Board get_board() {
        Board board;
        std::unique_ptr<std::deque<HistoricalState>> states;
        states = std::make_unique<std::deque<HistoricalState>>(1);

        board.state = &states->back();

        board.key() = board.pawn_key() = board.mat_key(WHITE) = board.mat_key(BLACK) = 0;
        board.ply = board.game_ply = 0;
        board.captured() = NO_PIECE;

        board.bb.fill(Bitboard(0ull));
        board.pieces.fill(Bitboard(0ull));
        board.board.fill(NO_PIECE);
        for (auto c : {WHITE, BLACK}) {
            board.rook_sq(c, 0) = board.rook_sq(c, 1) = NO_SQUARE;
        }

        int cnt = occ.count();
        for (int i = 0; i < cnt; i++) {
            int p = ((pieces[i >> 3] >> (4 * ((i & 7)))) & 15);
            Square sq = occ.get_lsb_square();
            occ ^= Bitboard(sq);
            bool color = p & 8 ? BLACK : WHITE;
            Piece piece(p & 7 == 6 ? PieceTypes::ROOK : Piece(p & 7), color);
            board.place_piece_at_sq(piece, sq);

            if (p & 7 == 6) {
                if (board.rook_sq(color, 0) == NO_SQUARE && !board.get_bb_piece(PieceTypes::KING, color))
                    board.rook_sq(color, 0) = sq;
                else
                    board.rook_sq(color, 1) = sq;
            }
        }
        board.turn = state & 0x80 ? BLACK : WHITE;
        board.enpas() = state & 0x7F;
        
        board.half_moves() = halfmove;
        board.move_index() = fullmove;
        board.get_pinned_pieces_and_checkers();
        board.get_threats(board.turn);
        return board;
    }

    void set_result(uint8_t res) {
        result = res;
    }
};

class MarlinMove {
    uint16_t move;

public:
    MarlinMove() = default;
    MarlinMove(Move _move) {
        move = _move.get_from_to();
        switch (_move.get_type())
        {
        case MoveTypes::ENPASSANT:
            move |= 1 << 14;
            break;
        case MoveTypes::CASTLE:
            move |= 2 << 14;
            break;
        case MoveTypes::KNIGHT_PROMO:
            move |= 3 << 14;
            break;
        case MoveTypes::BISHOP_PROMO:
            move |= (1 << 12) | (3 << 14);
            break;
        case MoveTypes::ROOK_PROMO:
            move |= (2 << 12) | (3 << 14);
            break;
        case MoveTypes::QUEEN_PROMO:
            move |= (3 << 12) | (3 << 14);
            break;
        
        default:
            break;
        }
    }

    Move to_move() {
        int type = MoveTypes::NO_TYPE;

        switch ((move >> 12))
        {
        case 4:
            type = MoveTypes::ENPASSANT;
            break;
        case 8:
            type = MoveTypes::CASTLE;
            break;
        case 12:
            type = MoveTypes::KNIGHT_PROMO;
            break;
        case 13:
            type = MoveTypes::BISHOP_PROMO;
            break;
        case 14:
            type = MoveTypes::ROOK_PROMO;
            break;
        case 15:
            type = MoveTypes::QUEEN_PROMO;
            break;
        
        default:
            break;
        }
        return Move(Square(move & 63), Square((move >> 6) & 63), type);
    }
};

class BinpackFormat {
    PackedBoard packed_board;
    std::vector<std::pair<MarlinMove, int16_t>> moves;

public:
    BinpackFormat() = default;
    void init(Board& board) {
        packed_board = PackedBoard(board);
        moves.clear();
        moves.reserve(1024);
    }

    void add_move(Move move, int16_t score) {
        moves.push_back(std::make_pair(MarlinMove(move), score));
    }

    void set_result(uint8_t res) {
        packed_board.set_result(res);
    }

    void write(std::ofstream& file) {
        file.write(reinterpret_cast<const char*>(&packed_board), sizeof(PackedBoard));
        file.write(reinterpret_cast<const char*>(moves.data()), moves.size() * sizeof(std::pair<MarlinMove, int16_t>));
        uint32_t terminator = 0;
        file.write(reinterpret_cast<const char*>(&terminator), sizeof(uint32_t));
    }
};