#pragma once

#include "cye/solution.hpp"
#include "alns/random_engine.hpp"

namespace cye {
    Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen);
}