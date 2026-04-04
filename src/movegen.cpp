#include "pch.h"
#include "bitboard.h"
#include "movegen.h"
#include "position.h"

namespace Zugzwang {
namespace {

inline void SplatMoves(MoveList& list, Square from, Bitboard toBb) {
    while (toBb) {
        list.Insert(Move(from, PopLsb(toBb)));
    }
}

template <Color Us, PieceType Pt>
void GenerateMoves(const Position& pos, MoveList& list, Bitboard target) {
    static_assert(Pt != KING && Pt != PAWN, "Unsupported piece type in GenerateMoves()");

    Bitboard bb = pos.Pieces(Us, Pt);

    while (bb) {
        Square from = PopLsb(bb);
        Bitboard b = Bitboards::GetAttacks<Pt>(from, pos.Pieces()) & target;

        SplatMoves(list, from, b);
    }
}

template <Color Us>
void GeneratePawnMoves(const Position& pos, MoveList& list) {
    constexpr Piece pawn = MakePiece(Us, PAWN);
    constexpr Rank startRank = RelativeRank(Us, RANK_2);
    constexpr Rank promoRank = RelativeRank(Us, RANK_7);

    auto addPromotions = [&](Square startSq, Square toSq) {
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, QUEEN));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, ROOK));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, BISHOP));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, KNIGHT));
    };

    Bitboard bb = pos.Pieces(Us, PAWN);

    while (bb) {
        Square from = PopLsb(bb);
        const Rank rank = RankOf(from);
        const Square oneForward = from + PawnPush(Us);

        assert(IsOk(oneForward));

        // pushes
        if (pos.PieceOn(oneForward) == NO_PIECE) {
            if (rank == promoRank) {
                addPromotions(from, oneForward);
            } else {
                const Square twoForward = from + 2 * PawnPush(Us);
                assert(IsOk(twoForward));

                list.Insert(Move(from, oneForward));
                if (rank == startRank && pos.PieceOn(twoForward) == NO_PIECE) {
                    list.Insert(Move(from, twoForward));
                }
            }
        }

        // captures
        const Bitboard pawnAtt = Bitboards::GetAttacks<PAWN>(from, 0, Us);
        Bitboard captures = pawnAtt & pos.Pieces(~Us);

        while (captures) {
            Square to = PopLsb(captures);
            if (rank == promoRank) {
                addPromotions(from, to);
            } else {
                list.Insert(Move(from, to));
            }
        }

        // en passant
        if (pos.EpSuare() != SQ_NONE && (pawnAtt & (1ULL << pos.EpSuare()))) {
            list.Insert(Move::Make<EN_PASSANT>(from, pos.EpSuare()));
        }
    }
}

template <Color Us>
void GenerateKingMoves(const Position& pos, MoveList& list) {
    Square startSq = pos.SquareOf<KING>(Us);

    Bitboard attacks = Bitboards::GetAttacks<KING>(startSq) & ~pos.Pieces(Us);
    SplatMoves(list, startSq, attacks);

    // castling
    if constexpr (Us == WHITE) {
        if (pos.CanCastle(WHITE_OO)) {
            if (pos.PieceOn(SQ_F1) == NO_PIECE && pos.PieceOn(SQ_G1) == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_F1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_G1));
                }
            }
        }

        if (pos.CanCastle(WHITE_OOO)) {
            if (pos.PieceOn(SQ_D1) == NO_PIECE && pos.PieceOn(SQ_C1) == NO_PIECE &&
                pos.PieceOn(SQ_B1) == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_D1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_C1));
                }
            }
        }
    } else {
        if (pos.CanCastle(BLACK_OO)) {
            if (pos.PieceOn(SQ_F8) == NO_PIECE && pos.PieceOn(SQ_G8) == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E8, WHITE) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_F8, WHITE)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E8, SQ_G8));
                }
            }
        }

        if (pos.CanCastle(BLACK_OOO)) {
            if (pos.PieceOn(SQ_D8) == NO_PIECE && pos.PieceOn(SQ_C8) == NO_PIECE &&
                pos.PieceOn(SQ_B8) == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E8, WHITE) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_D8, WHITE)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E8, SQ_C8));
                }
            }
        }
    }
}

template <Color Us>
void GeneratePseudoMoves(const Position& pos, MoveList& list) {
    const Bitboard target = ~pos.Pieces(Us);

    GeneratePawnMoves<Us>(pos, list);
    GenerateMoves<Us, KNIGHT>(pos, list, target);
    GenerateMoves<Us, BISHOP>(pos, list, target);
    GenerateMoves<Us, ROOK>(pos, list, target);
    GenerateMoves<Us, QUEEN>(pos, list, target);
    GenerateKingMoves<Us>(pos, list);
}

} // namespace

namespace MoveGen {

bool IsSquareAttacked(const Position& pos, Square sq, Color attacker) {
    const Bitboard occ = pos.Pieces();

    return (Bitboards::GetAttacks<BISHOP>(sq, occ) & pos.Pieces(attacker, BISHOP, QUEEN)) ||
        (Bitboards::GetAttacks<ROOK>(sq, occ) & pos.Pieces(attacker, ROOK, QUEEN)) ||
        (Bitboards::GetAttacks<KNIGHT>(sq) & pos.Pieces(attacker, KNIGHT)) ||
        (Bitboards::GetAttacks<PAWN>(sq, 0, ~attacker) & pos.Pieces(attacker, PAWN)) ||
        (Bitboards::GetAttacks<KING>(sq) & pos.Pieces(attacker, KING));
}

void GeneratePseudo(const Position& pos, MoveList& list) {
    pos.SideToMove() == WHITE ? GeneratePseudoMoves<WHITE>(pos, list)
                              : GeneratePseudoMoves<BLACK>(pos, list);
}

} // namespace MoveGen
} // namespace Zugzwang