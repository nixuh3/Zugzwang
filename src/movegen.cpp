#include "pch.h"
#include "bitboard.h"
#include "board.h"
#include "movegen.h"

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

    Bitboard bb = pos.byTypeBB[Pt] & pos.byColorBB[Us];

    while (bb) {
        Square from = PopLsb(bb);
        Bitboard b = Bitboards::GetAttacks<Pt>(from, pos.byTypeBB[ALL_PIECES]) & target;

        SplatMoves(list, from, b);
    }
}

template <Color Us>
void GeneratePawnMoves(const Position& pos, MoveList& list) {
    const Piece pawn = MakePiece(Us, PAWN);
    const int pieceNb = pos.pieceNb[pawn];

    const Rank startRank = RelativeRank(Us, RANK_2);
    const Rank promoRank = RelativeRank(Us, RANK_7);

    auto add_promotions = [&](Square startSq, Square toSq) {
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, QUEEN));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, ROOK));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, BISHOP));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, KNIGHT));
    };

    Bitboard bb = pos.byTypeBB[PAWN] & pos.byColorBB[Us];

    while (bb) {
        Square from = PopLsb(bb);
        const Rank rank = RankOf(from);
        const Square oneForward = from + PawnPush(Us);

        assert(IsOk(oneForward));

        // pushes
        if (pos.board[oneForward] == NO_PIECE) {
            if (rank == promoRank) {
                add_promotions(from, oneForward);
            } else {
                const Square twoForward = from + 2 * PawnPush(Us);
                assert(IsOk(twoForward));

                list.Insert(Move(from, oneForward));
                if (rank == startRank && pos.board[twoForward] == NO_PIECE) {
                    list.Insert(Move(from, twoForward));
                }
            }
        }

        // captures
        const Bitboard pawnAtt = Bitboards::GetAttacks<PAWN>(from, 0, Us);
        Bitboard captures = pawnAtt & pos.byColorBB[~Us];

        while (captures) {
            Square to = PopLsb(captures);
            if (rank == promoRank) {
                add_promotions(from, to);
            } else {
                list.Insert(Move(from, to));
            }
        }

        // en passant
        if (pos.epSquare != SQ_NONE && (pawnAtt & (1ULL << pos.epSquare))) {
            list.Insert(Move::Make<EN_PASSANT>(from, pos.epSquare));
        }
    }
}

void GenerateKingMoves(const Position& pos, MoveList& list) {
    const Color color = pos.sideToMove;
    Square startSq = pos.kingSquare[color];

    Bitboard attacks = Bitboards::GetAttacks<KING>(startSq) & ~pos.byColorBB[color];
    SplatMoves(list, startSq, attacks);

    // castling
    if (color == WHITE) {
        if (pos.castlingRights & WHITE_OO) {
            if (pos.board[SQ_F1] == NO_PIECE && pos.board[SQ_G1] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_F1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_G1));
                }
            }
        }

        if (pos.castlingRights & WHITE_OOO) {
            if (pos.board[SQ_D1] == NO_PIECE && pos.board[SQ_C1] == NO_PIECE &&
                pos.board[SQ_B1] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_D1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_C1));
                }
            }
        }
    } else {
        if (pos.castlingRights & BLACK_OO) {
            if (pos.board[SQ_F8] == NO_PIECE && pos.board[SQ_G8] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(pos, SQ_E8, WHITE) &&
                    !MoveGen::IsSquareAttacked(pos, SQ_F8, WHITE)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E8, SQ_G8));
                }
            }
        }

        if (pos.castlingRights & BLACK_OOO) {
            if (pos.board[SQ_D8] == NO_PIECE && pos.board[SQ_C8] == NO_PIECE &&
                pos.board[SQ_B8] == NO_PIECE) {
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
    const Bitboard target = ~pos.byColorBB[Us];

    GenerateMoves<Us, QUEEN>(pos, list, target);
    GenerateMoves<Us, BISHOP>(pos, list, target);
    GenerateMoves<Us, ROOK>(pos, list, target);
    GenerateMoves<Us, KNIGHT>(pos, list, target);
    GeneratePawnMoves<Us>(pos, list);
    GenerateKingMoves(pos, list);
}

} // namespace

namespace MoveGen {

bool IsSquareAttacked(const Position& pos, Square sq, Color attacker) {
    const Bitboard attackers = pos.byColorBB[attacker];
    const Bitboard occ = pos.byTypeBB[ALL_PIECES];

    if (Bitboards::GetAttacks<PAWN>(sq, 0, ~attacker) & attackers & pos.byTypeBB[PAWN]) {
        return true;
    }

    if (Bitboards::GetAttacks<KNIGHT>(sq) & attackers & pos.byTypeBB[KNIGHT]) {
        return true;
    }

    if (Bitboards::GetAttacks<KING>(sq) & attackers & pos.byTypeBB[KING]) {
        return true;
    }

    if (Bitboards::GetAttacks<BISHOP>(sq, occ) & attackers & pos.byTypeBB[BISHOP]) {
        return true;
    }

    if (Bitboards::GetAttacks<ROOK>(sq, occ) & attackers & pos.byTypeBB[ROOK]) {
        return true;
    }

    if (Bitboards::GetAttacks<QUEEN>(sq, occ) & attackers & pos.byTypeBB[QUEEN]) {
        return true;
    }

    return false;
}

void GeneratePseudo(const Position& pos, MoveList& list) {
    pos.sideToMove == WHITE ? GeneratePseudoMoves<WHITE>(pos, list)
                            : GeneratePseudoMoves<BLACK>(pos, list);
}

} // namespace MoveGen
} // namespace Zugzwang