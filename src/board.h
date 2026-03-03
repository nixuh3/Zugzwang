#pragma once

#include "types.h"

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

    Piece board[SQUARE_NB];
    int pieceNb[PIECE_NB];
    Square kingSquare[COLOR_NB];
    Color sideToMove;
    Bitboard byColorBB[COLOR_NB];
    Bitboard byTypeBB[PIECE_TYPE_NB];

    Square epSquare;
    int rule50;
    int gamePly;
    int castlingRights;
    Key posKey;

  private:
    void putPiece(Piece piece, Square sq);
    void removePiece(Square sq);
    void movePiece(Square from, Square to);

    void generatePosKey();
    void reset();
    void updateListsBitboards();
    void perft(int depth);

    uint64_t perftLealNodes;

    StateInfo history[MAX_PLIES];
};

} // namespace Zugzwang
