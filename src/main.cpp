#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <vector>

#include "alns/acceptance_criterion.hpp"
#include "alns/alns.hpp"
#include "alns/random_engine.hpp"
#include "cye/destroy.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "serial/json_archive.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto config = alns::Config<cye::Solution>(cye::nearest_neighbor(instance));
  config.acceptance_criterion = std::make_unique<alns::HillClimbingCriterion>();
  config.operator_selection = std::make_unique<alns::RandomOperatorSelection>();
  config.destroy_operators = {cye::random_destroy};
  config.repair_operators = {cye::greedy_repair};
  config.max_iterations = 100'000;
  config.verbose = true;

  std::cout << "Initial solution cost: " << config.initial_solution.get_cost() << std::endl;

  alns::RandomEngine gen{std::random_device{}()};
  auto best_solution = alns::optimize(config, gen);
  std::cout << "Best solution cost: " << best_solution.get_cost() << std::endl;

  return 0;
}
