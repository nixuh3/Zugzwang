#pragma once

#include "types.h"

namespace Zugzwang {

class Position;

namespace Search {

struct Info {
    Info() { depth = perft = 0; }

    Depth depth;
    Depth perft;
};

void Search(Position& pos, const Info& info);

}

}
