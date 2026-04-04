#include "pch.h"
#include "bitboard.h"
#include "movegen.h"
#include "position.h"

namespace Zugzwang {
namespace {

constexpr auto PieceToChar = " PNBRQK  pnbrqk";

// clang-format off
constexpr int CastlePerm[64] = {
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

uint64_t seed = 1804289383ULL;
uint64_t rand64() {
    uint64_t x = seed;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    seed = x;
    return x * 2685821657736338717ULL;
}

Key psq[PIECE_NB][SQUARE_NB]; // psq[NO_PIECE] for en passant
Key castling[CASTLING_RIGHT_NB];
Key side;

} // namespace

Position::Position() {
    for (int i = 0; i < PIECE_NB; ++i) {
        for (Square j = SQ_A1; j < SQUARE_NB; ++j) {
            psq[i][j] = rand64();
        }
    }
    side = rand64();
    for (int i = 0; i < CASTLING_RIGHT_NB; ++i) {
        castling[i] = rand64();
    }
}

void Position::putPiece(Piece piece, Square sq) {
    assert(piece != NO_PIECE);

    board[sq] = piece;
    posKey ^= psq[piece][sq];
    pieceNb[piece]++;

    byColorBB[ColorOf(piece)] |= sq;
    byTypeBB[ALL_PIECES] |= byTypeBB[TypeOf(piece)] |= sq;
}

void Position::removePiece(Square sq) {
    Piece piece = board[sq];
    assert(piece != NO_PIECE);

    posKey ^= psq[piece][sq];
    board[sq] = NO_PIECE;

    byTypeBB[ALL_PIECES] ^= sq;
    byTypeBB[TypeOf(piece)] ^= sq;
    byColorBB[ColorOf(piece)] ^= sq;

    pieceNb[piece]--;
}

void Position::movePiece(Square from, Square to) {
    Piece piece = board[from];
    assert(piece != NO_PIECE);

    Bitboard fromTo = from | to;

    posKey ^= psq[piece][from];
    posKey ^= psq[piece][to];
    board[from] = NO_PIECE;
    board[to] = piece;

    byTypeBB[ALL_PIECES] ^= fromTo;
    byTypeBB[TypeOf(piece)] ^= fromTo;
    byColorBB[ColorOf(piece)] ^= fromTo;
}

void Position::generatePosKey() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        Piece piece = board[i];
        if (piece != NO_PIECE) {
            posKey ^= psq[piece][i];
        }
    }

    if (sideToMove == WHITE) {
        posKey ^= side;
    }

    if (epSquare != SQ_NONE) {
        posKey ^= psq[NO_PIECE][epSquare];
    }

    posKey ^= castling[castlingRights];
}

void Position::reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        board[i] = NO_PIECE;
    }

    for (int i = 0; i < PIECE_NB; ++i) {
        pieceNb[i] = 0;
    }

    for (int i = 0; i < PIECE_TYPE_NB; ++i) {
        byTypeBB[i] = 0ULL;
    }

    byColorBB[WHITE] = byColorBB[BLACK] = 0ULL;
    sideToMove = WHITE;
    epSquare = SQ_NONE;
    rule50 = 0;
    gamePly = 0;
    castlingRights = NO_CASTLING;
    posKey = 0ULL;
}

void Position::updateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        const Piece piece = board[sq];

        if (piece != NO_PIECE) {
            byColorBB[ColorOf(piece)] |= sq;
            byTypeBB[ALL_PIECES] |= byTypeBB[TypeOf(piece)] |= sq;

            pieceNb[piece]++;
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
                board[sq] = Piece(idx);
                ++sq;
            }
        }
    }

    // 2. Active color
    ss >> token;
    sideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 3. Castling availability
    while ((ss >> token) && !isspace(token)) {
        switch (token) {
            case 'K': castlingRights |= WHITE_OO; break;
            case 'k': castlingRights |= BLACK_OO; break;
            case 'Q': castlingRights |= WHITE_OOO; break;
            case 'q': castlingRights |= BLACK_OOO; break;
        }
    }

    // 4. En passant square
    if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
        ((ss >> row) && (row == (sideToMove == WHITE ? '6' : '3')))) {
        epSquare = MakeSquare(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock (rule50)
    ss >> std::skipws >> rule50 >> gamePly;

    // Convert from fullmove starting from 1 to internal ply count
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

    generatePosKey();
    updateListsBitboards();
}

bool Position::MakeMove(const Move& move) {
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    // save state
    history[gamePly].posKey = posKey;
    history[gamePly].rule50 = rule50;
    history[gamePly].epSquare = epSquare;
    history[gamePly].castlingRights = castlingRights;
    history[gamePly].captured = board[to]; // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (epSquare != SQ_NONE) {
        posKey ^= psq[NO_PIECE][epSquare];
    }
    posKey ^= castling[castlingRights];

    // special move handling
    if (move.TypeOf() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        removePiece(to + (sideToMove == WHITE ? SOUTH : NORTH));
        rule50 = 0; // reset 50-move on capture
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
    if (board[to] != NO_PIECE) {
        removePiece(to);
        rule50 = 0;
    } else if (move.TypeOf() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        rule50++;
    }

    // move the piece
    movePiece(from, to);

    // promotion handling
    if (move.TypeOf() == PROMOTION) {
        removePiece(to);
        putPiece(MakePiece(sideToMove, move.PromotionType()), to);
    }

    // new en-passant target (from a double pawn push)
    epSquare = SQ_NONE;
    if (TypeOf(board[to]) == PAWN && std::abs(RankOf(from) - RankOf(to)) == 2) {
        epSquare = from + (sideToMove == WHITE ? NORTH : SOUTH);
    }
    if (epSquare != SQ_NONE) {
        posKey ^= psq[NO_PIECE][epSquare]; // add new EP key
    }

    // update castling rights and re-add castling key
    castlingRights &= CastlePerm[from];
    castlingRights &= CastlePerm[to];
    posKey ^= castling[castlingRights];

    // flip side
    sideToMove = ~sideToMove;
    posKey ^= side;

    gamePly++;

    if (MoveGen::IsSquareAttacked(*this, SquareOf<KING>(~sideToMove), sideToMove)) {
        UnmakeMove(move);
        return false;
    }
    return true;
}

void Position::UnmakeMove(const Move& move) {
    gamePly--;

    const Square from = move.FromSq();
    const Square to = move.ToSq();

    if (move.TypeOf() == EN_PASSANT) {
        putPiece(MakePiece(sideToMove, PAWN), to + PawnPush(sideToMove));
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: movePiece(SQ_D1, SQ_A1); break;
            case SQ_C8: movePiece(SQ_D8, SQ_A8); break;
            case SQ_G1: movePiece(SQ_F1, SQ_H1); break;
            case SQ_G8: movePiece(SQ_F8, SQ_H8); break;
            default: assert(false);
        }
    }

    sideToMove = ~sideToMove;

    movePiece(to, from);

    if (move.TypeOf() == PROMOTION) {
        removePiece(from);
        putPiece(MakePiece(sideToMove, PAWN), from);
    }

    if (history[gamePly].captured != NO_PIECE) {
        putPiece(history[gamePly].captured, to);
    }

    epSquare = history[gamePly].epSquare;
    rule50 = history[gamePly].rule50;
    castlingRights = history[gamePly].castlingRights;
    posKey = history[gamePly].posKey;
}

void Position::Print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) { // <= not <
            Square sq = MakeSquare(file, rank);
            Piece p = board[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (sideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (IsOk(epSquare)) {
        cout << epSquare;
    } else {
        cout << "none";
    }
    cout << "\n";
    cout << "Castle permissions: " << (castlingRights & WHITE_OO ? "K" : "-")
         << (castlingRights & WHITE_OOO ? "Q" : "-") << (castlingRights & BLACK_OO ? "k" : "-")
         << (castlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << std::hex << posKey << std::dec << "\n";
}

void Position::perft(int depth) {
    if (depth == 0) {
        perftLealNodes++;
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

    perftLealNodes = 0;
    MoveList list;

    const auto start = high_resolution_clock::now();

    MoveGen::GeneratePseudo(*this, list);
    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }

        uint64_t before = perftLealNodes;

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

        std::cout << ": " << perftLealNodes - before << "\n";
    }

    const auto stop = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << perftLealNodes << " nodes in " << duration << " ms\n\n";

    return duration;
}

} // namespace Zugzwang
