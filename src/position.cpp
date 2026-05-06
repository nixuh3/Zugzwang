#include "pch.h"
#include "bitboard.h"
#include "movegen.h"
#include "position.h"

namespace Zugzwang {
namespace {

constexpr auto PieceToChar = " PNBRQK  pnbrqk";

// clang-format off
constexpr int CastlePerm[SQUARE_NB] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,
};
// clang-format on

uint64_t Seed = 0x123456789ABCDEFULL;

uint64_t Splitmix64() {
    uint64_t z = (Seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

Key Psq[PIECE_NB][SQUARE_NB];
Key Castling[CASTLING_RIGHT_NB];
Key EnPassant[FILE_NB];
Key Side;

} // namespace

Position::Position() {
    for (int i = 0; i < PIECE_NB; ++i) {
        for (Square j = SQ_A1; j < SQUARE_NB; ++j) {
            Psq[i][j] = Splitmix64();
        }
    }

    for (File f = FILE_A; f < FILE_NB; ++f) {
        EnPassant[f] = Splitmix64();
    }

    Side = Splitmix64();

    for (int i = 0; i < CASTLING_RIGHT_NB; ++i) {
        Castling[i] = Splitmix64();
    }
}

bool Position::IsInCheck() const {
    return MoveGen::IsSquareAttacked(*this, SquareOf<KING>(m_sideToMove), ~m_sideToMove);
}

bool Position::IsRepetition() const {
    for (int i = m_gamePly - m_rule50; i < m_gamePly - 1; ++i) {
        if (m_posKey == m_history[i].posKey) {
            return true;
        }
    }
    return false;
}

void Position::putPiece(Piece piece, Square sq) {
    assert(piece != NO_PIECE);

    m_board[sq] = piece;
    m_posKey ^= Psq[piece][sq];
    m_pieceNb[piece]++;
    m_material[ColorOf(piece)] += PieceValue[piece];

    m_byColorBB[ColorOf(piece)] |= sq;
    m_byTypeBB[ALL_PIECES] |= m_byTypeBB[TypeOf(piece)] |= sq;
}

void Position::removePiece(Square sq) {
    Piece piece = m_board[sq];
    assert(piece != NO_PIECE);

    m_posKey ^= Psq[piece][sq];
    m_board[sq] = NO_PIECE;
    m_material[ColorOf(piece)] -= PieceValue[piece];

    m_byTypeBB[ALL_PIECES] ^= sq;
    m_byTypeBB[TypeOf(piece)] ^= sq;
    m_byColorBB[ColorOf(piece)] ^= sq;

    m_pieceNb[piece]--;
}

void Position::movePiece(Square from, Square to) {
    Piece piece = m_board[from];

    if (piece == NO_PIECE) {
        assert(false);
    }
    assert(piece != NO_PIECE);

    Bitboard fromTo = from | to;

    m_posKey ^= Psq[piece][from];
    m_posKey ^= Psq[piece][to];
    m_board[from] = NO_PIECE;
    m_board[to] = piece;

    m_byTypeBB[ALL_PIECES] ^= fromTo;
    m_byTypeBB[TypeOf(piece)] ^= fromTo;
    m_byColorBB[ColorOf(piece)] ^= fromTo;
}

void Position::generatePosKey() {
    Bitboard pieces = Pieces();
    while (pieces) {
        Square sq = PopLsb(pieces);
        m_posKey ^= Psq[m_board[sq]][sq];
    }

    if (m_sideToMove == WHITE) {
        m_posKey ^= Side;
    }

    if (m_epSquare != SQ_NONE) {
        m_posKey ^= EnPassant[FileOf(m_epSquare)];
    }

    m_posKey ^= Castling[m_castlingRights];
}

void Position::reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        m_board[i] = NO_PIECE;
    }

    for (int i = 0; i < PIECE_NB; ++i) {
        m_pieceNb[i] = 0;
    }

    for (int i = 0; i < PIECE_TYPE_NB; ++i) {
        m_byTypeBB[i] = 0ULL;
    }

    m_byColorBB[WHITE] = m_byColorBB[BLACK] = 0ULL;
    m_material[WHITE] = m_material[BLACK] = VALUE_ZERO;
    m_sideToMove = WHITE;
    m_epSquare = SQ_NONE;
    m_rule50 = 0;
    m_gamePly = 0;
    m_castlingRights = NO_CASTLING;
    m_posKey = 0ULL;
}

void Position::updateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        const Piece piece = m_board[sq];

        if (piece != NO_PIECE) {
            m_byColorBB[ColorOf(piece)] |= sq;
            m_byTypeBB[ALL_PIECES] |= m_byTypeBB[TypeOf(piece)] |= sq;

            m_pieceNb[piece]++;
            m_material[ColorOf(piece)] += PieceValue[piece];
        }
    }
}

void Position::ParseFen(const std::string& fen) {
    reset();

    unsigned char col, row, token;
    size_t idx;
    Square sq = SQ_A8;
    std::istringstream ss(fen);

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token)) {
            sq += (token - '0') * EAST;
        } else if (token == '/') {
            sq += 2 * SOUTH;
        } else {
            const char* p = std::strchr(PieceToChar, token);
            if (p) {
                idx = p - PieceToChar;
                m_board[sq] = Piece(idx);
                ++sq;
            }
        }
    }

    // 2. Active color
    ss >> token;
    m_sideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 3. Castling availability
    while ((ss >> token) && !isspace(token)) {
        switch (token) {
            case 'K': m_castlingRights |= WHITE_OO; break;
            case 'k': m_castlingRights |= BLACK_OO; break;
            case 'Q': m_castlingRights |= WHITE_OOO; break;
            case 'q': m_castlingRights |= BLACK_OOO; break;
        }
    }

    // 4. En passant square
    if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
        ((ss >> row) && (row == (m_sideToMove == WHITE ? '6' : '3')))) {
        m_epSquare = MakeSquare(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock (rule50)
    ss >> std::skipws >> m_rule50 >> m_gamePly;

    // Convert from fullmove starting from 1 to internal ply count
    m_gamePly = std::max(2 * (m_gamePly - 1), 0) + (m_sideToMove == BLACK);

    generatePosKey();
    updateListsBitboards();
}

bool Position::MakeMove(Move move) {
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    // save state
    m_history[m_gamePly].posKey = m_posKey;
    m_history[m_gamePly].rule50 = m_rule50;
    m_history[m_gamePly].epSquare = m_epSquare;
    m_history[m_gamePly].castlingRights = m_castlingRights;
    m_history[m_gamePly].captured =
        m_board[to]; // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (m_epSquare != SQ_NONE) {
        m_posKey ^= EnPassant[FileOf(m_epSquare)];
    }
    m_posKey ^= Castling[m_castlingRights];

    // special move handling
    if (move.TypeOf() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        removePiece(to + (m_sideToMove == WHITE ? SOUTH : NORTH));
        m_rule50 = 0; // reset 50-move on capture
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: movePiece(SQ_A1, SQ_D1); break;
            case SQ_C8: movePiece(SQ_A8, SQ_D8); break;
            case SQ_G1: movePiece(SQ_H1, SQ_F1); break;
            case SQ_G8: movePiece(SQ_H8, SQ_F8); break;
            default: assert(false);
        }
    }

    // normal capture handling (if a piece sits on 'to')
    if (m_board[to] != NO_PIECE) {
        removePiece(to);
        m_rule50 = 0;
    } else if (move.TypeOf() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        m_rule50++;
    }

    // move the piece
    movePiece(from, to);

    // promotion handling
    if (move.TypeOf() == PROMOTION) {
        removePiece(to);
        putPiece(MakePiece(m_sideToMove, move.PromotionType()), to);
    }

    // new en-passant target (from a double pawn push)
    m_epSquare = SQ_NONE;
    if (TypeOf(m_board[to]) == PAWN && std::abs(RankOf(from) - RankOf(to)) == 2) {
        m_epSquare = from + (m_sideToMove == WHITE ? NORTH : SOUTH);
    }
    if (m_epSquare != SQ_NONE) {
        m_posKey ^= EnPassant[FileOf(m_epSquare)]; // add new EP key
    }

    // update castling rights and re-add castling key
    m_castlingRights &= CastlePerm[from];
    m_castlingRights &= CastlePerm[to];
    m_posKey ^= Castling[m_castlingRights];

    // flip side
    m_sideToMove = ~m_sideToMove;
    m_posKey ^= Side;

    m_gamePly++;

    // if the we (opponent for current side) are in check, the move was illegal
    if (MoveGen::IsSquareAttacked(*this, SquareOf<KING>(~m_sideToMove), m_sideToMove)) {
        UnmakeMove(move);
        return false;
    }

    return true;
}

void Position::UnmakeMove(Move move) {
    m_gamePly--;

    const Square from = move.FromSq();
    const Square to = move.ToSq();

    if (move.TypeOf() == EN_PASSANT) {
        putPiece(MakePiece(m_sideToMove, PAWN), to + PawnPush(m_sideToMove));
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: movePiece(SQ_D1, SQ_A1); break;
            case SQ_C8: movePiece(SQ_D8, SQ_A8); break;
            case SQ_G1: movePiece(SQ_F1, SQ_H1); break;
            case SQ_G8: movePiece(SQ_F8, SQ_H8); break;
            default: assert(false);
        }
    }

    m_sideToMove = ~m_sideToMove;

    movePiece(to, from);

    if (move.TypeOf() == PROMOTION) {
        removePiece(from);
        putPiece(MakePiece(m_sideToMove, PAWN), from);
    }

    if (m_history[m_gamePly].captured != NO_PIECE) {
        putPiece(m_history[m_gamePly].captured, to);
    }

    m_epSquare = m_history[m_gamePly].epSquare;
    m_rule50 = m_history[m_gamePly].rule50;
    m_castlingRights = m_history[m_gamePly].castlingRights;
    m_posKey = m_history[m_gamePly].posKey;
}

void Position::Print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) { // <= not <
            Square sq = MakeSquare(file, rank);
            Piece p = m_board[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (m_sideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (IsOk(m_epSquare)) {
        cout << m_epSquare;
    } else {
        cout << "none";
    }
    cout << "\n";
    cout << "Castle permissions: " << (m_castlingRights & WHITE_OO ? "K" : "-")
         << (m_castlingRights & WHITE_OOO ? "Q" : "-") << (m_castlingRights & BLACK_OO ? "k" : "-")
         << (m_castlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << std::hex << m_posKey << std::dec << "\n";
}

void Position::perft(int depth) {
    if (depth == 0) {
        m_perftLealNodes++;
        return;
    }
    MoveList list;
    MoveGen::GeneratePseudoMoves(*this, list);

    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }
        perft(depth - 1);
        UnmakeMove(move);
    }
}

void Position::PerftTest(int depth) {
    using namespace std::chrono;

    std::cout << "Starting perft test to depth " << depth << "\n";

    m_perftLealNodes = 0;
    MoveList list;

    const auto start = high_resolution_clock::now();

    MoveGen::GeneratePseudoMoves(*this, list);
    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }

        uint64_t before = m_perftLealNodes;

        perft(depth - 1);
        UnmakeMove(move);

        auto PrintSq = [](Square sq) {
            std::cout << char('a' + FileOf(sq)) << char('1' + RankOf(sq));
        };
        PrintSq(move.FromSq());
        PrintSq(move.ToSq());

        if (move.TypeOf() == PROMOTION) {
            char promoCh;
            switch (move.PromotionType()) {
                case KNIGHT: promoCh = 'n'; break;
                case ROOK: promoCh = 'r'; break;
                case BISHOP: promoCh = 'b'; break;
                default: promoCh = 'q'; break;
            }
            std::cout << promoCh;
        }

        std::cout << ": " << m_perftLealNodes - before << "\n";
    }

    const auto stop = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << m_perftLealNodes << " nodes in " << duration << " ms\n\n";
}

}
