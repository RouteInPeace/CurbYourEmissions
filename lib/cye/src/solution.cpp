#include "cye/solution.hpp"
#include <cassert>
#include <cstddef>
#include <unordered_set>

// cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes)
//     : instance_(instance), routes_(std::move(routes)) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&customers,
                        std::vector<size_t> &&unassigned_customers)
    : instance_(instance), customers_(std::move(customers)), unassigned_customers_(std::move(unassigned_customers)), stations_valid_(false), depots_valid_(false) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> const &routes)
    : instance_(instance) {
  size_t customer_depot_id = 0;
  size_t customer_id = 0;
  for (auto id : routes) {
    if (instance_->node(id).type == NodeType::ChargingStation) {
      stations_.push_back({id, customer_depot_id});
    } else if (instance_->node(id).type == NodeType::Depot) {
      depots_.push_back(customer_id);
      ++customer_depot_id;
    } else {
      customers_.push_back(id);
      ++customer_depot_id;
      ++customer_id;
    }
  }
}

auto cye::Solution::is_energy_and_cargo_valid() const -> bool {
  auto energy = instance_->energy_capacity();
  auto cargo = instance_->max_cargo_capacity();

  size_t previous_node_id = 0; // depot
  for (auto it = ++begin(); it != end(); ++it) {
    auto node_id = *it;
    energy -= instance_->energy_required(previous_node_id, node_id);
    if (energy < 0) {
      return false;
    }

    auto &node = instance_->node(node_id);
    switch (node.type) {
      case NodeType::Depot:
        energy = instance_->energy_capacity();
        cargo = instance_->max_cargo_capacity();
        break;
      case NodeType::Customer:
        cargo -= node.demand;
        if (cargo < 0) {
          return false;
        }
        break;
      case NodeType::ChargingStation:
        energy = instance_->energy_capacity();
        break;
    }
  }

  return true;
}

auto cye::Solution::is_valid() const -> bool {
  if (depots_[0] != instance_->depot_id() || depots_.back() != customers_.size()) return false;

  size_t customer_cnt = customers_.size();
  if (instance_->customer_cnt() != customer_cnt) return false;

  return is_energy_and_cargo_valid();
}

auto cye::Solution::get_cost() const -> double {
  auto cost = 0.0;
  auto previous_node_id = instance_->depot_id();
  for (auto it = begin(); it != end(); ++it) {
    auto node_id = *it;
    cost += instance_->distance(previous_node_id, node_id);
    previous_node_id = node_id;
  }
  return cost;
}

auto cye::Solution::swap_customer(size_t customer_id1, size_t customer_id2) -> void {
  std::swap(customers_[customer_id1], customers_[customer_id2]);
  depots_valid_ = false;
  stations_valid_ = false;
}

auto cye::Solution::insert_charging_station(size_t position, size_t station_id) -> void {
  stations_.insert(stations_.begin() + position, {station_id, position});

  depots_valid_ = false;
  stations_valid_ = false;
}

  auto cye::Solution::insert_depot(size_t position) -> void {
    depots_.insert(depots_.begin() + position, 0);
    depots_valid_ = false;
    stations_valid_ = false;
  }

  auto cye::Solution::clear_charging_stations() -> void {
    stations_.clear();
  }
  auto cye::Solution::clear_depots() -> void {
    depots_.clear();
  }

auto cye::Solution::insert_customer(size_t i, size_t customer_id) -> void {
  customers_.insert(customers_.begin() + i, customer_id);
}

// auto cye::Solution::remove_customer(size_t i) -> void { routes_.erase(routes_.begin() + i); }

// auto cye::Solution::find_charging_station(size_t node1_id, size_t node2_id, float remaining_battery)
//     -> std::optional<size_t> {
//   auto best_station_id = std::optional<size_t>{};
//   auto min_distance = std::numeric_limits<float>::infinity();

//   if (node1_id != instance_->depot_id() && node2_id != instance_->depot_id() &&
//       remaining_battery >= instance_->energy_required(node1_id, instance_->depot_id())) {
//     min_distance =
//         instance_->distance(node1_id, instance_->depot_id()) + instance_->distance(instance_->depot_id(), node2_id);
//     best_station_id = instance_->depot_id();
//   }

//   for (const auto station_id : instance_->charging_station_ids()) {
//     if (station_id == node1_id || station_id == node2_id) continue;
//     if (remaining_battery < instance_->energy_required(node1_id, station_id)) continue;

//     auto distance = instance_->distance(node1_id, station_id) + instance_->distance(station_id, node2_id);
//     if (distance < min_distance) {
//       min_distance = distance;
//       best_station_id = station_id;
//     }
//   }

//   return best_station_id;
// }

// auto cye::Solution::reorder_charging_station(size_t pos) -> bool {
//   size_t route_start_id = 0;

//   size_t i = pos;
//   while (i >= 0) {
//     if (routes_[i] == instance_->depot_id()) {
//       route_start_id = i;
//       break;
//     }
//     --i;
//   }

//   // remove charging stations from the route
//   size_t end_position = 0;
//   std::vector<size_t> customers;
//   for (int i = route_start_id; i < routes_.size(); ++i) {
//     if (routes_[i] == instance_->depot_id() && i != route_start_id) {
//       end_position = i;
//       break;
//     }
//     if (instance_->node(routes_[i]).type == NodeType::ChargingStation) continue;
//     customers.push_back(routes_[i]);
//   }

//   auto energy = instance_->energy_capacity();
//   for (i = 1; i < customers.size(); i++) {
//     if (energy < instance_->energy_required(customers[i - 1], customers[i])) {
//       auto charging_station_id = find_charging_station(customers[i - 1], customers[i], energy);
//       while (!charging_station_id.has_value()) {
//         i -= 1;
//         if (i == 0) {
//           return false;
//         }
//         energy += instance_->energy_required(customers[i - 1], customers[i]);
//         charging_station_id = find_charging_station(customers[i - 1], customers[i], energy);
//       }

//       auto it = customers.begin() + i;
//       customers.insert(it, *charging_station_id);
//       energy = instance_->energy_capacity();
//     } else {
//       if (customers[i] == instance_->depot_id()) {
//         energy = instance_->energy_capacity();
//       } else {
//         energy -= instance_->energy_required(customers[i - 1], customers[i]);
//       }
//     }
//   }
//   // erase is [begin, end>
//   routes_.erase(routes_.begin() + route_start_id, routes_.begin() + end_position);
//   routes_.insert(routes_.begin() + route_start_id, customers.begin(), customers.end());

//   // TODO: check cargo elsewhere
//   return is_energy_and_cargo_valid();
// }

