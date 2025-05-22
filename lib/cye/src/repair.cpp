#include "cye/repair.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "cye/patchable_vector.hpp"
#include "cye/solution.hpp"

struct CargoDPCell {
  CargoDPCell() : dist(std::numeric_limits<float>::infinity()), prev(0), inserted(false) {}

  float dist;
  unsigned prev;
  bool inserted;
};

auto cye::patch_cargo_optimally(Solution &solution, unsigned bin_cnt) -> void {
  auto visited_node_cnt = solution.visited_node_cnt();
  auto &instance = solution.instance();
  auto dp = std::vector(bin_cnt, std::vector(visited_node_cnt, CargoDPCell()));

  // Amount of cargo per bin
  auto cargo_quant = instance.cargo_capacity() / static_cast<float>(bin_cnt - 1);

  // Forward pass

  // We always start at the depot with the full capacity remaining
  dp[bin_cnt - 1][0].dist = 0;

  // Iterate over nodes in routes
  auto previous_node_id = *solution.routes().begin();
  auto j = 1UZ;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end(); ++it) {
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

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_cost = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][visited_node_cnt - 1].dist < min_cost) {
      min_cost = dp[i][visited_node_cnt - 1].dist;
      ind = i;
    }
  }

  // Trace back through the table
  auto patch = Patch<size_t>();
  for (auto j = solution.visited_node_cnt() - 1; j >= 1; --j) {
    // Check if we detoured to the depot
    if (dp[ind][j].inserted) {
      patch.add_change(j, instance.depot_id());
    }
    ind = dp[ind][j].prev;
  }

  patch.reverse();
  solution.routes().add_patch(std::move(patch));
}

auto cye::patch_cargo_trivially(Solution &solution) -> void {
  auto &instance = solution.instance();
  auto cargo_capacity = instance.cargo_capacity();

  auto patch = Patch<size_t>();
  auto i = 0UZ;
  for (auto node_id : solution.routes()) {
    cargo_capacity -= instance.demand(node_id);
    if (cargo_capacity < 0) {
      patch.add_change(i, instance.depot_id());
      cargo_capacity = instance.cargo_capacity() - instance.demand(node_id);
    }

    ++i;
  }

  solution.routes().add_patch(std::move(patch));
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

auto cye::patch_energy_trivially(Solution &solution) -> void {
  auto &instance = solution.instance();

  auto energy = instance.battery_capacity();
  auto previous_node_id = *solution.routes().begin();
  auto patch = Patch<size_t>();
  auto offset = 0UZ;
  auto i = 1UZ;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end(); ++it) {
    auto current_node_id = *it;

    if (energy < instance.energy_required(previous_node_id, current_node_id)) {
      auto charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
      while (!charging_station_id.has_value()) {
        i -= 1;
        --it;
        current_node_id = previous_node_id;
        auto tmp_it = it;
        previous_node_id = *(--tmp_it);
        assert(i > 0);
        energy += instance.energy_required(previous_node_id, current_node_id);
        charging_station_id = find_charging_station(instance, previous_node_id, current_node_id, energy);
      }

      patch.add_change(i + offset, *charging_station_id);
      ++offset;
      energy = instance.battery_capacity();
    } else {
      if (current_node_id == instance.depot_id()) {
        energy = instance.battery_capacity();
      } else {
        energy -= instance.energy_required(previous_node_id, current_node_id);
      }
    }

    ++i;
    previous_node_id = current_node_id;
  }

  solution.routes().add_patch(std::move(patch));
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

struct DPCell {
  DPCell()
      : dist(std::numeric_limits<float>::infinity()),
        prev(0),
        entry_ind(std::numeric_limits<uint16_t>::max()),
        exit_ind(std::numeric_limits<uint16_t>::max()) {}

  float dist;
  unsigned prev;
  uint16_t entry_ind;
  uint16_t exit_ind;
};

auto cye::OptimalEnergyRepair::patch(Solution &solution, unsigned bin_cnt) -> void {
  auto visited_node_cnt = solution.visited_node_cnt();
  auto &instance = solution.instance();

  auto no_cs = std::numeric_limits<uint16_t>::max();
  auto dp = std::vector(bin_cnt, std::vector(visited_node_cnt, DPCell()));

  // Energy per bin
  auto energy_per_bin = instance.battery_capacity() / static_cast<float>(bin_cnt - 1);
  auto cs_cnt = instance_->charging_station_cnt() + 1;

  // Forward pass

  // We always start at the depot with a full battery
  dp[bin_cnt - 1][0].dist = 0.f;

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

        // TODO: handle this edge case better
        if (entry_node_id == previous_node_id) {
          continue;
        }

        auto distance_to_entry_cs = instance_->distance(previous_node_id, entry_node_id);
        auto energy_to_entry_cs = distance_to_entry_cs * instance.energy_consumption();
        auto remaining_battery = static_cast<float>(i) * energy_per_bin;

        if (energy_to_entry_cs > remaining_battery) {
          continue;
        }

        for (auto l = 0UZ; l < cs_cnt; ++l) {
          auto exit_node_id = l == 0 ? instance_->depot_id() : instance_->charging_station_ids()[l - 1];

          // TODO: handle this edge case better
          if (exit_node_id == current_node_id) {
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
      if (i + energy_quant < bin_cnt && dp[i][j].dist > dp[i + energy_quant][j - 1].dist + distance) {
        dp[i][j].dist = dp[i + energy_quant][j - 1].dist + distance;
        dp[i][j].prev = i + energy_quant;
        dp[i][j].entry_ind = no_cs;
        dp[i][j].exit_ind = no_cs;
      }
    }

    previous_node_id = current_node_id;
    ++j;
  }

  // Backward pass

  // Find the smallest cost in the last column
  auto ind = 0u;
  auto min_cost = std::numeric_limits<float>::infinity();
  for (auto i = 0u; i < bin_cnt; ++i) {
    if (dp[i][visited_node_cnt - 1].dist < min_cost) {
      min_cost = dp[i][visited_node_cnt - 1].dist;
      ind = i;
    }
  }

  if (min_cost == std::numeric_limits<float>::infinity()) {
    throw std::runtime_error("Solution not found");
  }

  // Trace back through the table
  auto patch = Patch<size_t>();
  auto offset = 0UZ;
  for (auto j = 1UZ; j < visited_node_cnt; ++j) {
    if (dp[ind][j].entry_ind != no_cs) {
      auto entry_node_id = dp[ind][j].entry_ind == 0 ? instance_->depot_id()
                                                     : instance_->charging_station_ids()[dp[ind][j].entry_ind - 1];
      auto exit_node_id =
          dp[ind][j].exit_ind == 0 ? instance_->depot_id() : instance_->charging_station_ids()[dp[ind][j].exit_ind - 1];

      if (entry_node_id == exit_node_id) {
        patch.add_change(j + offset, entry_node_id);
        ++offset;
      } else {
        patch.add_change(j + offset, exit_node_id);
        ++offset;

        auto [cs_ids, _] = *find_between_(entry_node_id, exit_node_id);
        for (auto cs_id : cs_ids) {
          patch.add_change(j + offset, cs_id);
          ++offset;
        }

        patch.add_change(j + offset, entry_node_id);
        ++offset;
      }
    }

    ind = dp[ind][j].prev;
  }

  solution.routes().add_patch(std::move(patch));
}

namespace {
struct Insertion {
  size_t route_id;
  size_t customer_id;
  double cost = std::numeric_limits<double>::max();

  friend bool operator<(Insertion const &x, Insertion const &y) { return x.cost < y.cost; }
};

auto get_customers_with_endpoints(cye::Solution const &solution) -> cye::Solution {
  auto customers = std::vector<size_t>();
  auto &instance = solution.instance();
  customers.reserve(instance.customer_cnt() + 2);
  customers.emplace_back(instance.depot_id());
  for (auto id : solution.routes()) {
    if (instance.is_customer(id)) {
      customers.emplace_back(id);
    }
  }
  customers.emplace_back(instance.depot_id());
  return cye::Solution(solution.instance_ptr(), std::move(customers));
}

auto reorder_solution(cye::Solution &solution) -> cye::Solution {
  auto new_solution = get_customers_with_endpoints(solution);
  cye::patch_cargo_trivially(new_solution);
  cye::patch_energy_trivially(new_solution);
  return new_solution;
}

auto reorder_solution_optimally(cye::Solution &solution) -> cye::Solution {
  auto new_solution = get_customers_with_endpoints(solution);
  cye::patch_cargo_optimally(new_solution, new_solution.instance().charging_station_cnt() + 1U);
  cye::patch_energy_trivially(new_solution);
  return new_solution;
}

template <typename Callback>
void find_best_insertion(cye::Solution &copy, size_t unassigned_id, Callback callback) {
  for (size_t j = 1; j < copy.routes().size(); ++j) {
    auto new_copy = copy;
    auto patch = cye::Patch<size_t>{};
    patch.add_change(j, unassigned_id);
    new_copy.routes().add_patch(std::move(patch));
    auto new_solution = reorder_solution(new_copy);
    callback(std::move(new_solution), j);
  }
}
}  // namespace

cye::Solution cye::greedy_repair(Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto unassigned_ids = copy.unassigned_customers();
  std::shuffle(unassigned_ids.begin(), unassigned_ids.end(), gen);

  for (auto unassigned_id : unassigned_ids) {
    auto best_cost = std::numeric_limits<double>::max();
    auto best_solution = cye::Solution(nullptr, std::vector<size_t>{});

    auto update_cost = [&](cye::Solution &&new_solution, size_t /**/) {
      auto cost = new_solution.get_cost();
      if (cost < best_cost) {
        best_cost = cost;
        best_solution = std::move(new_solution);
      }
    };

    find_best_insertion(copy, unassigned_id, update_cost);
    if (best_solution.routes().size()) {
      copy = std::move(best_solution);
    }
  }

  copy.clear_unassigned_customers();
  return copy;
}

cye::Solution cye::greedy_repair_best_first(cye::Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto unassigned_ids = copy.unassigned_customers();

  while (!unassigned_ids.empty()) {
    auto best_cost = std::numeric_limits<double>::max();
    int best_unassigned_id = -1;
    auto best_solution = cye::Solution(nullptr, std::vector<size_t>{});
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      auto unassigned_id = unassigned_ids[i];

      auto update_cost = [&](cye::Solution &&new_solution, size_t /**/) {
        auto cost = new_solution.get_cost();
        if (cost < best_cost) {
          best_cost = cost;
          best_solution = std::move(new_solution);
          best_unassigned_id = i;
        }
      };
      find_best_insertion(copy, unassigned_id, update_cost);
    }
    unassigned_ids.erase(unassigned_ids.begin() + best_unassigned_id);
    if (best_unassigned_id != -1) {
      copy = std::move(best_solution);
    }
  }
  copy.clear_unassigned_customers();
  return reorder_solution_optimally(copy);
}

cye::Solution cye::regret_repair(cye::Solution &&solution, alns::RandomEngine &gen) {
  std::uniform_int_distribution dist(2, 3);
  size_t const k_regret = dist(gen);
  auto copy = std::move(solution);
  auto original = copy;
  auto unassigned_ids = copy.unassigned_customers();

  while (!unassigned_ids.empty()) {
    std::vector<std::vector<Insertion>> insertions;
    insertions.resize(unassigned_ids.size());
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      auto unassigned_id = unassigned_ids[i];

      auto update_cost = [&](cye::Solution &&new_solution, size_t position) {
        auto cost = new_solution.get_cost();
        insertions[i].push_back({position, unassigned_id, cost});
      };
      find_best_insertion(copy, unassigned_id, update_cost);
    }
    for (auto &insertions_vec : insertions) {
      std::sort(insertions_vec.begin(), insertions_vec.end());
    }
    Insertion best_insertion{};
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      if (insertions[i].size() >= k_regret) {
        Insertion new_insert = Insertion{insertions[i][0].route_id, insertions[i][0].customer_id,
                                         insertions[i][0].cost - insertions[i][k_regret - 1].cost};
        best_insertion = std::min(best_insertion, new_insert);
      } else if (insertions[i].size() > 0) {
        best_insertion = std::min(best_insertion, insertions[i][0]);
      }
    }

    if (best_insertion.route_id == 0) return original;
    // reconstruct the solution
    auto patch = cye::Patch<size_t>{};
    patch.add_change(best_insertion.route_id, best_insertion.customer_id);
    copy.routes().add_patch(std::move(patch));
    copy = reorder_solution(copy);
    unassigned_ids.erase(std::find(unassigned_ids.begin(), unassigned_ids.end(), best_insertion.customer_id));
  }
  copy.clear_unassigned_customers();
  return reorder_solution_optimally(copy);
}
