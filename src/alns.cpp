
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ranges>
#include <vector>

#include "meta/alns/acceptance_criterion.hpp"
#include "meta/alns/alns.hpp"
#include "cye/destroy.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/alns/operator_selection.hpp"
#include "serial/json_archive.hpp"

auto main() -> int {

  auto start_time = std::chrono::high_resolution_clock::now();

  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto num_iter = 150000U;

  meta::alns::Config<cye::Solution> config(cye::nearest_neighbor(instance));
  config.acceptance_criterion = std::make_unique<meta::alns::RecordToRecordTravel>(0.15, 0.025, num_iter);
  config.operator_selection = std::make_unique<meta::alns::RouletteWheelOperatorSelection>();
  config.destroy_operators = {
    [](cye::Solution &&solution, meta::RandomEngine &gen) {
      return cye::random_destroy(std::move(solution), gen, 0.5);
    }
  ,[](cye::Solution &&solution, meta::RandomEngine &gen) {
   return cye::random_range_destroy(std::move(solution), gen, 0.5);
  }
  ,[](cye::Solution &&solution, meta::RandomEngine &gen) {
   return cye::worst_destroy(std::move(solution), gen, 0.5);
  }
  , [](cye::Solution &&solution, meta::RandomEngine &gen) {
   return cye::shaw_removal(std::move(solution), gen, 0.5);
  }
  };
  config.repair_operators = {
      [](cye::Solution &&solution, meta::RandomEngine &gen) { return cye::regret_repair(std::move(solution), gen, 3); },
      cye::greedy_repair_best_first,
      cye::random_repair
  };
  config.max_iterations = num_iter;
  config.verbose = true;

  auto solution = meta::alns::optimize(config, gen, cye::reorder_solution_optimally);
  std::cout << "Best cost: " << solution.cost() << '\n';

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
  std::cout << "Time taken: " << duration.count() << " seconds\n";

  return 0;
}
