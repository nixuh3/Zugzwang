#pragma once

#include "types.h"
#include <cstdint>

namespace Zugzwang {

class Position;

namespace Evaluation {

Value Evaluate(const Position& pos);

};

}
