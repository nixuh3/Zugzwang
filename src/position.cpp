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

uint64_t seed = 0x123456789ABCDEFULL;

uint64_t splitmix64() {
    uint64_t z = (seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

Key psq[PIECE_NB][SQUARE_NB];
Key castling[CASTLING_RIGHT_NB];
Key enPassant[FILE_NB];
Key side;

} // namespace

Position::Position() {
    for (int i = 0; i < PIECE_NB; ++i) {
        for (Square j = SQ_A1; j < SQUARE_NB; ++j) {
            psq[i][j] = splitmix64();
        }
    }

    for (File f = FILE_A; f < FILE_NB; ++f) {
        enPassant[f] = splitmix64();
    }

    side = splitmix64();

    for (int i = 0; i < CASTLING_RIGHT_NB; ++i) {
        castling[i] = splitmix64();
    }
}

void Position::putPiece(Piece piece, Square sq) {
    assert(piece != NO_PIECE);

    m_Board[sq] = piece;
    m_PosKey ^= psq[piece][sq];
    m_PieceNb[piece]++;
    m_Material[ColorOf(piece)] += PieceValue[piece];

    m_ByColorBB[ColorOf(piece)] |= sq;
    m_ByTypeBB[ALL_PIECES] |= m_ByTypeBB[TypeOf(piece)] |= sq;
}

void Position::removePiece(Square sq) {
    Piece piece = m_Board[sq];
    assert(piece != NO_PIECE);

    m_PosKey ^= psq[piece][sq];
    m_Board[sq] = NO_PIECE;
    m_Material[ColorOf(piece)] -= PieceValue[piece];

    m_ByTypeBB[ALL_PIECES] ^= sq;
    m_ByTypeBB[TypeOf(piece)] ^= sq;
    m_ByColorBB[ColorOf(piece)] ^= sq;

    m_PieceNb[piece]--;
}

void Position::movePiece(Square from, Square to) {
    Piece piece = m_Board[from];
    assert(piece != NO_PIECE);

    Bitboard fromTo = from | to;

    m_PosKey ^= psq[piece][from];
    m_PosKey ^= psq[piece][to];
    m_Board[from] = NO_PIECE;
    m_Board[to] = piece;

    m_ByTypeBB[ALL_PIECES] ^= fromTo;
    m_ByTypeBB[TypeOf(piece)] ^= fromTo;
    m_ByColorBB[ColorOf(piece)] ^= fromTo;
}

void Position::generatePosKey() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        Piece piece = m_Board[i];
        if (piece != NO_PIECE) {
            m_PosKey ^= psq[piece][i];
        }
    }

    if (m_SideToMove == WHITE) {
        m_PosKey ^= side;
    }

    if (m_EpSquare != SQ_NONE) {
        m_PosKey ^= enPassant[FileOf(m_EpSquare)];
    }

    m_PosKey ^= castling[m_CastlingRights];
}

void Position::reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        m_Board[i] = NO_PIECE;
    }

    for (int i = 0; i < PIECE_NB; ++i) {
        m_PieceNb[i] = 0;
    }

    for (int i = 0; i < PIECE_TYPE_NB; ++i) {
        m_ByTypeBB[i] = 0ULL;
    }

    m_ByColorBB[WHITE] = m_ByColorBB[BLACK] = 0ULL;
    m_Material[WHITE] = m_Material[BLACK] = VALUE_ZERO;
    m_SideToMove = WHITE;
    m_EpSquare = SQ_NONE;
    m_Rule50 = 0;
    m_GamePly = 0;
    m_CastlingRights = NO_CASTLING;
    m_PosKey = 0ULL;
}

void Position::updateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        const Piece piece = m_Board[sq];

        if (piece != NO_PIECE) {
            m_ByColorBB[ColorOf(piece)] |= sq;
            m_ByTypeBB[ALL_PIECES] |= m_ByTypeBB[TypeOf(piece)] |= sq;

            m_PieceNb[piece]++;
            m_Material[ColorOf(piece)] += PieceValue[piece];
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
                m_Board[sq] = Piece(idx);
                ++sq;
            }
        }
    }

    // 2. Active color
    ss >> token;
    m_SideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 3. Castling availability
    while ((ss >> token) && !isspace(token)) {
        switch (token) {
            case 'K': m_CastlingRights |= WHITE_OO; break;
            case 'k': m_CastlingRights |= BLACK_OO; break;
            case 'Q': m_CastlingRights |= WHITE_OOO; break;
            case 'q': m_CastlingRights |= BLACK_OOO; break;
        }
    }

    // 4. En passant square
    if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
        ((ss >> row) && (row == (m_SideToMove == WHITE ? '6' : '3')))) {
        m_EpSquare = MakeSquare(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock (rule50)
    ss >> std::skipws >> m_Rule50 >> m_GamePly;

    // Convert from fullmove starting from 1 to internal ply count
    m_GamePly = std::max(2 * (m_GamePly - 1), 0) + (m_SideToMove == BLACK);

    generatePosKey();
    updateListsBitboards();
}

bool Position::MakeMove(const Move& move) {
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    // save state
    m_History[m_GamePly].PosKey = m_PosKey;
    m_History[m_GamePly].Rule50 = m_Rule50;
    m_History[m_GamePly].EpSquare = m_EpSquare;
    m_History[m_GamePly].CastlingRights = m_CastlingRights;
    m_History[m_GamePly].Captured =
        m_Board[to]; // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (m_EpSquare != SQ_NONE) {
        m_PosKey ^= enPassant[FileOf(m_EpSquare)];
    }
    m_PosKey ^= castling[m_CastlingRights];

    // special move handling
    if (move.TypeOf() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        removePiece(to + (m_SideToMove == WHITE ? SOUTH : NORTH));
        m_Rule50 = 0; // reset 50-move on capture
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
    if (m_Board[to] != NO_PIECE) {
        removePiece(to);
        m_Rule50 = 0;
    } else if (move.TypeOf() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        m_Rule50++;
    }

    // move the piece
    movePiece(from, to);

    // promotion handling
    if (move.TypeOf() == PROMOTION) {
        removePiece(to);
        putPiece(MakePiece(m_SideToMove, move.PromotionType()), to);
    }

    // new en-passant target (from a double pawn push)
    m_EpSquare = SQ_NONE;
    if (TypeOf(m_Board[to]) == PAWN && std::abs(RankOf(from) - RankOf(to)) == 2) {
        m_EpSquare = from + (m_SideToMove == WHITE ? NORTH : SOUTH);
    }
    if (m_EpSquare != SQ_NONE) {
        m_PosKey ^= enPassant[FileOf(m_EpSquare)]; // add new EP key
    }

    // update castling rights and re-add castling key
    m_CastlingRights &= CastlePerm[from];
    m_CastlingRights &= CastlePerm[to];
    m_PosKey ^= castling[m_CastlingRights];

    // flip side
    m_SideToMove = ~m_SideToMove;
    m_PosKey ^= side;

    m_GamePly++;

    if (MoveGen::IsSquareAttacked(*this, SquareOf<KING>(~m_SideToMove), m_SideToMove)) {
        UnmakeMove(move);
        return false;
    }
    return true;
}

void Position::UnmakeMove(const Move& move) {
    m_GamePly--;

    const Square from = move.FromSq();
    const Square to = move.ToSq();

    if (move.TypeOf() == EN_PASSANT) {
        putPiece(MakePiece(m_SideToMove, PAWN), to + PawnPush(m_SideToMove));
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: movePiece(SQ_D1, SQ_A1); break;
            case SQ_C8: movePiece(SQ_D8, SQ_A8); break;
            case SQ_G1: movePiece(SQ_F1, SQ_H1); break;
            case SQ_G8: movePiece(SQ_F8, SQ_H8); break;
            default: assert(false);
        }
    }

    m_SideToMove = ~m_SideToMove;

    movePiece(to, from);

    if (move.TypeOf() == PROMOTION) {
        removePiece(from);
        putPiece(MakePiece(m_SideToMove, PAWN), from);
    }

    if (m_History[m_GamePly].Captured != NO_PIECE) {
        putPiece(m_History[m_GamePly].Captured, to);
    }

    m_EpSquare = m_History[m_GamePly].EpSquare;
    m_Rule50 = m_History[m_GamePly].Rule50;
    m_CastlingRights = m_History[m_GamePly].CastlingRights;
    m_PosKey = m_History[m_GamePly].PosKey;
}

void Position::Print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) { // <= not <
            Square sq = MakeSquare(file, rank);
            Piece p = m_Board[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (m_SideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (IsOk(m_EpSquare)) {
        cout << m_EpSquare;
    } else {
        cout << "none";
    }
    cout << "\n";
    cout << "Castle permissions: " << (m_CastlingRights & WHITE_OO ? "K" : "-")
         << (m_CastlingRights & WHITE_OOO ? "Q" : "-") << (m_CastlingRights & BLACK_OO ? "k" : "-")
         << (m_CastlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << std::hex << m_PosKey << std::dec << "\n";
    cout << m_Material[WHITE] << ", " << m_Material[BLACK] << "\n";
}

void Position::perft(int depth) {
    if (depth == 0) {
        m_PerftLealNodes++;
        return;
    }
    MoveList list;
    MoveGen::GeneratePseudo(*this, list);

    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }
        perft(depth - 1);
        UnmakeMove(move);
    }
}

uint64_t Position::PerftTest(int depth) {
    using namespace std::chrono;

    std::cout << "Starting perft test to depth " << depth << "\n";

    m_PerftLealNodes = 0;
    MoveList list;

    const auto start = high_resolution_clock::now();

    MoveGen::GeneratePseudo(*this, list);
    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }

        uint64_t before = m_PerftLealNodes;

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

        std::cout << ": " << m_PerftLealNodes - before << "\n";
    }

    const auto stop = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << m_PerftLealNodes << " nodes in " << duration << " ms\n\n";

    return duration;
}

}
