#pragma once
#include "board.h"

std::string Board::fen()
{
    std::string fen;
    for (int i = 7; i >= 0; i--)
    {
        int cnt = 0;
        for (Square sq = Square(i, 0); sq < Square(i + 1, 0); sq++)
        {
            if (piece_at(sq) == NO_PIECE)
                cnt++;
            else
            {
                if (cnt)
                    fen += char(cnt + '0');
                cnt = 0;
                fen += piece_char[piece_at(sq)];
            }
        }
        if (cnt)
            fen += char(cnt + '0');
        if (i)
            fen += "/";
    }
    fen += " ";
    fen += (turn == WHITE ? "w" : "b");
    fen += " ";
    const std::string castle_ch = "qkQK";
    bool has_castling_rights = false;
    for (int i = 3; i >= 0; i--)
    {
        if (rook_sq(i / 2, i % 2) != NO_SQUARE)
            fen += castle_ch[i], has_castling_rights = true;
    }
    if (!has_castling_rights)
        fen += "-";
    fen += " ";
    if (enpas() != NO_SQUARE)
    {
        fen += char('a' + enpas() % 8);
        fen += char('1' + enpas() / 8);
    }
    else
        fen += "-";
    fen += " " + std::to_string(half_moves()) + " " + std::to_string(move_index());
    return fen;
}

void Board::set_fen(const std::string fen, HistoricalState &next_state)
{
    int ind = 0;
    state = &next_state;

    key() = pawn_key() = mat_key(WHITE) = mat_key(BLACK) = 0;
    ply = game_ply = 0;
    captured() = NO_PIECE;

    bb.fill(Bitboard(0ull));
    pieces.fill(Bitboard(0ull));

    for (int i = 7; i >= 0; i--)
    {
        Square sq = Square(i, 0);
        while (fen[ind] != '/' && fen[ind] != ' ')
        {
            if (fen[ind] < '0' || '9' < fen[ind])
            {
                place_piece_at_sq(cod[fen[ind]], sq);
                sq++;
            }
            else
            {
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

    ind += 2;
    rook_sq(WHITE, 0) = rook_sq(WHITE, 1) = rook_sq(BLACK, 0) = rook_sq(BLACK, 1) = NO_SQUARE;
    if (fen[ind] != 'K' && fen[ind] != 'Q' && fen[ind] != 'k' && fen[ind] != 'q' && fen[ind] != '-')
    { // most likely chess 960
        int kingSq = get_king(WHITE);
        if ('A' <= fen[ind] && fen[ind] <= 'H' && fen[ind] - 'A' > kingSq)
            key() ^= castleKey[WHITE][1], rook_sq(WHITE, 1) = fen[ind++] - 'A';
        if ('A' <= fen[ind] && fen[ind] <= 'H' && kingSq > fen[ind] - 'A')
            key() ^= castleKey[WHITE][0], rook_sq(WHITE, 0) = fen[ind++] - 'A';
        kingSq = get_king(BLACK);
        if ('a' <= fen[ind] && fen[ind] <= 'h' && 56 + fen[ind] - 'a' > kingSq)
            key() ^= castleKey[BLACK][1], rook_sq(BLACK, 1) = 56 + fen[ind++] - 'a';
        if ('a' <= fen[ind] && fen[ind] <= 'h' && kingSq > 56 + fen[ind] - 'a')
            key() ^= castleKey[BLACK][0], rook_sq(BLACK, 0) = 56 + fen[ind++] - 'a';
        chess960 = true;
    }
    else
    {
        const std::string castle_ch = "qkQK";
        uint8_t castle_rights = 0;
        for (int i = 3; i >= 0; i--)
        {
            if (fen[ind] == castle_ch[i])
            {
                castle_rights |= (1 << i);
                key() ^= castleKey[i / 2][i % 2];
                ind++;
            }
        }
        if (fen[ind] == '-')
            ind++;

        Square a = NO_SQUARE, b = NO_SQUARE;
        for (Square i = Squares::A1; i <= Squares::H1; i++)
        {
            if (piece_at(i) == Pieces::WhiteRook)
            {
                a = b;
                b = i;
            }
        }

        for (auto &rook : {a, b})
        {
            if (rook != NO_SQUARE)
            {
                if (rook < get_king(WHITE) && (castle_rights & 4))
                {
                    rook_sq(WHITE, 0) = rook;
                    if (rook % 8 && rook % 8 != 7)
                        chess960 = true;
                }
                if (get_king(WHITE) < rook && (castle_rights & 8))
                {
                    rook_sq(WHITE, 1) = rook;
                    if (rook % 8 && rook % 8 != 7)
                        chess960 = true;
                }
            }
        }
        a = NO_SQUARE, b = NO_SQUARE;
        for (Square i = Squares::A8; i <= Squares::H8; i++)
        {
            if (piece_at(i) == Pieces::BlackRook)
            {
                b = a;
                a = i;
            }
        }
        for (auto &rook : {a, b})
        {
            if (rook != NO_SQUARE)
            {
                if (rook < get_king(BLACK) && (castle_rights & 1))
                {
                    rook_sq(BLACK, 0) = rook;
                    if (rook % 8 && rook % 8 != 7)
                        chess960 = true;
                }
                if (get_king(BLACK) < rook && (castle_rights & 2))
                {
                    rook_sq(BLACK, 1) = rook;
                    if (rook % 8 && rook % 8 != 7)
                        chess960 = true;
                }
            }
        }
        if (((castle_rights & 12) && get_king(WHITE) % 8 != 4) || ((castle_rights & 3) && get_king(BLACK) % 8 != 4))
            chess960 = true;
    }

    ind++;
    if (fen[ind] != '-')
    {
        enpas() = Square(fen[ind + 1] - '1', fen[ind] - 'a');
        ind += 3;
        key() ^= enPasKey[enpas()];
    }
    else
    {
        enpas() = NO_SQUARE;
        ind += 2;
    }

    int nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9')
        nr = nr * 10 + fen[ind++] - '0';
    half_moves() = nr;

    ind++;
    nr = 0;
    while ('0' <= fen[ind] && fen[ind] <= '9')
        nr = nr * 10 + fen[ind++] - '0';
    move_index() = nr;

    get_pinned_pieces_and_checkers();
    get_threats(turn);
}

void Board::set_frc_side(bool color, int idx)
{
    Square ind = Squares::A1.mirror(color);

    place_piece_at_sq(Piece(PieceTypes::BISHOP, color), ind + 1 + (idx % 4) * 2);
    idx /= 4;
    place_piece_at_sq(Piece(PieceTypes::BISHOP, color), ind + 0 + (idx % 4) * 2);
    idx /= 4;
    int cnt = 0;
    for (Square i = ind; i < ind + 8; i++)
    {
        if (piece_at(i) == NO_PIECE)
        {
            if (idx % 6 == cnt)
            {
                place_piece_at_sq(Piece(PieceTypes::QUEEN, color), i);
                break;
            }
            cnt++;
        }
    }
    idx /= 6;

    constexpr int vals[10][2] = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, {1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 4},
    };
    cnt = 0;
    for (Square i = ind; i < ind + 8; i++)
    {
        if (piece_at(i) == NO_PIECE)
        {
            if (cnt == vals[idx][0] || cnt == vals[idx][1])
            {
                place_piece_at_sq(Piece(PieceTypes::KNIGHT, color), i);
            }
            cnt++;
        }
    }
    cnt = 0;
    for (Square i = ind; i < ind + 8; i++)
    {
        if (piece_at(i) == NO_PIECE)
        {
            if (cnt == 0 || cnt == 2)
            {
                place_piece_at_sq(Piece(PieceTypes::ROOK, color), i);
                rook_sq(color, cnt / 2) = i;
                key() ^= castleKey[color][cnt / 2];
            }
            else
            {
                place_piece_at_sq(Piece(PieceTypes::KING, color), i);
            }
            cnt++;
        }
    }
}

void Board::set_dfrc(int idx, HistoricalState &next_state)
{
    state = &next_state;
    chess960 = true;

    key() = pawn_key() = mat_key(WHITE) = mat_key(BLACK) = 0;
    ply = game_ply = 0;
    captured() = NO_PIECE;

    bb.fill(Bitboard(0ull));
    pieces.fill(Bitboard(0ull));
    board.fill(NO_PIECE);

    int idxw = idx / 960, idxb = idx % 960;
    set_frc_side(WHITE, idxw);
    set_frc_side(BLACK, idxb);

    for (Square i = Squares::A2; i <= Squares::H2; i++)
        place_piece_at_sq(Pieces::WhitePawn, i);
    for (Square i = Squares::A7; i <= Squares::H7; i++)
        place_piece_at_sq(Pieces::BlackPawn, i);

    turn = WHITE;
    key() ^= turn;
    enpas() = NO_SQUARE;
    half_moves() = 0;
    move_index() = 1;
    get_pinned_pieces_and_checkers();
    get_threats(turn);
}