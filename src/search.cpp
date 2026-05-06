#include "pch.h"
#include "search.h"
#include "position.h"
#include "evaluation.h"
#include "movegen.h"

namespace Zugzwang {

std::string MoveToStr(Move move) {
    Square from = move.FromSq();
    Square to = move.ToSq();
    std::stringstream ss;
    ss << char('a' + FileOf(from)) << char('1' + RankOf(from)) << char('a' + FileOf(to))
       << char('1' + RankOf(to));
    if (move.TypeOf() == PROMOTION) {
        char c;
        switch (move.PromotionType()) {
            case KNIGHT: c = 'n'; break;
            case BISHOP: c = 'b'; break;
            case ROOK: c = 'r'; break;
            case QUEEN: c = 'q'; break;
        }
        ss << c;
    }
    return ss.str();
}

void Searcher::Search(Position& pos, const Info& info) {
    assert(info.depth < MAX_DEPTH);

    int multiplier = (pos.SideToMove() == WHITE) ? 1 : -1;
    for (Depth depth = 1; depth <= info.depth; depth++) {
        std::cout << "info depth " << depth << " score cp "
                  << AlphaBeta(pos, -VALUE_INFINITE, VALUE_INFINITE, depth, 0) * multiplier
                  << " pv ";
        for (int i = 0; i < m_pvLength[0]; i++) {
            std::cout << MoveToStr(m_pv[0][i]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "bestmove " << MoveToStr(m_pv[0][0]) << "\n";
}

Value Searcher::AlphaBeta(Position& pos, Value alpha, Value beta, Depth depth, int ply) {
    m_pvLength[ply] = 0;

    if (depth == 0) {
        return Quiescence(pos, alpha, beta);
    }

    Value best = -VALUE_INFINITE;

    MoveList list;
    MoveGen::GeneratePseudoMoves(pos, list);
    int legalMoves = 0;

    for (Move move : list) {
        if (!pos.MakeMove(move)) {
            continue;
        }
        legalMoves++;

        Value score = -AlphaBeta(pos, -beta, -alpha, depth - 1, ply + 1);
        pos.UnmakeMove(move);

        if (score > best) {
            best = score;

            if (score > alpha) {
                alpha = score;
                // ONLY update PV if it's a new best move within bounds
                m_pv[ply][0] = move;
                for (int i = 0; i < m_pvLength[ply + 1]; i++) {
                    m_pv[ply][i + 1] = m_pv[ply + 1][i];
                }
                m_pvLength[ply] = m_pvLength[ply + 1] + 1;
            }
        }

        if (score >= beta) {
            return best;
        }
    }

    if (legalMoves == 0) {
        if (pos.IsInCheck()) {
            return -VALUE_MATE + ply; // Favor shorter mates
        } else {
            return VALUE_DRAW;
        }
    }

    return best;
}

Value Searcher::Quiescence(Position& pos, Value alpha, Value beta) {
    return Evaluation::Evaluate(pos);
}

}
