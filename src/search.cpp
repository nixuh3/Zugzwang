#include "pch.h"
#include "search.h"
#include "position.h"
#include "evaluation.h"
#include "movegen.h"

namespace Zugzwang {

namespace {

Value Negamax(Position& pos, Depth depth) {
    if (depth == 0) {
        return Evaluation::Evaluate(pos);
    }

    Value max = -VALUE_INFINITE;

    MoveList list;
    MoveGen::GeneratePseudoMoves(pos, list);

    for (Move move : list) {
        if (!pos.MakeMove(move)) {
            continue;
        }

        Value score = -Negamax(pos, depth - 1);

        pos.UnmakeMove(move);

        if (score > max) {
            max = score;
        }
    }

    return max;
}
}

void Search::Search(Position& pos, const Info& info) {
    std::cout << Evaluation::Evaluate(pos) << "\n";

    int multiplier = (pos.SideToMove() == WHITE) ? 1 : -1;
    for (Depth depth = 1; depth <= info.depth; depth++) {
        std::cout << "info depth " << depth << " score cp " << Negamax(pos, depth) * multiplier
                  << "\n";
    }
}

}