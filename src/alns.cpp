
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ranges>
#include <vector>

#include "alns/acceptance_criterion.hpp"
#include "alns/alns.hpp"
#include "cye/destroy.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "serial/json_archive.hpp"

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  alns::Config<cye::Solution> config(cye::nearest_neighbor(instance));
  config.acceptance_criterion = std::make_unique<alns::HillClimbingCriterion>();
  config.operator_selection = std::make_unique<alns::RandomOperatorSelection>();
  config.destroy_operators = {cye::random_destroy};
  config.repair_operators = {cye::regret_repair, cye::greedy_repair_best_first};
  config.max_iterations = 100000;
  config.verbose = true;

  auto solution = alns::optimize(config, gen);
  std::cout << "Best cost: " << solution.get_cost() << '\n';

  return 0;
}
