
#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "cye/init_heuristics.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/simulated_annealing.hpp"
#include "serial/json_archive.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/E-n101-k8.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto config = meta::Config<cye::Solution>(cye::nearest_neighbor(instance));
  config.get_temperature = meta::create_geometric_schedule(100.0, 1e-8, 0.9999);

  // auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);

  config.get_neighbour = [](RandomEngine &gen, cye::Solution const &solution) {
    auto dist = std::uniform_int_distribution(0UZ, solution.instance().customer_cnt() - 1);
    auto routes = solution.get_customers();
    std::swap(routes[dist(gen)], routes[dist(gen)]);
    routes.insert(routes.begin(), solution.instance().depot_id());
    routes.push_back(solution.instance().depot_id());
    auto new_solution = cye::Solution(solution.instance_ptr(), std::move(routes));
    // new_solution = cye::repair_cargo_violations_optimally(
    //     std::move(new_solution), static_cast<unsigned>(solution.instance().cargo_capacity()) + 1u);
    // new_solution = optimal_energy_repair.repair(std::move(new_solution), 1001u);

    new_solution = cye::repair_cargo_violations_trivially(std::move(new_solution));
    new_solution = cye::repair_energy_violations_trivially(std::move(new_solution));
    return new_solution;
  };
  config.verbose = true;

  std::mt19937 gen(0);
  auto solution = meta::optimize(gen, config);
  return 0;
}
