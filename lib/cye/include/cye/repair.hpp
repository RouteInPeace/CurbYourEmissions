#pragma once

#include "alns/random_engine.hpp"
#include "cye/solution.hpp"

namespace cye {

auto repair_cargo_violations_optimally(Solution &&solution, unsigned bin_cnt) -> Solution;
auto repair_cargo_violations_trivially(Solution &&solution) -> Solution;

auto repair_energy_violations_trivially(Solution &&solution) -> Solution;

Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen);

}  // namespace cye