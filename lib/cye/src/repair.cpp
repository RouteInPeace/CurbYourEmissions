#include "cye/repair.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "cye/patchable_vector.hpp"
#include "cye/solution.hpp"

struct CargoDPCell {
  CargoDPCell() : dist(std::numeric_limits<double>::infinity()), prev(0), inserted(false) {}

  double dist;
  unsigned prev;
  bool inserted;
};

auto cye::patch_cargo_optimally(Solution &solution, unsigned bin_cnt) -> void {
  auto visited_node_cnt = solution.visited_node_cnt();
  auto &instance = solution.instance();
  auto dp = std::vector(bin_cnt, std::vector(visited_node_cnt + 2, CargoDPCell()));

  // Amount of cargo per bin
  auto cargo_quant = instance.cargo_capacity() / static_cast<double>(bin_cnt - 1);

  // Forward pass

  // We always start at the depot with the full capacity remaining
  dp[bin_cnt - 1][0].dist = 0;

  // Iterate over nodes in routes
  auto previous_node_id = instance.depot_id();
  auto j = 1UZ;
  solution.base().push_back(instance.depot_id());
  for (auto it = solution.routes().begin(); it != solution.routes().end(); ++it) {
    auto current_node_id = *it;

    // The distance between the curent ad previous node
    auto distance = instance.distance(previous_node_id, current_node_id);
    // The distance if we go from the previous node to the depo and back to the current node
    auto distance_with_depot = instance.distance(previous_node_id, instance.depot_id()) +
                               instance.distance(instance.depot_id(), current_node_id);

    auto demand = instance.demand(current_node_id);
    auto demand_quant = static_cast<unsigned>(std::ceil(demand / cargo_quant));

    // For every cargo quantization
    for (auto i = 0u; i < bin_cnt; ++i) {
      // If we go from the node j-1 with capacity i to the depot and than back to the node j
      if (dp[bin_cnt - demand_quant - 1][j].dist > dp[i][j - 1].dist + distance_with_depot) {
        dp[bin_cnt - demand_quant - 1][j].dist = dp[i][j - 1].dist + distance_with_depot;
        dp[bin_cnt - demand_quant - 1][j].prev = i;
        dp[bin_cnt - demand_quant - 1][j].inserted = true;
      }

      // If we go straight from the node j-1 to j and end up with a remaining capacity i
      if (i + demand_quant < bin_cnt && dp[i][j].dist > dp[i + demand_quant][j - 1].dist + distance) {
        dp[i][j].dist = dp[i + demand_quant][j - 1].dist + distance;
        dp[i][j].prev = i + demand_quant;
        dp[i][j].inserted = false;
      }
    }

    ++j;
    previous_node_id = current_node_id;
  }
  solution.base().pop_back();

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_cost = std::numeric_limits<double>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][visited_node_cnt + 1].dist < min_cost) {
      min_cost = dp[i][visited_node_cnt + 1].dist;
      ind = i;
    }
  }

  // Trace back through the table
  auto patch = Patch<size_t>();
  patch.add_change(solution.visited_node_cnt(), instance.depot_id());
  for (auto j = solution.visited_node_cnt() + 1; j >= 1; --j) {
    // Check if we detoured to the depot
    if (dp[ind][j].inserted) {
      patch.add_change(j - 1, instance.depot_id());
    }
    ind = dp[ind][j].prev;
  }
  patch.add_change(0, instance.depot_id());
  patch.reverse();
  solution.add_patch(std::move(patch));
}

auto cye::patch_cargo_trivially(Solution &solution) -> void {
  auto &instance = solution.instance();
  auto cargo_capacity = instance.cargo_capacity();

  auto patch = Patch<size_t>();
  patch.add_change(0, instance.depot_id());

  auto i = 0UZ;
  for (auto node_id : solution.routes()) {
    cargo_capacity -= instance.demand(node_id);
    if (cargo_capacity < 0) {
      patch.add_change(i, instance.depot_id());
      cargo_capacity = instance.cargo_capacity() - instance.demand(node_id);
    }

    ++i;
  }
  patch.add_change(solution.visited_node_cnt(), instance.depot_id());
  solution.add_patch(std::move(patch));
}

auto find_charging_station(const cye::Instance &instance, size_t node1_id, size_t node2_id, double remaining_battery)
    -> std::optional<std::pair<size_t, double>> {
  auto best_station_id = std::optional<size_t>{};
  auto min_distance = std::numeric_limits<double>::infinity();

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

  return std::pair<size_t, double>(*best_station_id, min_distance);
}

auto cye::patch_energy_trivially(Solution &solution) -> void {
  auto &instance = solution.instance();

  auto energy = instance.battery_capacity();
  auto previous_node_id = *solution.routes().begin();
  auto patch = Patch<size_t>();
  auto i = 1UZ;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end();) {
    auto current_node_id = *it;

    if (energy < instance.energy_required(previous_node_id, current_node_id)) {
      auto charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
      while (!charging_station_id.has_value()) {
        i -= 1;
        --it;
        current_node_id = previous_node_id;

        if (!patch.empty() && i == patch.back().ind) {
          previous_node_id = patch.back().value;
        } else {
          auto tmp_it = it;
          previous_node_id = *(--tmp_it);
        }

        energy += instance.energy_required(previous_node_id, current_node_id);
        charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
      }

      patch.add_change(i, (*charging_station_id).first);
      energy = instance.battery_capacity();
      previous_node_id = (*charging_station_id).first;
    } else {
      if (current_node_id == instance.depot_id()) {
        energy = instance.battery_capacity();
      } else {
        energy -= instance.energy_required(previous_node_id, current_node_id);
      }
      ++i;
      ++it;
      previous_node_id = current_node_id;
    }
  }

  solution.add_patch(std::move(patch));
}

namespace {
bool is_energy_valid(std::vector<size_t> const &route, cye::Instance const &instance,
                     std::vector<std::pair<size_t, size_t>> const &cs_ids) {
  auto energy = instance.battery_capacity();
  auto previous_node_id = instance.depot_id();

  size_t cs_index = 0;

  for (auto i = 0UZ; i < route.size();) {
    auto current_node_id = std::invoke([&]() {
      if (cs_index < cs_ids.size() && i == cs_ids[cs_index].first) {
        ++cs_index;
        return cs_ids[cs_index - 1].second;
      }
      ++i;
      return route[i - 1];
    });

    if (energy < instance.energy_required(previous_node_id, current_node_id)) {
      return false;
    }
    if (instance.is_charging_station(current_node_id) || current_node_id == instance.depot_id()) {
      energy = instance.battery_capacity();
    } else {
      energy -= instance.energy_required(previous_node_id, current_node_id);
    }
    previous_node_id = current_node_id;
  }
  return true;
}
}  // namespace

auto cye::patch_energy_removal_heuristic(cye::Solution &solution) -> void {
  auto &instance = solution.instance();

  struct ChargingStation_ {
    double dist;
    size_t ind;
    size_t cs_id;

    bool operator<(const ChargingStation_ &other) const { return dist < other.dist; }
  };
  std::priority_queue<ChargingStation_> charging_stations_queue;
  std::vector<std::pair<size_t, size_t>> charging_stations;

  auto max_energy = instance.battery_capacity();
  auto previous_node_id = *solution.routes().begin();
  auto i = 1UZ;

  std::vector<size_t> route{instance.depot_id()};
  for (auto it = ++solution.routes().begin(); it != solution.routes().end(); ++it) {
    auto current_node_id = *it;
    route.emplace_back(current_node_id);
    auto charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, max_energy);

    if (!charging_station_id.has_value()) {
      std::cout << "No charging stations between " << previous_node_id << " and " << current_node_id << "\n";
      continue;
    }

    ChargingStation_ cs{
        .dist = (*charging_station_id).second,
        .ind = i,
        .cs_id = (*charging_station_id).first,
    };
    charging_stations_queue.push(cs);
    charging_stations.emplace_back(cs.ind, cs.cs_id);
    ++i;
    ++it;
  }

  while (!charging_stations_queue.empty()) {
    auto cs = charging_stations_queue.top();
    charging_stations_queue.pop();

    auto copy_cs = charging_stations;
    auto it = std::find(copy_cs.begin(), copy_cs.end(), std::make_pair(cs.ind, cs.cs_id));
    copy_cs.erase(it);

    if (is_energy_valid(route, instance, copy_cs)) {
      charging_stations = std::move(copy_cs);
    }
  }

  auto patch = Patch<size_t>();
  for (auto cs : charging_stations) {
    patch.add_change(cs.first, cs.second);
  }
  solution.add_patch(std::move(patch));
}

// auto cye::patch_energy_trivially(Solution &solution) -> void {
//   auto &instance = solution.instance();

//   auto energy = instance.battery_capacity();
//   auto previous_node_id = *solution.routes().begin();
//   auto patch = Patch<size_t>();
//   auto i = 1UZ;
//   for (auto it = ++solution.routes().begin(); it != solution.routes().end();) {
//     auto current_node_id = *it;

//     if (energy < instance.energy_required(previous_node_id, current_node_id)) {
//       auto charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
//       while (!charging_station_id.has_value()) {
//         i -= 1;
//         --it;
//         current_node_id = previous_node_id;

//         if (!patch.empty() && i == patch.back().ind) {
//           previous_node_id = patch.back().value;
//         } else {
//           auto tmp_it = it;
//           previous_node_id = *(--tmp_it);
//         }

//         energy += instance.energy_required(previous_node_id, current_node_id);
//         charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
//       }

//       patch.add_change(i, (*charging_station_id).first);
//       energy = instance.battery_capacity();
//       previous_node_id = (*charging_station_id).first;
//     } else {
//       if (current_node_id == instance.depot_id()) {
//         energy = instance.battery_capacity();
//       } else {
//         energy -= instance.energy_required(previous_node_id, current_node_id);
//       }
//       ++i;
//       ++it;
//       previous_node_id = current_node_id;
//     }
//   }

//   solution.add_patch(std::move(patch));
// }

auto cye::patch_energy_optimal_heuristic(Solution &solution) -> void {
  auto &instance = solution.instance();

  auto energy = instance.battery_capacity();
  auto previous_node_id = *solution.routes().begin();
  auto patch = Patch<size_t>();
  auto i = 1UZ;

  auto closest_charging_station = [&](size_t node_id, double energy_remaining) -> std::optional<size_t> {
    auto cs_id = instance.closest_charging_station(node_id);
    if (energy_remaining >= instance.energy_required(node_id, cs_id)) {
      return cs_id;
    }
    return std::nullopt;
  };

  auto previous_depot_it = solution.routes().begin();
  auto previous_cs_id = 0UZ;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end();) {
    auto current_node_id = *it;

    if (energy < instance.energy_required(previous_node_id, current_node_id)) {
      std::pair<size_t, size_t> best_cs;
      double best_cost = std::numeric_limits<double>::infinity();

      while (it != previous_depot_it && previous_cs_id+1 < i) {
        auto cs = closest_charging_station(current_node_id != instance.depot_id() ? current_node_id : previous_cs_id,
                                           energy);
        if (cs.has_value()) {
          auto distance_added = instance.distance(previous_node_id, *cs) + instance.distance(*cs, current_node_id) -
                                instance.distance(previous_node_id, current_node_id);
          if (distance_added < best_cost) {
            best_cost = distance_added;
            best_cs = std::make_pair(i, *cs);
          }
        }
        --i;
        --it;
        current_node_id = previous_node_id;
        previous_node_id = *it;
        energy += instance.energy_required(previous_node_id, current_node_id);
      }
      if(best_cost == std::numeric_limits<double>::infinity()) {
        cye::OptimalEnergyRepair repair(solution.instance_ptr());
        repair.patch(solution, 151U);
        return;
      }

      patch.add_change(best_cs.first, best_cs.second);
      previous_cs_id = best_cs.first;
      energy = instance.battery_capacity();
      previous_node_id = best_cs.second;
      while (i < best_cs.first) {
        ++i;
        ++it;
      }
    } else {
      if (current_node_id == instance.depot_id()) {
        energy = instance.battery_capacity();
        previous_depot_it = it;
      } else {
        energy -= instance.energy_required(previous_node_id, current_node_id);
      }
      ++i;
      ++it;
      previous_node_id = current_node_id;
    }
  }
  //std::cout << "Sucess\n";
  solution.add_patch(std::move(patch));
}

cye::OptimalEnergyRepair::OptimalEnergyRepair(std::shared_ptr<Instance> instance)
    : instance_(instance), unvisited_queue_(cmp_) {
  compute_cs_dist_mat_();
}

auto cye::OptimalEnergyRepair::compute_cs_dist_mat_() -> void {
  auto cs_cnt = instance_->charging_station_cnt() + 1;
  cs_dist_mat_ = std::vector(cs_cnt, std::vector(cs_cnt, std::numeric_limits<double>::infinity()));

  for (auto i = 0UZ; i < cs_cnt; ++i) {
    cs_dist_mat_[i][i] = 0.0;
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
}

auto cye::OptimalEnergyRepair::reset_() -> void {
  visited_.clear();
  while (!unvisited_queue_.empty()) {
    unvisited_queue_.pop();
  }
}

auto cye::OptimalEnergyRepair::find_between_(size_t start_node_id, size_t goal_node_id)
    -> std::optional<std::pair<std::vector<size_t>, double>> {
  reset_();

  unvisited_queue_.emplace(start_node_id, start_node_id, 0.0, instance_->distance(start_node_id, goal_node_id));

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

auto cye::OptimalEnergyRepair::fill_dp(Solution &solution, unsigned bin_cnt) -> std::vector<std::vector<DPCell>> {
  auto visited_node_cnt = solution.visited_node_cnt();
  auto &instance = solution.instance();

  auto no_cs = std::numeric_limits<uint16_t>::max();
  auto dp = std::vector(bin_cnt, std::vector(visited_node_cnt, DPCell()));

  // Energy per bin
  auto energy_per_bin = instance.battery_capacity() / static_cast<double>(bin_cnt - 1);
  auto cs_cnt = instance_->charging_station_cnt() + 1;

  // Forward pass

  // We always start at the depot with a full battery
  dp[bin_cnt - 1][0].dist = 0.0;

  // Iterate over nodes in routes
  auto previous_node_id = *solution.routes().begin();
  auto j = 1UZ;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end(); ++it) {
    auto current_node_id = *it;
    // For every energy quantization
    for (auto i = 0u; i < bin_cnt; ++i) {
      // If we charge the vehicle between nodes j-1 and j
      for (auto k = 0UZ; k < cs_cnt; ++k) {
        auto entry_node_id = k == 0 ? instance_->depot_id() : instance_->charging_station_ids()[k - 1];

        if (entry_node_id == previous_node_id) {
          continue;
        }

        auto distance_to_entry_cs = instance_->distance(previous_node_id, entry_node_id);
        auto energy_to_entry_cs = distance_to_entry_cs * instance.energy_consumption();
        auto remaining_battery = static_cast<double>(i) * energy_per_bin;

        if (energy_to_entry_cs > remaining_battery) {
          continue;
        }

        // for (auto l = 0UZ; l < cs_cnt; ++l) {
        for (auto l = k; l == k; ++l) {
        auto exit_node_id = l == 0 ? instance_->depot_id() : instance_->charging_station_ids()[l - 1];

          // Not really necessary, but it cleans up the table
          if (instance.is_charging_station(current_node_id) && exit_node_id != current_node_id) {
            continue;
          }

          auto distance_from_exit_cs = instance_->distance(exit_node_id, current_node_id);
          auto energy_from_exit_cs = distance_from_exit_cs * instance_->energy_consumption();
          auto energy_from_exit_cs_quant = static_cast<unsigned>(std::ceil(energy_from_exit_cs / energy_per_bin));
          auto total_distance = distance_to_entry_cs + cs_dist_mat_[k][l] + distance_from_exit_cs;

          if (energy_from_exit_cs_quant < bin_cnt &&
              dp[bin_cnt - energy_from_exit_cs_quant - 1][j].dist > dp[i][j - 1].dist + total_distance) {
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].dist = dp[i][j - 1].dist + total_distance;
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].prev = i;
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].entry_ind = k;
            dp[bin_cnt - energy_from_exit_cs_quant - 1][j].exit_ind = l;
          }
        }
      }

      // The distance between the curent and the previous node
      auto distance = instance.distance(previous_node_id, current_node_id);
      auto energy = distance * instance.energy_consumption();
      auto energy_quant = static_cast<unsigned>(std::ceil(energy / energy_per_bin));

      // If we go straight from the node j-1 to j and end up with a remaining battery i
      auto bin_after = instance.is_charging_station(current_node_id) ? bin_cnt - 1 : i;
      if (i + energy_quant < bin_cnt && dp[bin_after][j].dist > dp[i + energy_quant][j - 1].dist + distance) {
        dp[bin_after][j].dist = dp[i + energy_quant][j - 1].dist + distance;
        dp[bin_after][j].prev = i + energy_quant;
        dp[bin_after][j].entry_ind = no_cs;
        dp[bin_after][j].exit_ind = no_cs;
      }
    }

    previous_node_id = current_node_id;
    ++j;
  }

  return dp;
}

auto cye::OptimalEnergyRepair::patch(Solution &solution, unsigned bin_cnt) -> void {
  auto dp = fill_dp(solution, bin_cnt);
  auto visited_node_cnt = solution.visited_node_cnt();
  auto no_cs = std::numeric_limits<uint16_t>::max();

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_cost = std::numeric_limits<double>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][visited_node_cnt - 1].dist < min_cost) {
      min_cost = dp[i][visited_node_cnt - 1].dist;
      ind = i;
    }
  }

  if (min_cost == std::numeric_limits<double>::infinity()) {
    throw std::runtime_error("Solution not found");
  }

  // Trace back through the table
  auto patch = Patch<size_t>();
  for (auto j = solution.visited_node_cnt() - 1; j >= 1; --j) {
    if (dp[ind][j].entry_ind != no_cs) {
      auto entry_node_id = dp[ind][j].entry_ind == 0 ? instance_->depot_id()
                                                     : instance_->charging_station_ids()[dp[ind][j].entry_ind - 1];
      auto exit_node_id =
          dp[ind][j].exit_ind == 0 ? instance_->depot_id() : instance_->charging_station_ids()[dp[ind][j].exit_ind - 1];

      if (entry_node_id == exit_node_id) {
        patch.add_change(j, entry_node_id);
      } else {
        patch.add_change(j, exit_node_id);

        auto [cs_ids, _] = *find_between_(entry_node_id, exit_node_id);
        for (auto cs_id : cs_ids) {
          patch.add_change(j, cs_id);
        }

        patch.add_change(j, entry_node_id);
      }
    }

    ind = dp[ind][j].prev;
  }

  patch.reverse();
  solution.add_patch(std::move(patch));
}
