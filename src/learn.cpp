#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto routes = std::vector<size_t>();
  routes.push_back(instance->depot_id());
  for (auto c : instance->customer_ids()) {
    routes.push_back(c);
  }
  routes.push_back(instance->depot_id());

  auto seed = 4;

  std::mt19937 gen(seed);

  auto copy = routes;
  std::shuffle(copy.begin() + 1, copy.end() - 1, gen);
  auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));

  for (auto node_ind : solution.routes()) std::print("{} ", node_ind);
  std::cout << '\n';

  auto solution2 = cye::repair_energy_violations_optimally(std::move(solution), 11u);

  std::cout << solution2.is_valid() << '\n';
  for (auto node_ind : solution2.routes()) std::print("{:5} ", node_ind);
  std::cout << '\n';
}