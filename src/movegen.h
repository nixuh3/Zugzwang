#pragma once

#include "types.h"

namespace Zugzwang {

class Position;

namespace MoveGen {

bool IsSquareAttacked(const Position& pos, Square sq, Color attacker);
void GeneratePseudo(const Position& pos, MoveList& list);

} // namespace MoveGen

} // namespace Zugzwang
