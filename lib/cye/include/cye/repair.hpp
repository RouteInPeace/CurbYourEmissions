#pragma once

#include "alns/random_engine.hpp"
#include "cye/solution.hpp"

namespace cye {
Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen);
Solution greedy_repair_best_first(Solution &&solution, alns::RandomEngine &gen);
Solution regret_repair(Solution &&solution, alns::RandomEngine &gen);
}  // namespace cye