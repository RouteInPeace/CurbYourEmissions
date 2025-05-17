#pragma once

#include "alns/random_engine.hpp"
#include "cye/solution.hpp"

namespace cye {
Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen);
}
