#pragma once

#include "bitboard.h"
#include <string>

namespace Zugzwang {

struct StateInfo {
    Square EpSquare;
    int Rule50;
    int CastlingRights;
    Piece Captured;
    Key PosKey;
};

class Position {
  public:
    Position();

    void ParseFen(const std::string& fen);

    bool MakeMove(const Move& move);
    void UnmakeMove(const Move& move);

    void Print() const;

    uint64_t PerftTest(int depth);

    Bitboard Pieces() const { return m_ByTypeBB[ALL_PIECES]; }
    Bitboard Pieces(Color c) const { return m_ByColorBB[c]; }

    template <typename... PieceTypes>
    Bitboard Pieces(PieceTypes... pts) const {
        return (m_ByTypeBB[pts] | ...);
    }

    template <typename... PieceTypes>
    Bitboard Pieces(Color c, PieceTypes... pts) const {
        return Pieces(c) & Pieces(pts...);
    }

    template <PieceType Pt>
    int Count(Color c) const {
        return m_PieceNb[MakePiece(c, Pt)];
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
        return m_Board[sq];
    }

    Color SideToMove() const { return m_SideToMove; }
    Square EpSuare() const { return m_EpSquare; }
    bool CanCastle(CastlingRights cr) const { return m_CastlingRights & cr; }

  private:
    void putPiece(Piece piece, Square sq);
    void removePiece(Square sq);
    void movePiece(Square from, Square to);

    void generatePosKey();
    void reset();
    void updateListsBitboards();
    void perft(int depth);

    Piece m_Board[SQUARE_NB];
    int m_PieceNb[PIECE_NB];
    Color m_SideToMove;
    Bitboard m_ByColorBB[COLOR_NB];
    Bitboard m_ByTypeBB[PIECE_TYPE_NB];

    Square m_EpSquare;
    int m_Rule50;
    int m_GamePly;
    int m_CastlingRights;
    Key m_PosKey;

    uint64_t m_PerftLealNodes;

    StateInfo m_History[MAX_PLIES];
};

} // namespace Zugzwang
