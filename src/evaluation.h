#pragma once

#include "types.h"
#include <cstdint>

namespace Zugzwang {

class Position;

class Evaluation {
  public:
    static Value Evaluate(const Position& pos);

  private:
    Evaluation();
};

}
