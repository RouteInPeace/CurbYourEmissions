#include "cye/init_heuristics.hpp"
#include <cassert>
#include <limits>
#include <unordered_set>
#include <utility>
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"

auto cye::nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution {
  auto remaining_customer_ids = std::ranges::to<std::unordered_set<size_t>>(instance->customer_ids());
  std::vector<size_t> routes;
  routes.push_back(instance->depot_id());

  // Nearest neighbor
  while (!remaining_customer_ids.empty()) {
    auto best_customer_id = 0UZ;
    auto min_distance = std::numeric_limits<float>::infinity();

    for (const auto customer_id : remaining_customer_ids) {
      auto distance = instance->distance(routes.back(), customer_id);

      if (distance < min_distance) {
        min_distance = distance;
        best_customer_id = customer_id;
      }
    }

    routes.push_back(best_customer_id);
    remaining_customer_ids.erase(best_customer_id);
  }

  routes.push_back(instance->depot_id());

  auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);

  auto solution = repair_cargo_violations_optimally(Solution(instance, std::move(routes)),
                                                    static_cast<unsigned>(instance->cargo_capacity()) + 1u);
  return optimal_energy_repair.repair(std::move(solution), 1001u);
}