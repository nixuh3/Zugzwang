#pragma once

#include "position.h"
#include <iosfwd>
#include <string_view>

namespace Zugzwang {

class UCIEngine {
  public:
    UCIEngine(int argc, char** argv);
    void Loop();

  private:
    void go(std::istringstream& is);
    void position(std::istringstream& is);
    Move parseMove(std::string_view str) const;

    Position m_Board;
};

}