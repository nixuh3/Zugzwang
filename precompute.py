def generate_pawn_attacks():
    # Helper to mask files to prevent wrapping
    not_a_file = 0xfefefefefefefefe
    not_h_file = 0x7f7f7f7f7f7f7f7f

    attacks = [[0] * 64 for _ in range(2)]

    for sq in range(64):
        bitboard = 1 << sq

        # White Pawn Attacks (Index 0) - Moving Up
        # Northeast (up-right) and Northwest (up-left)
        white_attacks = 0
        if (bitboard << 9) & 0xFFFFFFFFFFFFFFFF:
            white_attacks |= (bitboard << 9) & not_a_file
        if (bitboard << 7) & 0xFFFFFFFFFFFFFFFF:
            white_attacks |= (bitboard << 7) & not_h_file
        attacks[0][sq] = white_attacks

        # Black Pawn Attacks (Index 1) - Moving Down
        # Southeast (down-right) and Southwest (down-left)
        black_attacks = 0
        if (bitboard >> 7) & 0xFFFFFFFFFFFFFFFF:
            black_attacks |= (bitboard >> 7) & not_a_file
        if (bitboard >> 9) & 0xFFFFFFFFFFFFFFFF:
            black_attacks |= (bitboard >> 9) & not_h_file
        attacks[1][sq] = black_attacks

    # Format the output for C++
    print("constexpr Bitboard PawnAttacks[2][64] = {")
    for side in range(2):
        print("    { ", end="")
        for sq in range(64):
            comma = ", " if sq < 63 else ""
            print(f"0x{attacks[side][sq]:X}ULL{comma}", end="")
        print(" }" + ("," if side == 0 else ""))
    print("};")


if __name__ == "__main__":
    pass
