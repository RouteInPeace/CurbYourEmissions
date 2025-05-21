#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/small.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto routes = std::vector<size_t>();
  routes.push_back(instance->depot_id());
  for (auto c : instance->customer_ids()) {
    routes.push_back(c);
  }
  routes.push_back(instance->depot_id());

  auto seed = 2150;

  std::mt19937 gen(seed);

  auto copy = routes;
  std::shuffle(copy.begin() + 1, copy.end() - 1, gen);
  // cye::patch_cargo_trivially(std::move(copy));
  // auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));

  // for (auto node_ind : solution.routes()) std::print("{} ", node_ind);
  // std::cout << '\n';

  // auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);
  // solution = optimal_energy_repair.repair(std::move(solution), 6u);

  // std::cout << solution.is_valid() << '\n';
  // for (auto node_ind : solution.routes()) std::print("{:3} ", node_ind);
  // std::cout << '\n';

  // auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);

  // for (int i = 0; i < 100000; i++) {
  //   std::mt19937 gen(i);

  //   auto copy = routes;
  //   std::shuffle(copy.begin() + 1, copy.end() - 1, gen);
  //   auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));

  //   solution = optimal_energy_repair.repair(std::move(solution), 11u);
  //   // solution = cye::repair_energy_violations_trivially(std::move(solution));

  //   if (!solution.is_valid()) {
  //     std::cout << i << '\n';
  //   }
  // }
}