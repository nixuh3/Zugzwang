#pragma once

#include "types.h"
#include <vector>

namespace Zugzwang {

class Position;

class Searcher {
  public:
    struct Info {
        Info() { depth = perft = 0; }

        Depth depth;
        Depth perft;
        int startTime;
        int stopTime;
        int timeSet;
        int depthSet;
        int movestogo;

        bool infinite;
        bool quit;
        bool stopped;

        uint64_t nodes;
    };

    void Search(Position& pos, const Info& info);

  private:
    Value AlphaBeta(Position& pos, Value alpha, Value beta, Depth depth, int ply);
    Value Quiescence(Position& pos, Value alpha, Value beta);

    Move m_pv[MAX_PLY][MAX_PLY];
    int m_pvLength[MAX_PLY];

    int m_history[PIECE_NB][SQUARE_NB];
    int m_killers[2][MAX_PLY];
};

}
