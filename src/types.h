#pragma once

#include <cassert>
#include <cstdint>

namespace Zugzwang {

// chess engine
using Bitboard = uint64_t;
using Key = uint64_t;

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLIES = 2048;

// clang-format off
enum PieceType {
    ALL_PIECES, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 8
};

enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

enum Color { WHITE, BLACK, COLOR_NB = 2 };

enum CastlingRights {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    CASTLING_RIGHT_NB = 16
};

enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE, SQUARE_NB = 64
};
// clang-format on

enum Direction : int {
    NORTH = 8,
    EAST = 1,
    SOUTH = -NORTH,
    WEST = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };

enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

#define ENABLE_INCR_OPERATORS_ON(T)                             \
    constexpr T& operator++(T& d) { return d = T(int(d) + 1); } \
    constexpr T& operator--(T& d) { return d = T(int(d) - 1); }

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_INCR_OPERATORS_ON

constexpr Direction operator+(Direction d1, Direction d2) { return Direction(int(d1) + int(d2)); }
constexpr Direction operator*(int i, Direction d) { return Direction(i * int(d)); }

// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

// Toggle color
constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

constexpr Square MakeSquare(File f, Rank r) { return Square((r << 3) + f); }

constexpr Piece MakePiece(Color c, PieceType pt) { return Piece((c << 3) + pt); }

constexpr PieceType TypeOf(Piece pc) { return PieceType(pc & 7); }

constexpr Color ColorOf(Piece pc) {
    assert(pc != NO_PIECE);
    return Color(pc >> 3);
}

constexpr bool IsOk(Square s) { return s >= SQ_A1 && s <= SQ_H8; }

constexpr File FileOf(Square s) { return File(s & 7); }

constexpr Rank RankOf(Square s) { return Rank(s >> 3); }

constexpr Rank RelativeRank(Color c, Rank r) { return Rank(r ^ (c * 7)); }

constexpr Direction PawnPush(Color c) { return c == WHITE ? NORTH : SOUTH; }

enum MoveType { NORMAL, PROMOTION = 1 << 14, EN_PASSANT = 2 << 14, CASTLING = 3 << 14 };

// 16 bits:
// bit  0- 5: destination square (from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
class Move {
    uint16_t data;

  public:
    Move() = default;

    constexpr explicit Move(uint16_t d) : data(d) {}

    constexpr Move(Square from, Square to) : data((from << 6) + to) {}

    template <MoveType T>
    static constexpr Move Make(Square from, Square to, PieceType pt = KNIGHT) {
        return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

    constexpr Square FromSq() const {
        assert(IsOk());
        return Square((data >> 6) & 0x3F);
    }

    constexpr Square ToSq() const {
        assert(IsOk());
        return Square(data & 0x3F);
    }

    constexpr MoveType TypeOf() const { return MoveType(data & (3 << 14)); }

    constexpr PieceType PromotionType() const { return PieceType(((data >> 12) & 3) + KNIGHT); }

    constexpr bool IsOk() const { return None().data != data; }

    static constexpr Move None() { return Move(0); }

    constexpr bool operator==(const Move& m) const { return data == m.data; }
    constexpr bool operator!=(const Move& m) const { return data != m.data; }

    constexpr explicit operator bool() const { return data != 0; }
};

class MoveList {
  public:
    MoveList() : count(0) {}

    void Insert(Move move) {
        assert(count < MAX_MOVES);
        moves[count++] = move;
    }

    Move& operator[](int i) {
        assert(i >= 0 && i < count);
        return moves[i];
    }

    // For non-const range-based for loops
    Move* begin() { return moves; }
    Move* end() { return moves + count; }

    // For const range-based for loops
    const Move* begin() const { return moves; }
    const Move* end() const { return moves + count; }

  private:
    Move moves[MAX_MOVES];
    int count;
};

} // namespace Zugzwang
