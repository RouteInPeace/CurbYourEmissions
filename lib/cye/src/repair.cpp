#include "cye/repair.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <unordered_map>
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

cye::ChargingStationFinder::ChargingStationFinder() : unvisited_queue_(cmp_) {}

auto cye::ChargingStationFinder::clear_() -> void {
  visited_.clear();
  while (!unvisited_queue_.empty()) {
    unvisited_queue_.pop();
  }
}

auto cye::ChargingStationFinder::find_between(cye::Instance const &instance, size_t start_node_id, size_t goal_node_id,
                                              float remaining_battery) -> std::optional<std::vector<size_t>> {
  clear_();

  for (auto cs_id : instance.charging_station_ids()) {
    if (cs_id == start_node_id || cs_id == goal_node_id) {
      continue;
    }

    if (instance.energy_required(start_node_id, cs_id) <= remaining_battery) {
      unvisited_queue_.emplace(cs_id, start_node_id, instance.distance(start_node_id, cs_id),
                               instance.distance(cs_id, goal_node_id));
    }
  }

  if (start_node_id != instance.depot_id() && goal_node_id != instance.depot_id()) {
    if (instance.energy_required(start_node_id, instance.depot_id()) <= remaining_battery) {
      unvisited_queue_.emplace(instance.depot_id(), start_node_id,
                               instance.distance(start_node_id, instance.depot_id()),
                               instance.distance(instance.depot_id(), goal_node_id));
    }
  }

  while (!unvisited_queue_.empty()) {
    auto unvisited_node = unvisited_queue_.top();
    unvisited_queue_.pop();

    if (visited_.contains(unvisited_node.node_id)) {
      continue;
    }

    visited_.emplace(unvisited_node.node_id, VisitedNode_{unvisited_node.g, unvisited_node.parent});

    if (unvisited_node.node_id == goal_node_id) {
      break;
    }

    if (instance.energy_required(unvisited_node.node_id, goal_node_id) <= instance.battery_capacity()) {
      unvisited_queue_.emplace(goal_node_id, unvisited_node.node_id,
                               unvisited_node.g + instance.distance(unvisited_node.node_id, goal_node_id), 0.f);
    }

    for (auto cs_id : instance.charging_station_ids()) {
      if (visited_.contains(cs_id)) {
        continue;
      }

      if (instance.energy_required(unvisited_node.node_id, cs_id) <= instance.battery_capacity()) {
        unvisited_queue_.emplace(cs_id, unvisited_node.node_id,
                                 unvisited_node.g + instance.distance(unvisited_node.node_id, cs_id),
                                 instance.distance(cs_id, goal_node_id));
      }
    }

    if (!visited_.contains(instance.depot_id())) {
      if (instance.energy_required(unvisited_node.node_id, instance.depot_id()) <= instance.battery_capacity()) {
        unvisited_queue_.emplace(instance.depot_id(), unvisited_node.node_id,
                                 unvisited_node.g + instance.distance(unvisited_node.node_id, instance.depot_id()),
                                 instance.distance(instance.depot_id(), goal_node_id));
      }
    }
  }

  if (!visited_.contains(goal_node_id)) {
    return {};
  }

  auto ret = std::vector<size_t>();
  for (auto node_id = visited_[goal_node_id].parent; node_id != start_node_id; node_id = visited_[node_id].parent) {
    ret.push_back(node_id);
  }

  return {ret};
}

auto cye::repair_energy_violations_trivially(Solution &&solution) -> Solution {
  auto &instance = solution.instance();
  auto cs_finder = ChargingStationFinder();

  auto energy = instance.battery_capacity();
  for (auto i = 1UZ; i < solution.visited_node_cnt(); i++) {
    if (energy < instance.energy_required(solution.node_id(i - 1), solution.node_id(i))) {
      auto cs_ids = cs_finder.find_between(instance, solution.node_id(i - 1), solution.node_id(i), energy);
      while (!cs_ids.has_value()) {
        i -= 1;
        assert(i > 0);
        energy += instance.energy_required(solution.node_id(i - 1), solution.node_id(i));
        cs_ids = cs_finder.find_between(instance, solution.node_id(i - 1), solution.node_id(i), energy);
      }

      for (auto cs_id : *cs_ids) {
        solution.insert_customer(i, cs_id);
      }

      energy = instance.battery_capacity();
    } else {
      if (solution.node_id(i) == instance.depot_id()) {
        energy = instance.battery_capacity();
      } else {
        energy -= instance.energy_required(solution.node_id(i - 1), solution.node_id(i));
      }
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
