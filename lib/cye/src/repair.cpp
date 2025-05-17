#include "cye/repair.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

auto cye::repair_cargo_violations_optimally(Solution &&solution, unsigned bin_cnt) -> Solution {
  auto &instance = solution.instance();
  auto dp = std::vector(bin_cnt, std::vector(solution.visited_node_cnt(),
                                             std::pair<float, unsigned>(std::numeric_limits<float>::infinity(), 0)));

  // Amount of cargo per bin
  auto cargo_quant = instance.cargo_capacity() / static_cast<float>(bin_cnt - 1);
  auto eps = 1e-12f;

  // Forward pass

  // We always start at the depot with the full capacity remaining
  dp[bin_cnt - 1][0].first = 0;

  // Iterate over nodes in routes
  for (auto j = 1UZ; j < solution.visited_node_cnt(); ++j) {
    // The distance between the curent ad previous node
    auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
    // The distance if we go from the previous node to the depo and back to the current node
    auto distance_with_depot = instance.distance(solution.node_id(j - 1), instance.depot_id()) +
                               instance.distance(instance.depot_id(), solution.node_id(j));

    auto demand = instance.demand(solution.node_id(j));
    auto demand_quant = static_cast<unsigned>(std::ceil(demand / cargo_quant));

    // For every cargo quantization
    for (auto i = 0u; i < bin_cnt; ++i) {
      // If we go from the node j-1 with capacity i to the depot and than back to the node j
      if (dp[bin_cnt - demand_quant - 1][j].first > dp[i][j - 1].first + distance_with_depot) {
        dp[bin_cnt - demand_quant - 1][j].first = dp[i][j - 1].first + distance_with_depot;
        dp[bin_cnt - demand_quant - 1][j].second = i;
      }

      // If we go straight from the node j-1 to j and end up with a remaining capacity i
      if (i + demand_quant < bin_cnt && dp[i][j].first > dp[i + demand_quant][j - 1].first + distance) {
        dp[i][j].first = dp[i + demand_quant][j - 1].first + distance;
        dp[i][j].second = i + demand_quant;
      }
    }
  }

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_const = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][solution.visited_node_cnt() - 1].first < min_const) {
      min_const = dp[i][solution.visited_node_cnt() - 1].first;
      ind = i;
    }
  }

  // Trace back through the table
  auto insertion_places = std::vector<size_t>();
  for (auto j = solution.visited_node_cnt() - 1; j >= 1; --j) {
    auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
    auto demand = instance.demand(solution.node_id(j));
    auto demand_quant = static_cast<unsigned>(std::ceil(demand / cargo_quant));

    // Check if we detoured to the depot
    if (ind + demand_quant >= bin_cnt ||
        std::abs(dp[ind][j].first - (dp[ind + demand_quant][j - 1].first + distance)) > eps) {
      insertion_places.push_back(j);
    }
    ind = dp[ind][j].second;
  }

  // Insert depot into the solution
  for (auto ind : insertion_places) {
    solution.insert_customer(ind, instance.depot_id());
  }

  return solution;
}

auto cye::repair_cargo_violations_trivially(Solution &&solution) -> Solution {
  auto &instance = solution.instance();
  auto cargo_capacity = instance.cargo_capacity();

  for (auto i = 0UZ; i < solution.visited_node_cnt(); ++i) {
    cargo_capacity -= instance.demand(solution.node_id(i));
    if (cargo_capacity < 0) {
      solution.insert_customer(i, instance.depot_id());
      cargo_capacity = instance.cargo_capacity() - instance.demand(solution.node_id(i));
    }
  }

  return solution;
}

cye::Solution cye::greedy_repair(Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto const &unassigned_ids = copy.unassigned_customers();

  for (auto unassigned_id : unassigned_ids) {
    auto best_cost = std::numeric_limits<double>::max();
    // charging stations can get reordered
    auto best_solution = cye::Solution(nullptr, {});

    auto update_cost = [&](cye::Solution &&new_solution) {
      auto cost = copy.get_cost();
      if (cost < best_cost) {
        best_cost = cost;
        best_solution = std::move(new_solution);
      }
    };

    for (size_t i = 1; i < copy.routes().size(); ++i) {
      auto new_copy = copy;
      new_copy.insert_customer(i, unassigned_id);
      if (new_copy.is_energy_and_cargo_valid()) {
        update_cost(std::move(new_copy));
        continue;
      }
      if (new_copy.reorder_charging_station(i)) {
        update_cost(std::move(new_copy));
      }
    }
    if (best_solution.routes().size()) {
      copy = best_solution;
    }
  }

  copy.clear_unassigned_customers();
  return copy;
}
