#include "pch.h"
#include "movegen.h"
#include "uci.h"

namespace Zugzwang {

namespace {

constexpr const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

bool isMoveStr(std::string_view str) {
    auto IsFileValid = [](char ch) { return ch >= 'a' && ch <= 'h'; };
    auto IsRankValid = [](char ch) { return ch >= '1' && ch <= '8'; };
    auto IsPromoValid = [](char ch) { return ch == 'q' || ch == 'r' || ch == 'b' || ch == 'n'; };

    if (str.size() != 4 && str.size() != 5) {
        return false;
    }
    if (!IsFileValid(str[0]) || !IsRankValid(str[1]) || !IsFileValid(str[2]) ||
        !IsRankValid(str[3])) {
        return false;
    }
    if (str[0] == str[2] && str[1] == str[3]) { // same from and to square
        return false;
    }
    if (str.size() == 5 && !IsPromoValid(str[4])) {
        return false;
    }
    return true;
}

} // namespace

UCIEngine::UCIEngine(int argc, char** argv) : board() { board.ParseFen(StartFEN); }

void UCIEngine::Loop() {
    std::string token, cmd;

    while (true) {
        if (!getline(std::cin, cmd)) {
            cmd = "quit";
        }

        std::istringstream is(cmd);

        token.clear();
        is >> std::skipws >> token;

        if (token == "uci") {
            std::cout << "id name Zugzwang 1.0\nid author Paul\n";
            std::cout << "uciok\n";
        } else if (token == "isready") {
            std::cout << "readyok\n";
        } else if (token == "position") {
            position(is);
            board.Print();
        } else if (token == "go") {
            go(is);
        } else if (token == "quit") {
            break;
        } else if (!token.empty() && token[0] != '#') {
            std::cout << "Unknown command: '" << cmd << "'.\n";
        }
    }
}

void UCIEngine::go(std::istringstream& is) {
    std::string token;
    is >> token;
    if (token != "perft") {
        return;
    }

    int depth;
    is >> depth;
    if (depth >= 1 && depth <= 6) {
        board.PerftTest(depth);
    }
}

void UCIEngine::position(std::istringstream& is) {
    std::string token, fen;

    is >> token;
    if (token == "startpos") {
        fen = StartFEN;
        is >> token; // Consume the "moves" token, if any
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            fen += token + " ";
        }
    } else {
        return;
    }

    std::vector<std::string> moves;

    while (is >> token) {
        moves.push_back(token);
    }

    board.ParseFen(fen);
    for (const auto& move : moves) {
        if (!isMoveStr(move)) {
            break;
        }

        Move mv = parseMove(move);
        if (mv == Move::None()) {
            break;
        }

        board.MakeMove(mv);
    }
}

Move UCIEngine::parseMove(std::string_view str) const {
    Square from = MakeSquare(File(str[0] - 'a'), Rank(str[1] - '1'));
    Square to = MakeSquare(File(str[2] - 'a'), Rank(str[3] - '1'));

    MoveList list;
    MoveGen::GeneratePseudo(board, list);

    for (const auto move : list) {
        if (move.FromSq() == from && move.ToSq() == to) {
            if (move.TypeOf() == PROMOTION) {
                PieceType type = move.PromotionType();
                if ((type == KNIGHT && str[4] == 'n') || (type == ROOK && str[4] == 'r') ||
                    (type == BISHOP && str[4] == 'b') || (type == QUEEN && str[4] == 'q')) {
                    return move;
                }
                continue;
            }
            return move;
        }
    }
    return Move::None();
}

} // namespace Zugzwang