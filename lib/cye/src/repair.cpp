#include "cye/repair.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <print>
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
  auto min_cost = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][solution.visited_node_cnt() - 1].first < min_cost) {
      min_cost = dp[i][solution.visited_node_cnt() - 1].first;
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

auto find_charging_station(const cye::Instance &instance, size_t node1_id, size_t node2_id, float remaining_battery)
    -> std::optional<size_t> {
  auto best_station_id = std::optional<size_t>{};
  auto min_distance = std::numeric_limits<float>::infinity();

  if (node1_id != instance.depot_id() && node2_id != instance.depot_id() &&
      remaining_battery >= instance.energy_required(node1_id, instance.depot_id())) {
    min_distance = instance.distance(node1_id, instance.depot_id()) + instance.distance(instance.depot_id(), node2_id);
    best_station_id = instance.depot_id();
  }

  for (const auto station_id : instance.charging_station_ids()) {
    if (station_id == node1_id || station_id == node2_id) continue;
    if (remaining_battery < instance.energy_required(node1_id, station_id)) continue;

    auto distance = instance.distance(node1_id, station_id) + instance.distance(station_id, node2_id);
    if (distance < min_distance) {
      min_distance = distance;
      best_station_id = station_id;
    }
  }

  return best_station_id;
}

auto cye::repair_energy_violations_trivially(Solution &&solution) -> Solution {
  auto &instance = solution.instance();

  auto energy = instance.battery_capacity();
  for (auto i = 1UZ; i < solution.visited_node_cnt(); i++) {
    if (energy < instance.energy_required(solution.node_id(i - 1), solution.node_id(i))) {
      auto charging_station_id = find_charging_station(instance, solution.node_id(i - 1), solution.node_id(i), energy);
      while (!charging_station_id.has_value()) {
        i -= 1;
        assert(i > 0);
        energy += instance.energy_required(solution.node_id(i - 1), solution.node_id(i));
        charging_station_id = find_charging_station(instance, solution.node_id(i - 1), solution.node_id(i), energy);
      }

      solution.insert_customer(i, *charging_station_id);
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

cye::OptimalEnergyRepair::OptimalEnergyRepair(std::shared_ptr<Instance> instance)
    : instance_(instance), unvisited_queue_(cmp_) {
  compute_cs_dist_mat_();
}

auto cye::OptimalEnergyRepair::compute_cs_dist_mat_() -> void {
  auto cs_cnt = instance_->charging_station_cnt() + 1;
  cs_dist_mat_ = std::vector(cs_cnt, std::vector(cs_cnt, std::numeric_limits<float>::infinity()));

  for (auto i = 0UZ; i < cs_cnt; ++i) {
    cs_dist_mat_[i][i] = 0.f;
    auto start_node_id = i == 0 ? instance_->depot_id() : instance_->charging_station_ids()[i - 1];
    for (auto j = i + 1; j < cs_cnt; ++j) {
      auto goal_node_id = j == 0 ? instance_->depot_id() : instance_->charging_station_ids()[j - 1];
      auto ret = find_between_(start_node_id, goal_node_id);
      if (ret) {
        cs_dist_mat_[i][j] = ret->second;
        cs_dist_mat_[j][i] = ret->second;
      }
    }
  }

  // for (const auto &row : cs_dist_mat_) {
  //   for (const auto x : row) {
  //     std::print("{:4.1f}  ", x);
  //   }
  //   std::cout << '\n';
  // }
}

auto cye::OptimalEnergyRepair::reset_() -> void {
  visited_.clear();
  while (!unvisited_queue_.empty()) {
    unvisited_queue_.pop();
  }
}

auto cye::OptimalEnergyRepair::find_between_(size_t start_node_id, size_t goal_node_id)
    -> std::optional<std::pair<std::vector<size_t>, float>> {
  reset_();

  unvisited_queue_.emplace(start_node_id, start_node_id, 0.f, instance_->distance(start_node_id, goal_node_id));

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

    for (auto cs_id : instance_->charging_station_ids()) {
      if (visited_.contains(cs_id)) {
        continue;
      }

      if (instance_->energy_required(unvisited_node.node_id, cs_id) <= instance_->battery_capacity()) {
        unvisited_queue_.emplace(cs_id, unvisited_node.node_id,
                                 unvisited_node.g + instance_->distance(unvisited_node.node_id, cs_id),
                                 instance_->distance(cs_id, goal_node_id));
      }
    }

    if (!visited_.contains(instance_->depot_id())) {
      if (instance_->energy_required(unvisited_node.node_id, instance_->depot_id()) <= instance_->battery_capacity()) {
        unvisited_queue_.emplace(instance_->depot_id(), unvisited_node.node_id,
                                 unvisited_node.g + instance_->distance(unvisited_node.node_id, instance_->depot_id()),
                                 instance_->distance(instance_->depot_id(), goal_node_id));
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

  return {{ret, visited_[goal_node_id].g}};
}

auto cye::OptimalEnergyRepair::repair(Solution &&solution, unsigned bin_cnt) -> Solution {
  auto &instance = solution.instance();
  auto dp = std::vector(bin_cnt, std::vector(solution.visited_node_cnt(),
                                             std::pair<float, unsigned>(std::numeric_limits<float>::infinity(), 0)));

  // Energy per bin
  auto energy_per_bin = instance.battery_capacity() / static_cast<float>(bin_cnt - 1);
  auto eps = 1e-12f;
  auto cs_cnt = instance_->charging_station_cnt() + 1;

  // Forward pass

  // We always start at the depot with a full battery
  dp[bin_cnt - 1][0].first = 0.f;

  // Iterate over nodes in routes
  for (auto j = 1UZ; j < solution.visited_node_cnt(); ++j) {
    // For every energy quantization
    for (auto i = 0u; i < bin_cnt; ++i) {
      // If we charge the vehicle between nodes j-1 and j
      for (auto k = 0UZ; k < cs_cnt; ++k) {
        auto entry_node_id = k == 0 ? instance_->depot_id() : instance_->charging_station_ids()[k - 1];

        // TODO: handle this edge case better
        if (entry_node_id == solution.node_id(j - 1)) {
          continue;
        }

        auto distance_to_entry_cs = instance_->distance(solution.node_id(j - 1), entry_node_id);
        auto energy_to_entry_cs = distance_to_entry_cs * instance.energy_consumption();
        auto remaining_battery = static_cast<float>(i) * energy_per_bin;

        if (energy_to_entry_cs > remaining_battery) {
          continue;
        }

        for (auto l = 0UZ; l < cs_cnt; ++l) {
          auto exit_node_id = l == 0 ? instance_->depot_id() : instance_->charging_station_ids()[l - 1];

          // TODO: handle this edge case better
          if (exit_node_id == solution.node_id(j)) {
            continue;
          }

          auto distance_from_exit_cs = instance_->distance(exit_node_id, solution.node_id(j));
          auto energy_from_exit_cs = distance_from_exit_cs * instance_->energy_consumption();
          auto energy_from_exit_cs_quant = static_cast<unsigned>(std::ceil(energy_from_exit_cs / energy_per_bin));
          auto total_distance = distance_to_entry_cs + cs_dist_mat_[k][l] + distance_from_exit_cs;

          if (energy_from_exit_cs_quant < bin_cnt &&
              dp[bin_cnt - energy_from_exit_cs_quant - 1][j].first > dp[i][j - 1].first + total_distance) {
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].first = dp[i][j - 1].first + total_distance;
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].second = i;
          }
        }
      }

      // The distance between the curent and previous node
      auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
      auto energy = distance * instance.energy_consumption();
      auto energy_quant = static_cast<unsigned>(std::ceil(energy / energy_per_bin));

      // If we go straight from the node j-1 to j and end up with a remaining battery i
      if (i + energy_quant < bin_cnt && dp[i][j].first > dp[i + energy_quant][j - 1].first + distance) {
        dp[i][j].first = dp[i + energy_quant][j - 1].first + distance;
        dp[i][j].second = i + energy_quant;
      }
    }
  }

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_cost = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][solution.visited_node_cnt() - 1].first < min_cost) {
      min_cost = dp[i][solution.visited_node_cnt() - 1].first;
      ind = i;
    }
  }

  // Trace back through the table
  auto insertion_places = std::vector<std::pair<size_t, size_t>>();
  for (auto j = solution.visited_node_cnt() - 1; j >= 1; --j) {
    auto distance = instance.distance(solution.node_id(j - 1), solution.node_id(j));
    auto energy = distance * instance.energy_consumption();
    auto energy_quant = static_cast<unsigned>(std::ceil(energy / energy_per_bin));

    // Check if we detoured through charging stations
    if (ind + energy_quant >= bin_cnt ||
        std::abs(dp[ind][j].first - (dp[ind + energy_quant][j - 1].first + distance)) > eps) {
      insertion_places.emplace_back(dp[ind][j].second, j);
    }
    // std::cout << ind << ' ';
    ind = dp[ind][j].second;
  }
  // std::cout << ind << "\nInsertion places: ";

  // Insert charging stations into the solution
  for (auto [i, j] : insertion_places) {
    // std::cout << j << ' ';

    auto min_entry_node = std::numeric_limits<size_t>::max();
    auto min_exit_node = std::numeric_limits<size_t>::max();
    auto min_dist = std::numeric_limits<float>::infinity();
    for (auto k = 0UZ; k < cs_cnt; ++k) {
      auto entry_node_id = k == 0 ? instance_->depot_id() : instance_->charging_station_ids()[k - 1];

      // TODO: handle this edge case better
      if (entry_node_id == solution.node_id(j - 1)) {
        continue;
      }

      auto distance_to_entry_cs = instance_->distance(solution.node_id(j - 1), entry_node_id);
      auto energy_to_entry_cs = distance_to_entry_cs * instance.energy_consumption();
      auto remaining_battery = static_cast<float>(i) * energy_per_bin;

      if (energy_to_entry_cs > remaining_battery) {
        continue;
      }

      for (auto l = 0UZ; l < cs_cnt; ++l) {
        auto exit_node_id = l == 0 ? instance_->depot_id() : instance_->charging_station_ids()[l - 1];

        // TODO: handle this edge case better
        if (exit_node_id == solution.node_id(j)) {
          continue;
        }

        auto distance_from_exit_cs = instance_->distance(exit_node_id, solution.node_id(j));
        auto energy_from_exit_cs = distance_from_exit_cs * instance_->energy_consumption();
        auto energy_from_exit_cs_quant = static_cast<unsigned>(std::ceil(energy_from_exit_cs / energy_per_bin));
        auto total_distance = distance_to_entry_cs + cs_dist_mat_[k][l] + distance_from_exit_cs;

        if (energy_from_exit_cs_quant < bin_cnt &&
            (dp[bin_cnt - energy_from_exit_cs_quant - 1][j].first - (dp[i][j - 1].first + total_distance)) < eps) {
          if (total_distance < min_dist) {
            min_dist = total_distance;
            min_entry_node = entry_node_id;
            min_exit_node = exit_node_id;
          }
        }
      }
    }

    if (min_entry_node == min_exit_node) {
      solution.insert_customer(j, min_entry_node);
    } else {
      solution.insert_customer(j, min_exit_node);

      auto [cs_ids, _] = *find_between_(min_entry_node, min_exit_node);
      for (auto cs_id : cs_ids) {
        solution.insert_customer(j, cs_id);
      }

      solution.insert_customer(j, min_entry_node);
    }
  }
  // std::cout << '\n';

  // std::println("Ind: {}, Min const: {}", ind, min_cost);

  // for (const auto &row : dp) {
  //   for (const auto &x : row) {
  //     std::print("{:5.1f},{:2}  ", x.first, x.second);
  //   }
  //   std::cout << '\n';
  // }

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
