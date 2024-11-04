#include "board.h"
#include "cuckoo.h"

Board::Board() { set_fen(START_POS_FEN); }

void Board::clear() { ply = 0; }

void Board::print() {
    for (int i = 7; i >= 0; i--) {
        for (Square sq = Square(i, 0); sq < Square(i + 1, 0); sq++)
            std::cerr << piece_char[board[sq]] << " ";
        std::cerr << "\n";
    }
}

Square Board::get_king(const bool color) { return get_bb_piece(PieceTypes::KING, color).get_lsb_square(); }

Bitboard Board::get_bb_piece(const Piece piece, const bool color) { return bb[Piece(piece, color)]; }
Bitboard Board::get_bb_color(const bool color) { return pieces[color]; }
Bitboard Board::get_bb_piece_type(const Piece piece_type) {  return get_bb_piece(piece_type, WHITE) | get_bb_piece(piece_type, BLACK); }

Bitboard Board::diagonal_sliders(const bool color) { return get_bb_piece(PieceTypes::BISHOP, color) | get_bb_piece(PieceTypes::QUEEN, color); }
Bitboard Board::orthogonal_sliders(const bool color) { return get_bb_piece(PieceTypes::ROOK, color) | get_bb_piece(PieceTypes::QUEEN, color); }

Piece Board::piece_at(const Square sq) { return board[sq]; }
Piece Board::piece_type_at(const Square sq) { return piece_at(sq).type(); }
Piece Board::get_captured_type(const Move move) { return move.get_type() == MoveTypes::ENPASSANT ? PieceTypes::PAWN : piece_type_at(move.get_to()); }

bool Board::is_capture(const Move move) { return move.get_type() != MoveTypes::CASTLE && piece_at(move.get_to()) != NO_PIECE; }
bool Board::is_noisy_move(const Move move) { 
    return (move.get_type() && move.get_type() != MoveTypes::CASTLE) || is_capture(move); 
}

Bitboard Board::get_attackers(const bool color, const Bitboard blockers, const Square sq) {
    return (attacks::genAttacksPawn(1 ^ color, sq) & get_bb_piece(PieceTypes::PAWN, color)) |
        (attacks::genAttacksKnight(sq) & get_bb_piece(PieceTypes::KNIGHT, color)) | 
        (attacks::genAttacksBishop(blockers, sq) & diagonal_sliders(color)) |
        (attacks::genAttacksRook(blockers, sq) & orthogonal_sliders(color)) | 
        (attacks::genAttacksKing(sq) & get_bb_piece(PieceTypes::KING, color));
}

Bitboard Board::get_pinned_pieces() {
    const bool enemy = turn ^ 1;
    const Square king = get_king(turn);
    Bitboard us = pieces[turn], them = pieces[enemy];
    Bitboard pinned; /// squares attacked by enemy / pinned pieces
    Bitboard mask = (attacks::genAttacksRook(them, king) & orthogonal_sliders(enemy)) | 
                    (attacks::genAttacksBishop(them, king) & diagonal_sliders(enemy));

    while (mask) {
        Bitboard b2 = us & between_mask[mask.get_square_pop()][king];
        if (b2.count() == 1) pinned ^= b2;
    }

    return pinned;
}

Bitboard Board::get_pawn_attacks(const bool color) {
    const Bitboard b = get_bb_piece(PieceTypes::PAWN, color);
    const int fileA = color == WHITE ? 0 : 7, fileH = 7 - fileA;
    return shift_mask<NORTHWEST>(color, b & ~file_mask[fileA]) | shift_mask<NORTHEAST>(color, b & ~file_mask[fileH]);
}

bool Board::is_attacked_by(const bool color, const Square sq) { 
    return get_attackers(color, get_bb_color(WHITE) | get_bb_color(BLACK), sq); 
}

void Board::make_move(const Move move) { /// assuming move is at least pseudo-legal
    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(from), piece_cap = piece_at(to);

    history[game_ply] = state;
    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    half_moves()++;

    if (piece.type() == PieceTypes::PAWN)
        half_moves() = 0;

    captured() = NO_PIECE;
    enpas() = NO_SQUARE;

    switch (move.get_type()) {
    case NO_TYPE:
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        if (piece.type() == PieceTypes::PAWN) pawn_key() ^= hashKey[piece][from] ^ hashKey[piece][to];
        else mat_key(turn) ^= hashKey[piece][from] ^ hashKey[piece][to];
        /// moved a castle rook
        if (piece == Pieces::WhiteRook)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == Pieces::BlackRook)
            castle_rights() &= castleRightsDelta[BLACK][from];

        if (piece_cap != NO_PIECE) {
            half_moves() = 0;

            pieces[1 ^ turn] ^= (1ULL << to);
            bb[piece_cap] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            if (piece_cap.type() == PieceTypes::PAWN) pawn_key() ^= hashKey[piece_cap][to];
            else mat_key(1 ^ turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == Pieces::WhiteRook)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == Pieces::BlackRook)
                castle_rights() &= castleRightsDelta[BLACK][to];
        }

        board[from] = NO_PIECE;
        board[to] = piece;
        captured() = piece_cap;

        /// double push
        if (piece.type() == PieceTypes::PAWN && (from ^ to) == 16) {
            if ((to % 8 && board[to - 1] == Piece(PieceTypes::PAWN, turn ^ 1)) || 
                (to % 8 < 7 && board[to + 1] == Piece(PieceTypes::PAWN, turn ^ 1))) 
                enpas() = shift_square<NORTH>(turn, from), key() ^= enPasKey[enpas()];
        }

        /// moved the king
        if (piece == Pieces::WhiteKing) {
            castle_rights() &= castleRightsDelta[WHITE][from];
        }
        else if (piece == Pieces::BlackKing) {
            castle_rights() &= castleRightsDelta[BLACK][from];
        }

        break;
    case MoveTypes::ENPASSANT:
    {
        const Square pos = shift_square<SOUTH>(turn, to);
        piece_cap = Piece(PieceTypes::PAWN, 1 ^ turn);
        half_moves() = 0;
        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^ hashKey[piece_cap][pos];
        pawn_key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^ hashKey[piece_cap][pos];

        pieces[1 ^ turn] ^= (1ULL << pos);
        bb[piece_cap] ^= (1ULL << pos);

        board[from] = board[pos] = NO_PIECE;
        board[to] = piece;
    }

    break;
    case MoveTypes::CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece = Piece(PieceTypes::ROOK, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else { // queen side castle
            rFrom = to;
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to) ^ (1ULL << rFrom) ^ (1ULL << rTo);
        bb[piece] ^= (1ULL << from) ^ (1ULL << to);
        bb[rPiece] ^= (1ULL << rFrom) ^ (1ULL << rTo);

        key() ^= hashKey[piece][from] ^ hashKey[piece][to] ^
                 hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];
        mat_key(turn) ^= hashKey[piece][from] ^ hashKey[piece][to] ^
                         hashKey[rPiece][rFrom] ^ hashKey[rPiece][rTo];

        board[from] = board[rFrom] = NO_PIECE;
        board[to] = piece;
        board[rTo] = rPiece;
        captured() = NO_PIECE;

        if (piece == Pieces::WhiteKing)
            castle_rights() &= castleRightsDelta[WHITE][from];
        else if (piece == Pieces::BlackKing)
            castle_rights() &= castleRightsDelta[BLACK][from];
    }

    break;
    default: /// promotion
    {
        Piece prom_piece = Piece(move.get_prom() + PieceTypes::KNIGHT, turn);

        pieces[turn] ^= (1ULL << from) ^ (1ULL << to);
        bb[piece] ^= (1ULL << from);
        bb[prom_piece] ^= (1ULL << to);

        if (piece_cap != NO_PIECE) {
            bb[piece_cap] ^= (1ULL << to);
            pieces[1 ^ turn] ^= (1ULL << to);
            key() ^= hashKey[piece_cap][to];
            mat_key(1 ^ turn) ^= hashKey[piece_cap][to];

            /// special case: captured rook might have been a castle rook
            if (piece_cap == Pieces::WhiteRook)
                castle_rights() &= castleRightsDelta[WHITE][to];
            else if (piece_cap == Pieces::BlackRook)
                castle_rights() &= castleRightsDelta[BLACK][to];
        }

        board[from] = NO_PIECE;
        board[to] = prom_piece;
        captured() = piece_cap;

        key() ^= hashKey[piece][from] ^ hashKey[prom_piece][to];
        pawn_key() ^= hashKey[piece][from];
    }

    break;
    }

    key() ^= castleKeyModifier[castle_rights() ^ history[game_ply].castleRights];
    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], get_king(1 ^ turn));

    turn ^= 1;
    ply++;
    game_ply++;
    key() ^= 1;
    if (turn == WHITE) move_index()++;

    pinned_pieces() = get_pinned_pieces();
}

void Board::undo_move(const Move move) {
    turn ^= 1;
    ply--;
    game_ply--;
    Piece piece_cap = captured();
    
    state = history[game_ply];

    Square from = move.get_from(), to = move.get_to();
    Piece piece = piece_at(to);

    switch (move.get_type()) {
    case NO_TYPE:
        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);

        board[from] = piece;
        board[to] = piece_cap;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= Bitboard(to);
            bb[piece_cap] ^= Bitboard(to);
        }
        break;
    case MoveTypes::CASTLE:
    {
        Square rFrom, rTo;
        Piece rPiece = Piece(PieceTypes::ROOK, turn);

        piece = Piece(PieceTypes::KING, turn);

        if (to > from) { // king side castle
            rFrom = to;
            to = Squares::G1.mirror(turn);
            rTo = Squares::F1.mirror(turn);
        }
        else { // queen side castle
            rFrom = to;
            to = Squares::C1.mirror(turn);
            rTo = Squares::D1.mirror(turn);
        }

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to) ^ Bitboard(rFrom) ^ Bitboard(rTo);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);
        bb[rPiece] ^= Bitboard(rFrom) ^ Bitboard(rTo);

        board[to] = board[rTo] = NO_PIECE;
        board[from] = piece;
        board[rFrom] = rPiece;
    }
    break;
    case MoveTypes::ENPASSANT:
    {
        Square pos = shift_square<SOUTH>(turn, to);

        piece_cap = Piece(PieceTypes::PAWN, 1 ^ turn);

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from) ^ Bitboard(to);

        pieces[1 ^ turn] ^= Bitboard(pos);
        bb[piece_cap] ^= Bitboard(pos);

        board[to] = NO_PIECE;
        board[from] = piece;
        board[pos] = piece_cap;
    }
    break;
    default: /// promotion
    {
        Piece prom_piece = Piece(move.get_prom() + PieceTypes::KNIGHT, turn);

        piece = Piece(PieceTypes::PAWN, turn);

        pieces[turn] ^= Bitboard(from) ^ Bitboard(to);
        bb[piece] ^= Bitboard(from);
        bb[prom_piece] ^= Bitboard(to);

        board[to] = piece_cap;
        board[from] = piece;

        if (piece_cap != NO_PIECE) {
            pieces[1 ^ turn] ^= Bitboard(to);
            bb[piece_cap] ^= Bitboard(to);
        }
    }
    break;
    }

    captured() = history[game_ply].captured;
}

void Board::make_null_move() {
    history[game_ply] = state;
    
    key() ^= enpas() != NO_SQUARE ? enPasKey[enpas()] : 0;

    checkers() = get_attackers(turn, pieces[WHITE] | pieces[BLACK], get_king(1 ^ turn));
    captured() = NO_PIECE;
    enpas() = NO_SQUARE;
    turn ^= 1;
    key() ^= 1;
    pinned_pieces() = get_pinned_pieces();
    ply++;
    game_ply++;
    half_moves()++;
    move_index()++;
}

void Board::undo_null_move() {
    turn ^= 1;
    ply--;
    game_ply--;

    state = history[game_ply];
}

bool Board::has_non_pawn_material(const bool color) {
    return (get_bb_piece(PieceTypes::KING, color) ^ get_bb_piece(PieceTypes::PAWN, color)) != get_bb_color(color);
}

//void Board::bring_up_to_date();

NetInput Board::to_netinput() {
    NetInput ans;
    for (auto color : {WHITE, BLACK}) {
        for (Piece i = Pieces::BlackPawn; i <= Pieces::WhiteKing; i++) {
            Bitboard b = bb[i];
            while (b) {
                ans.ind[color].push_back(net_index(i, b.get_lsb_square(), get_king(color), color));
                b ^= b.lsb();
            }
        }
    }

    return ans;
}

void Board::place_piece_at_sq(Piece piece, Square sq) {
    board[sq] = piece;
    key() ^= hashKey[piece][sq];
    if (piece.type() == PieceTypes::PAWN) pawn_key() ^= hashKey[piece][sq];
    else mat_key(piece.color()) ^= hashKey[piece][sq];

    pieces[piece.color()] |= (1ULL << sq);
    bb[piece] |= (1ULL << sq);
}

std::string Board::fen() {
    std::string fen;
    for (int i = 7; i >= 0; i--) {
        int cnt = 0;
        for (Square sq = Square(i, 0); sq < Square(i + 1, 0); sq++) {
            if (piece_at(sq) == NO_PIECE) cnt++;
            else {
                if (cnt) fen += char(cnt + '0');
                cnt = 0;
                fen += piece_char[piece_at(sq)];
            }
        }
        if (cnt) fen += char(cnt + '0');
        if (i) fen += "/";
    }
    fen += " ";
    fen += (turn == WHITE ? "w" : "b");
    fen += " ";
    const std::string castle_ch = "qkQK";
    for (int i = 3; i >= 0; i--) {
        if (castle_rights() & (1 << i)) fen += castle_ch[i];
    }
    if (!castle_rights()) fen += "-";
    fen += " ";
    if (enpas() != NO_SQUARE) {
        fen += char('a' + enpas() % 8);
        fen += char('1' + enpas() / 8);
    }
    else
        fen += "-";
    fen += " " + std::to_string(half_moves()) + " " + std::to_string(move_index());
    return fen;
}

void Board::set_fen(const std::string fen) {
    int ind = 0;
    key() = pawn_key() = mat_key(WHITE) = mat_key(BLACK) = 0;
    ply = game_ply = 0;
    captured() = NO_PIECE;

    //checkers() = 0;
    bb.fill(Bitboard());
    pieces.fill(Bitboard());

    for (int i = 7; i >= 0; i--) {
        Square sq = Square(i, 0);
        while (fen[ind] != '/' && fen[ind] != ' ') {
            if (fen[ind] < '0' || '9' < fen[ind]) {
                place_piece_at_sq(cod[fen[ind]], sq);
                sq++;
            }
            else {
                int nr = fen[ind] - '0';
                while (nr)
                    board[sq++] = NO_PIECE, nr--;
            }
            ind++;
        }
        ind++;
    }
    turn = fen[ind] == 'w';

    key() ^= turn;

    castle_rights() = 0;
    ind += 2;
    rookSq[WHITE][0] = rookSq[WHITE][1] = rookSq[BLACK][0] = rookSq[BLACK][1] = 64;
    if (fen[ind] != 'K' && fen[ind] != 'Q' && fen[ind] != 'k' && fen[ind] != 'q' && fen[ind] != '-') { // most likely chess 960
        int kingSq = get_king(WHITE);
        if ('A' <= fen[ind] && fen[ind] <= 'H' && fen[ind] - 'A' > kingSq)
            castle_rights() |= (1 << 3), key() ^= castleKey[WHITE][1], rookSq[WHITE][1] = fen[ind++] - 'A';
        if ('A' <= fen[ind] && fen[ind] <= 'H' && kingSq > fen[ind] - 'A')
            castle_rights() |= (1 << 2), key() ^= castleKey[WHITE][0], rookSq[WHITE][0] = fen[ind++] - 'A';
        kingSq = get_king(BLACK);
        if ('a' <= fen[ind] && fen[ind] <= 'h' && 56 + fen[ind] - 'a' > kingSq)
            castle_rights() |= (1 << 1), key() ^= castleKey[BLACK][1], rookSq[BLACK][1] = 56 + fen[ind++] - 'a';
        if ('a' <= fen[ind] && fen[ind] <= 'h' && kingSq > 56 + fen[ind] - 'a')
            castle_rights() |= (1 << 0), key() ^= castleKey[BLACK][0], rookSq[BLACK][0] = 56 + fen[ind++] - 'a';
        chess960 = true;
    }
    else {
    const std::string castle_ch = "qkQK";
        for (int i = 3; i >= 0; i--) {
            if (fen[ind] == castle_ch[i]) {
                castle_rights() |= (1 << i);
                key() ^= castleKey[i / 2][i % 2];
                ind++;
            }
        }
        if (fen[ind] == '-') ind++;

        Square a = NO_SQUARE, b = NO_SQUARE;
        for (Square i = Squares::A1; i <= Squares::H1; i++) {
            if (piece_at(i) == Pieces::WhiteRook) {
                a = b;
                b = i;
            }
        }

        for (auto& rook : { a, b }) {
            if (rook != NO_SQUARE) {
                if (rook % 8 && rook % 8 != 7) chess960 = true;
                if (rook < get_king(WHITE) && (castle_rights() & 4)) rookSq[WHITE][0] = rook;
                if (get_king(WHITE) < rook && (castle_rights() & 8)) rookSq[WHITE][1] = rook;
            }
        }
        a = NO_SQUARE, b = NO_SQUARE;
        for (Square i = Squares::A8; i <= Squares::H8; i++) {
            if (piece_at(i) == Pieces::BlackRook) {
                b = a;
                a = i;
            }
        }
        for (auto& rook : { a, b }) {
            if (rook != NO_SQUARE) {
                if (rook % 8 && rook % 8 != 7) chess960 = true;
                if (rook < get_king(BLACK) && (castle_rights() & 1)) rookSq[BLACK][0] = rook;
                if (get_king(BLACK) < rook && (castle_rights() & 2)) rookSq[BLACK][1] = rook;
            }
        }
    }
    fill_multiarray<uint8_t, 2, 64>(castleRightsDelta, 15);
    if (rookSq[BLACK][0] != NO_SQUARE)
        castleRightsDelta[BLACK][rookSq[BLACK][0]] = 14, castleRightsDelta[BLACK][get_king(BLACK)] = 12;
    if (rookSq[BLACK][1] != NO_SQUARE)
        castleRightsDelta[BLACK][rookSq[BLACK][1]] = 13, castleRightsDelta[BLACK][get_king(BLACK)] = 12;
    if (rookSq[WHITE][0] != NO_SQUARE)
        castleRightsDelta[WHITE][rookSq[WHITE][0]] = 11, castleRightsDelta[WHITE][get_king(WHITE)] = 3;
    if (rookSq[WHITE][1] != NO_SQUARE)
        castleRightsDelta[WHITE][rookSq[WHITE][1]] = 7, castleRightsDelta[WHITE][get_king(WHITE)] = 3;
    
    ind++;
    if (fen[ind] != '-') {
        enpas() = Square(fen[ind + 1] - '1', fen[ind] - 'a');
        ind += 3;
        key() ^= enPasKey[enpas()];
    }
    else {
        enpas() = NO_SQUARE;
        ind += 2;
    }

    int nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9') nr = nr * 10 + fen[ind++] - '0';
    half_moves() = nr;

    ind++;
    nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9') nr = nr * 10 + fen[ind++] - '0';
    move_index() = nr;

    checkers() = get_attackers(1 ^ turn, pieces[WHITE] | pieces[BLACK], get_king(turn));
    pinned_pieces() = get_pinned_pieces();
}

void Board::set_frc_side(bool color, int idx) {
    Square ind = color == WHITE ? Squares::A1 : Squares::A8;

    place_piece_at_sq(Piece(PieceTypes::BISHOP, color), ind + 1 + (idx % 4) * 2);
    idx /= 4;
    place_piece_at_sq(Piece(PieceTypes::BISHOP, color), ind + 0 + (idx % 4) * 2);
    idx /= 4;
    int cnt = 0;
    for (Square i = ind; i < ind + 8; i++) {
        if (piece_at(i) == NO_PIECE) {
            if (idx % 6 == cnt) {
                place_piece_at_sq(Piece(PieceTypes::QUEEN, color), i);
                break;
            }
            cnt++;
        }
    }
    idx /= 6;

    constexpr int vals[10][2] = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4},
        {1, 2}, {1, 3}, {1, 4},
        {2, 3}, {2, 4},
        {3, 4},
    };
    cnt = 0;
    for (Square i = ind; i < ind + 8; i++) {
        if (piece_at(i) == NO_PIECE) {
            if (cnt == vals[idx][0] || cnt == vals[idx][1]) {
                place_piece_at_sq(Piece(PieceTypes::KNIGHT, color), i);
            }
            cnt++;
        }
    }
    cnt = 0;
    for (Square i = ind; i < ind + 8; i++) {
        if (piece_at(i) == NO_PIECE) {
            if (cnt == 0 || cnt == 2) {
                place_piece_at_sq(Piece(PieceTypes::ROOK, color), i);
            }
            else {
                place_piece_at_sq(Piece(PieceTypes::KING, color), i);
            }
            cnt++;
        }
    }
}

void Board::set_dfrc(int idx) {
    ply = game_ply = 0;
    captured() = NO_PIECE;

    bb.fill(Bitboard());
    pieces.fill(Bitboard());
    board.fill(NO_PIECE);

    int idxw = idx / 960, idxb = idx % 960;
    set_frc_side(WHITE, idxw);
    set_frc_side(BLACK, idxb);

    for (Square i = Squares::A2; i <= Squares::H2; i++) place_piece_at_sq(Pieces::WhitePawn, i);
    for (Square i = Squares::A7; i <= Squares::H7; i++) place_piece_at_sq(Pieces::BlackPawn, i);

    turn = WHITE;
    key() ^= turn;

    castle_rights() = 15;

    Square a = NO_SQUARE, b = NO_SQUARE;
    for (Square i = Squares::A1; i <= Squares::H1; i++) {
        if (piece_at(i) == Pieces::WhiteRook) {
            a = b;
            b = i;
        }
    }

    for (auto& rook : { a, b }) {
        if (rook != NO_SQUARE) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < get_king(WHITE) && (castle_rights() & 4)) rookSq[WHITE][0] = rook;
            if (get_king(WHITE) < rook && (castle_rights() & 8)) rookSq[WHITE][1] = rook;
        }
    }
    a = NO_SQUARE, b = NO_SQUARE;
    for (Square i = Squares::A8; i <= Squares::H8; i++) {
        if (piece_at(i) == Pieces::BlackRook) {
            b = a;
            a = i;
        }
    }
    for (auto& rook : { a, b }) {
        if (rook != NO_SQUARE) {
            if (rook % 8 && rook % 8 != 7) chess960 = true;
            if (rook < get_king(BLACK) && (castle_rights() & 1)) rookSq[BLACK][0] = rook;
            if (get_king(BLACK) < rook && (castle_rights() & 2)) rookSq[BLACK][1] = rook;
        }
    }
    fill_multiarray<uint8_t, 2, 64>(castleRightsDelta, 15);
    if (rookSq[BLACK][0] != NO_SQUARE)
        castleRightsDelta[BLACK][rookSq[BLACK][0]] = 14, castleRightsDelta[BLACK][get_king(BLACK)] = 12;
    if (rookSq[BLACK][1] != NO_SQUARE)
        castleRightsDelta[BLACK][rookSq[BLACK][1]] = 13, castleRightsDelta[BLACK][get_king(BLACK)] = 12;
    if (rookSq[WHITE][0] != NO_SQUARE)
        castleRightsDelta[WHITE][rookSq[WHITE][0]] = 11, castleRightsDelta[WHITE][get_king(WHITE)] = 3;
    if (rookSq[WHITE][1] != NO_SQUARE)
        castleRightsDelta[WHITE][rookSq[WHITE][1]] = 7, castleRightsDelta[WHITE][get_king(WHITE)] = 3;

    enpas() = NO_SQUARE;
    half_moves() = 0;
    move_index() = 1;
    checkers() = get_attackers(1 ^ turn, pieces[WHITE] | pieces[BLACK], get_king(turn));
    pinned_pieces() = get_pinned_pieces();
}

bool Board::is_material_draw() {
    /// KvK, KBvK, KNvK, KNNvK
    const int num = (get_bb_color(WHITE) | get_bb_color(BLACK)).count();
    return num == 2 || 
        (num == 3 && (get_bb_piece_type(PieceTypes::BISHOP) || get_bb_piece_type(PieceTypes::KNIGHT))) ||
        (num == 4 && (bb[Pieces::WhiteKnight].count() == 2 || bb[Pieces::BlackKnight].count() == 2));
}

bool Board::is_repetition(const int ply) {
    int cnt = 1;
    for (int i = 2; i <= game_ply && i <= half_moves(); i += 2) {
        if (history[game_ply - i].key == key()) {
            cnt++;
            if (ply > i || cnt == 3) return true;
        }
    }
    return false;
}

bool Board::is_draw(const int ply) {
    if (half_moves() < 100 || !checkers()) return is_material_draw() || is_repetition(ply) || half_moves() >= 100;
    MoveList moves;
    return gen_legal_moves(moves) > 0;
}

bool Board::has_upcoming_repetition(const int ply) {
    const Bitboard all_pieces = get_bb_color(WHITE) | get_bb_color(BLACK);
    uint64_t b = ~(key() ^ history[game_ply - 1].key);
    for (int i = 3; i <= half_moves() && i <= game_ply; i += 2) {
        b ^= ~(history[game_ply - i].key ^ history[game_ply - i + 1].key);
        if (b) continue;
        const uint64_t key_delta = key() ^ history[game_ply - i].key;
        int cuckoo_ind = cuckoo::hash1(key_delta);

        if (cuckoo::cuckoo[cuckoo_ind] != key_delta) cuckoo_ind = cuckoo::hash2(key_delta);
        if (cuckoo::cuckoo[cuckoo_ind] != key_delta) continue;

        const Move move = cuckoo::cuckoo_move[cuckoo_ind];
        const Square from = move.get_from(), to = move.get_to();
        if ((between_mask[from][to] ^ Bitboard(to)) & all_pieces) continue;
        if (ply > i) return true;
        const Piece piece = piece_at(from) != NO_PIECE ? piece_at(from) : piece_at(to);
        return piece != NO_PIECE && piece.color() == turn; 
    }
    return false;
}

uint64_t Board::speculative_next_key(const Move move) {
    const int from = move.get_from(), to = move.get_to();
    const Piece piece = piece_at(from);
    return key() ^ hashKey[piece][from] ^ hashKey[piece][to] ^ (piece_at(to) != NO_PIECE ? hashKey[piece_at(to)][to] : 0) ^ 1;
}