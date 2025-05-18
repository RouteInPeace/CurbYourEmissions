#include "cye/solution.hpp"
#include <cassert>
#include <cstddef>
#include <unordered_set>

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&customers,
                        std::vector<size_t> &&unassigned_customers)
    : instance_(instance),
      customers_(std::move(customers)),
      unassigned_customers_(std::move(unassigned_customers)),
      stations_valid_(false),
      depots_valid_(false) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> const &routes) : instance_(instance) {
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
  auto energy = instance_->battery_capacity();
  auto cargo = instance_->cargo_capacity();

  size_t previous_node_id = 0;  // depot
  for (auto it = ++begin(); it != end(); ++it) {
    auto node_id = *it;
    energy -= instance_->energy_required(previous_node_id, node_id);
    if (energy < 0) {
      return false;
    }

    auto &node = instance_->node(node_id);
    switch (node.type) {
      case NodeType::Depot:
        energy = instance_->battery_capacity();
        cargo = instance_->cargo_capacity();
        break;
      case NodeType::Customer:
        cargo -= node.demand;
        if (cargo < 0) {
          return false;
        }
        break;
      case NodeType::ChargingStation:
        energy = instance_->battery_capacity();
        break;
    }
    previous_node_id = node_id;
  }

  return true;
}

auto cye::Solution::is_cargo_valid() const -> bool {
  auto cargo = instance_->cargo_capacity();
  for (auto it = customer_depot_begin(); it != customer_depot_end(); ++it) {
    auto &node = instance_->node(*it);
    if (node.type == NodeType::Customer) {
      cargo -= node.demand;
      if (cargo < 0) {
        return false;
      }
    } else if (node.type == NodeType::Depot) {
      cargo = instance_->cargo_capacity();
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

auto cye::Solution::pop_depot() -> size_t {
  assert(!depots_.empty());
  auto depot_id = depots_.back();
  depots_.pop_back();
  depots_valid_ = false;
  stations_valid_ = false;
  return depot_id;
}

auto cye::Solution::insert_depot(size_t position) -> void {
  depots_.emplace_back(position);
  depots_valid_ = false;
  stations_valid_ = false;
}

auto cye::Solution::clear_charging_stations() -> void { stations_.clear(); }
auto cye::Solution::clear_depots() -> void { depots_.clear(); }

auto cye::Solution::insert_customer(size_t i, size_t customer_id) -> void {
  customers_.insert(customers_.begin() + i, customer_id);
}

