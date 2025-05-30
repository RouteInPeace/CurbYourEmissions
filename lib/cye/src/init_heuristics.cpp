#include "cye/init_heuristics.hpp"
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"

auto cye::random_customer_permutation(meta::RandomEngine &gen, std::shared_ptr<Instance> instance) -> Solution {
  auto customers = instance->customer_ids() | std::ranges::to<std::vector<size_t>>();
  std::ranges::shuffle(customers, gen);

  auto solution = Solution(instance, std::move(customers));
  patch_cargo_trivially(solution);
  patch_energy_trivially(solution);

  return solution;
}

auto cye::nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution {
  auto remaining_customer_ids = std::ranges::to<std::unordered_set<size_t>>(instance->customer_ids());
  std::vector<size_t> routes;

  // Nearest neighbor
  while (!remaining_customer_ids.empty()) {
    auto best_customer_id = 0UZ;
    auto min_distance = std::numeric_limits<float>::infinity();

    for (const auto customer_id : remaining_customer_ids) {
      auto previous_node_id = routes.empty() ? instance->depot_id() : routes.back();
      auto distance = instance->distance(previous_node_id, customer_id);

      if (distance < min_distance) {
        min_distance = distance;
        best_customer_id = customer_id;
      }
    }

    routes.push_back(best_customer_id);
    remaining_customer_ids.erase(best_customer_id);
  }

  auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);

  auto solution = Solution(instance, std::move(routes));
  patch_cargo_optimally(solution);
  optimal_energy_repair.patch(solution, 1001u);

  return solution;
}

auto cye::stochastic_nearest_neighbor(meta::RandomEngine &gen, std::shared_ptr<Instance> instance, size_t k)
    -> Solution {
  auto remaining_customer_ids = std::ranges::to<std::unordered_set<size_t>>(instance->customer_ids());
  auto routes = std::vector<size_t>();
  auto candidates = std::set<std::pair<float, size_t>>();

  auto dist = std::uniform_int_distribution(0UZ, k - 1);

  while (!remaining_customer_ids.empty()) {
    for (const auto customer_id : remaining_customer_ids) {
      auto previous_node_id = routes.empty() ? instance->depot_id() : routes.back();
      auto distance = instance->distance(previous_node_id, customer_id);

      candidates.emplace(distance, customer_id);
      if (candidates.size() > k) {
        candidates.erase(--candidates.end());
      }
    }

    auto ind = std::min(dist(gen), candidates.size() - 1);
    auto it = candidates.begin();
    while (ind > 0) {
      ++it;
      --ind;
    }

    routes.push_back(it->second);
    remaining_customer_ids.erase(it->second);
    candidates.clear();
  }

  auto solution = Solution(instance, std::move(routes));
  patch_cargo_trivially(solution);
  patch_energy_trivially(solution);

  return solution;
}