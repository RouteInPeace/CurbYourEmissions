#include "cye/repair.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <print>
#include <vector>

auto cye::repair_cargo_violations_optimally(Solution &&solution, unsigned bin_cnt) -> Solution {
  auto &instance = solution.instance();
  auto dp = std::vector(bin_cnt, std::vector(solution.visited_node_cnt(),
                                             std::pair<float, unsigned>(std::numeric_limits<float>::infinity(), 0)));
  auto cargo_quant = instance.cargo_capacity() / static_cast<float>(bin_cnt - 1);
  auto eps = 1e-9f;

  // Forward
  dp[bin_cnt - 1][0].first = 0;
  for (auto j = 1UZ; j < solution.visited_node_cnt(); ++j) {
    auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
    auto distance_with_depot = instance.distance(solution.node_id(j - 1), instance.depot_id()) +
                               instance.distance(instance.depot_id(), solution.node_id(j));

    auto demand = instance.demand(solution.node_id(j));
    auto demand_quant = static_cast<unsigned>(std::ceil(demand / cargo_quant));

    for (auto i = 0u; i < bin_cnt; ++i) {
      if (dp[bin_cnt - demand_quant - 1][j].first > dp[i][j - 1].first + distance_with_depot) {
        dp[bin_cnt - demand_quant - 1][j].first = dp[i][j - 1].first + distance_with_depot;
        dp[bin_cnt - demand_quant - 1][j].second = i;
      }

      if (i + demand_quant < bin_cnt && dp[i][j].first > dp[i + demand_quant][j - 1].first + distance) {
        dp[i][j].first = dp[i + demand_quant][j - 1].first + distance;
        dp[i][j].second = i + demand_quant;
      }
    }
  }

  // Backward
  auto ind = 0u;
  auto min_const = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][solution.visited_node_cnt() - 1].first < min_const) {
      min_const = dp[i][solution.visited_node_cnt() - 1].first;
      ind = i;
    }
  }

  // std::cout << "Backtrace: ";
  auto insertion_places = std::vector<size_t>();
  for (auto j = solution.visited_node_cnt() - 1; j >= 1; --j) {
    auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
    auto demand = instance.demand(solution.node_id(j));
    auto demand_quant = static_cast<unsigned>(std::ceil(demand / cargo_quant));

    if (ind + demand_quant >= bin_cnt ||
        std::abs(dp[ind][j].first - (dp[ind + demand_quant][j - 1].first + distance)) > eps) {
      insertion_places.push_back(j);
    }
    // std::cout << ind << ' ';
    ind = dp[ind][j].second;
  }
  // std::cout << ind << "\nInsertion places: ";

  for (auto ind : insertion_places) {
    // std::cout << ind << ' ';
    solution.insert_customer(ind, instance.depot_id());
  }
  // std::cout << "\nNode ids:      ";

  // for (const auto &x : solution.routes()) {
  //   std::print("{:2} ", x);
  // }
  // std::cout << "\nQuant demands: ";
  // for (const auto &x : solution.routes()) {
  //   std::print("{:2} ", static_cast<size_t>(std::ceil(instance.demand(x) / cargo_quant)));
  // }

  // std::cout << "\n\n";

  // for (const auto &row : dp) {
  //   for (const auto &x : row) {
  //     std::print("{:6.2f},{:2}  ", x.first, x.second);
  //   }
  //   std::cout << '\n';
  // }

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
