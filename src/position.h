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

    bool MakeMove(Move move);
    void UnmakeMove(Move move);

    void Print() const;

    void PerftTest(int depth);

    Bitboard Pieces() const { return m_byTypeBB[ALL_PIECES]; }
    Bitboard Pieces(Color c) const { return m_byColorBB[c]; }

    template <typename... PieceTypes>
    Bitboard Pieces(PieceTypes... pts) const {
        return (m_byTypeBB[pts] | ...);
    }

    template <typename... PieceTypes>
    Bitboard Pieces(Color c, PieceTypes... pts) const {
        return Pieces(c) & Pieces(pts...);
    }

    template <PieceType Pt>
    int Count(Color c) const {
        return m_pieceNb[MakePiece(c, Pt)];
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
        return m_board[sq];
    }

    Value GetMaterial(Color c) const { return m_material[c]; }

    Color SideToMove() const { return m_sideToMove; }
    Square EpSuare() const { return m_epSquare; }
    bool CanCastle(CastlingRights cr) const { return m_castlingRights & cr; }

  private:
    void putPiece(Piece piece, Square sq);
    void removePiece(Square sq);
    void movePiece(Square from, Square to);

    void generatePosKey();
    void reset();
    void updateListsBitboards();
    void perft(int depth);

    Piece m_board[SQUARE_NB];
    int m_pieceNb[PIECE_NB];
    Color m_sideToMove;
    Bitboard m_byColorBB[COLOR_NB];
    Bitboard m_byTypeBB[PIECE_TYPE_NB];
    Value m_material[COLOR_NB];

    Square m_epSquare;
    int m_rule50;
    int m_gamePly;
    int m_castlingRights;
    Key m_posKey;

    uint64_t m_perftLealNodes;

    StateInfo m_history[MAX_PLIES];
};

}
