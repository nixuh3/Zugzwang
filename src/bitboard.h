#pragma once

#include "types.h"
#include <bit>

namespace Zugzwang {

namespace Bitboards {

void Init();

template <PieceType P>
Bitboard GetAttacks(Square square, Bitboard occupancy = 0, Color color = WHITE);

}

constexpr Bitboard SquareBb(Square s) {
    assert(IsOk(s));
    return (1ULL << s);
}

constexpr Bitboard operator&(Bitboard b, Square s) { return b & SquareBb(s); }
constexpr Bitboard operator|(Bitboard b, Square s) { return b | SquareBb(s); }
constexpr Bitboard operator^(Bitboard b, Square s) { return b ^ SquareBb(s); }
constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= SquareBb(s); }
constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b ^= SquareBb(s); }

constexpr Bitboard operator|(Square s1, Square s2) { return SquareBb(s1) | s2; }

inline Square Lsb(Bitboard b) {
    assert(b != 0);
    return static_cast<Square>(std::countr_zero(b));
}

inline Square PopLsb(Bitboard& b) {
    assert(b);
    Square index = Square(Lsb(b));
    b &= (b - 1);
    return index;
}

constexpr int Popcount(Bitboard b) { return std::popcount(b); }

}