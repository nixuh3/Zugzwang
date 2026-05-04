#pragma once

#include "types.h"

namespace Zugzwang {

class Position;

namespace MoveGen {

bool IsSquareAttacked(const Position& pos, Square sq, Color attacker);
void GeneratePseudoMoves(const Position& pos, MoveList& list);
void GenerateLegalMoves(const Position& pos, MoveList& list);

}

}
