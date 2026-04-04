#pragma once

#include "bitboard.h"
#include <string>

namespace Zugzwang {

struct StateInfo {
    Square epSquare;
    int rule50;
    int castlingRights;
    Piece captured;
    Key posKey;
};

class Position {
  public:
    Position();

    void ParseFen(const std::string& fen);

    bool MakeMove(const Move& move);
    void UnmakeMove(const Move& move);

    void Print() const;

    uint64_t PerftTest(int depth);

    Bitboard Pieces() const { return byTypeBB[ALL_PIECES]; }
    Bitboard Pieces(Color c) const { return byColorBB[c]; }

    template <typename... PieceTypes>
    Bitboard Pieces(PieceTypes... pts) const {
        return (byTypeBB[pts] | ...);
    }

    template <typename... PieceTypes>
    Bitboard Pieces(Color c, PieceTypes... pts) const {
        return Pieces(c) & Pieces(pts...);
    }

    template <PieceType Pt>
    int Count(Color c) const {
        return pieceNb[MakePiece(c, Pt)];
    }

    template <PieceType Pt>
    int Count() const {
        return Count<Pt>(WHITE) + Count<Pt>(BLACK);
    }

    template <PieceType Pt>
    Square SquareOf(Color c) const {
        assert(Count<Pt>(c) == 1);
        return Lsb(Pieces(c, Pt));
    }

    Piece PieceOn(Square sq) const {
        assert(IsOk(sq));
        return board[sq];
    }

    Color SideToMove() const { return sideToMove; }
    Square EpSuare() const { return epSquare; }
    bool CanCastle(CastlingRights cr) const { return castlingRights & cr; }

  private:
    void putPiece(Piece piece, Square sq);
    void removePiece(Square sq);
    void movePiece(Square from, Square to);

    void generatePosKey();
    void reset();
    void updateListsBitboards();
    void perft(int depth);

    Piece board[SQUARE_NB];
    int pieceNb[PIECE_NB];
    Color sideToMove;
    Bitboard byColorBB[COLOR_NB];
    Bitboard byTypeBB[PIECE_TYPE_NB];

    Square epSquare;
    int rule50;
    int gamePly;
    int castlingRights;
    Key posKey;

    uint64_t perftLealNodes;

    StateInfo history[MAX_PLIES];
};

} // namespace Zugzwang
