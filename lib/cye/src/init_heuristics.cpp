#include "cye/init_heuristics.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>
#include "cye/instance.hpp"
#include "cye/solution.hpp"

auto cye::random_customer_permutation(meta::RandomEngine &gen, std::shared_ptr<Instance> instance) -> Solution {
  auto customers = instance->customer_ids() | std::ranges::to<std::vector<size_t>>();
  std::ranges::shuffle(customers, gen);

  auto solution = Solution(instance, std::move(customers));
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

  auto solution = Solution(instance, std::move(routes));
  return solution;
}

auto cye::stochastic_rank_nearest_neighbor(meta::RandomEngine &gen, std::shared_ptr<Instance> instance, size_t k)
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

  return solution;
}

auto cye::stochastic_nearest_neighbor(meta::RandomEngine &gen, std::shared_ptr<Instance> instance) -> Solution {
  auto remaining_customer_ids = std::ranges::to<std::unordered_set<size_t>>(instance->customer_ids());
  auto routes = std::vector<size_t>();

  auto distribution = std::vector<std::pair<double, size_t>>();

  while (!remaining_customer_ids.empty()) {
    distribution.resize(remaining_customer_ids.size());
    auto i = 0UZ;
    auto previous_node_id = routes.empty() ? instance->depot_id() : routes.back();
    for (const auto customer_id : remaining_customer_ids) {
      auto distance = instance->distance(previous_node_id, customer_id);

      auto s = i == 0 ? 0.0 : distribution[i - 1].first;
      distribution[i] = {s + 1.0 / distance, customer_id};
      i++;
    }

    auto dist = std::uniform_real_distribution(0.0, distribution.back().first);
    auto p = dist(gen);

    auto next_customer =
        std::ranges::lower_bound(distribution, p, std::less{}, [](const auto &a) { return a.first; })->second;

    routes.push_back(next_customer);
    remaining_customer_ids.erase(next_customer);
  }

  auto solution = Solution(instance, std::move(routes));

  return solution;
}

auto cye::clarke_and_wright(meta::RandomEngine &gen, std::shared_ptr<Instance> instance, size_t k) -> Solution {
    const auto& customers = instance->customer_ids();
    const size_t depot_id = instance->depot_id();
    const float cargo_capacity = instance->cargo_capacity();

    float skip_probability = 0.02f;

    // Step 1: Initialize routes as Depot -> Customer -> Depot for each customer
    struct Route {
        std::vector<size_t> path;
        float total_demand; // Track cumulative demand for cargo feasibility
    };
    std::vector<Route> routes;
    for (const auto customer_id : customers) {
        routes.push_back({
            .path = {depot_id, customer_id, depot_id},
            .total_demand = instance->demand(customer_id)
        });
    }

    // Step 2: Compute savings for all pairs (i,j)
    struct SavingsPair {
        float savings;
        size_t i;
        size_t j;
    };
    std::vector<SavingsPair> savings_list;

    for (size_t idx_i = 0; idx_i < customers.size(); ++idx_i) {
        for (size_t idx_j = idx_i + 1; idx_j < customers.size(); ++idx_j) {
            const size_t i = customers[idx_i];
            const size_t j = customers[idx_j];
            const float cost_i_depot = instance->distance(i, depot_id);
            const float cost_depot_j = instance->distance(depot_id, j);
            const float cost_i_j = instance->distance(i, j);
            const float savings = cost_i_depot + cost_depot_j - cost_i_j;
            savings_list.push_back({savings, i, j});
        }
    }

    // Step 3: Sort savings in descending order
    std::ranges::sort(savings_list, [](const SavingsPair& a, const SavingsPair& b) {
        return a.savings > b.savings;
    });

    // Step 4: Merge routes based on highest savings (if feasible)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    //std::uniform_int_distribution<size_t> dist(1, k);
    k = dist(gen);
    size_t iter = 0;
    for (const auto& [savings, i, j] : savings_list) {
        // if (iter++ >= k) {
        //     break;
        // }
        if (dist(gen) < skip_probability) {
            continue;
        }
        // Find routes where:
        // - i is the last customer before depot in Route A
        // - j is the first customer after depot in Route B
        auto route_i_it = std::ranges::find_if(routes, [i](const Route& route) {
            return route.path[route.path.size() - 2] == i; // i is last before depot
        });
        auto route_j_it = std::ranges::find_if(routes, [j](const Route& route) {
            return route.path[1] == j; // j is first after depot
        });

        // Skip if:
        // - Routes not found
        // - Same route
        // - Merged demand exceeds cargo capacity
        if (route_i_it == routes.end() || route_j_it == routes.end() || route_i_it == route_j_it) {
            continue;
        }

        const float merged_demand = route_i_it->total_demand + route_j_it->total_demand;
        if (merged_demand > cargo_capacity) {
            continue; // Skip if cargo constraint violated
        }

        // Merge Route A and Route B: [..., i, depot] + [depot, j, ...] → [..., i, j, ...]
        route_i_it->path.pop_back(); // Remove depot from the end of Route A
        route_i_it->path.insert(route_i_it->path.end(), route_j_it->path.begin() + 1, route_j_it->path.end());
        route_i_it->total_demand = merged_demand; // Update total demand

        // Remove the now-merged Route B
        routes.erase(route_j_it);
    }

    // Step 5: Flatten routes into a single sequence (depot → ... → depot → depot → ... → depot)
    std::vector<size_t> flattened_routes;
    for (const auto& route : routes) {
        flattened_routes.insert(flattened_routes.end(), route.path.begin() + 1, route.path.end() - 1);
    }

    return Solution(instance, std::move(flattened_routes));
}
