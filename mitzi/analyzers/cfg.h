#pragma once

#include <vector>

namespace mitzi {

struct basic_block {
  std::vector<int> instructions;  // Indices into an instructions array
  std::vector<int> successors;    // Indices into the cfg
};

using cfg = std::vector<basic_block>;

struct lifetime_constraint {
  int superset;
  int subset;
};

}  // namespace mitzi