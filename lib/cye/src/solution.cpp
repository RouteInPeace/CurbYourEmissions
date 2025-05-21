#include "cye/solution.hpp"
#include <cassert>
#include <cstddef>
#include <unordered_set>
#include <vector>

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes)
    : instance_(instance), routes_(std::move(routes)) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
                        std::vector<size_t> &&unassigned_customers)
    : instance_(instance), routes_(std::move(routes)), unassigned_customers_(std::move(unassigned_customers)) {}

auto cye::Solution::is_cargo_valid() const -> bool {
  auto cargo = instance_->cargo_capacity();

  for (auto i = 1UZ; i < routes_.size(); i++) {
    auto &node = instance_->node(routes_[i]);
    switch (node.type) {
      case NodeType::Depot:
        cargo = instance_->cargo_capacity();
        break;
      case NodeType::Customer:
        cargo -= node.demand;
        if (cargo < 0) {
          return false;
        }
        break;
      case NodeType::ChargingStation:
        break;
    }
  }

  return true;
}

auto cye::Solution::is_energy_and_cargo_valid() const -> bool {
  auto energy = instance_->battery_capacity();
  auto cargo = instance_->cargo_capacity();

  for (auto i = 1UZ; i < routes_.size(); i++) {
    energy -= instance_->energy_required(routes_[i - 1], routes_[i]);
    if (energy < 0) {
      return false;
    }

    auto &node = instance_->node(routes_[i]);
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
  }

  return true;
}

auto cye::Solution::is_valid() const -> bool {
  if (routes_[0] != instance_->depot_id() || routes_.back() != instance_->depot_id()) return false;

  auto is_customer = [this](size_t ind) { return instance_->node(ind).type == NodeType::Customer; };
  size_t customer_cnt = std::ranges::count_if(routes_, is_customer);
  auto customers_on_route = std::ranges::to<std::unordered_set<size_t>>(routes_ | std::views::filter(is_customer));

  if (instance_->customer_cnt() != customer_cnt || customers_on_route.size() != customer_cnt) return false;

  return is_energy_and_cargo_valid();
}

auto cye::Solution::get_cost() const -> double {
  auto cost = 0.0;
  for (size_t i = 1UZ; i < routes_.size(); i++) {
    cost += instance_->distance(routes_[i - 1], routes_[i]);
  }
  return cost;
}

auto cye::Solution::insert_customer(size_t i, size_t customer_id) -> void {
  routes_.insert(routes_.begin() + i, customer_id);
}

auto cye::Solution::remove_customer(size_t i) -> void { routes_.erase(routes_.begin() + i); }

auto cye::Solution::find_charging_station(size_t node1_id, size_t node2_id, float remaining_battery)
    -> std::optional<size_t> {
  auto best_station_id = std::optional<size_t>{};
  auto min_distance = std::numeric_limits<float>::infinity();

  if (node1_id != instance_->depot_id() && node2_id != instance_->depot_id() &&
      remaining_battery >= instance_->energy_required(node1_id, instance_->depot_id())) {
    min_distance =
        instance_->distance(node1_id, instance_->depot_id()) + instance_->distance(instance_->depot_id(), node2_id);
    best_station_id = instance_->depot_id();
  }

  for (const auto station_id : instance_->charging_station_ids()) {
    if (station_id == node1_id || station_id == node2_id) continue;
    if (remaining_battery < instance_->energy_required(node1_id, station_id)) continue;

    auto distance = instance_->distance(node1_id, station_id) + instance_->distance(station_id, node2_id);
    if (distance < min_distance) {
      min_distance = distance;
      best_station_id = station_id;
    }
  }

  return best_station_id;
}

auto cye::Solution::reorder_charging_station(size_t pos) -> bool {
  size_t route_start_id = 0;

  size_t i = pos;
  while (i >= 0) {
    if (routes_[i] == instance_->depot_id()) {
      route_start_id = i;
      break;
    }
    --i;
  }

  // remove charging stations from the route
  size_t end_position = 0;
  std::vector<size_t> customers;
  for (int i = route_start_id; i < routes_.size(); ++i) {
    if (routes_[i] == instance_->depot_id() && i != route_start_id) {
      end_position = i;
      break;
    }
    if (instance_->node(routes_[i]).type == NodeType::ChargingStation) continue;
    customers.push_back(routes_[i]);
  }

  auto energy = instance_->battery_capacity();
  for (i = 1; i < customers.size(); i++) {
    if (energy < instance_->energy_required(customers[i - 1], customers[i])) {
      auto charging_station_id = find_charging_station(customers[i - 1], customers[i], energy);
      while (!charging_station_id.has_value()) {
        i -= 1;
        if (i == 0) {
          return false;
        }
        energy += instance_->energy_required(customers[i - 1], customers[i]);
        charging_station_id = find_charging_station(customers[i - 1], customers[i], energy);
      }

      auto it = customers.begin() + i;
      customers.insert(it, *charging_station_id);
      energy = instance_->battery_capacity();
    } else {
      if (customers[i] == instance_->depot_id()) {
        energy = instance_->battery_capacity();
      } else {
        energy -= instance_->energy_required(customers[i - 1], customers[i]);
      }
    }
  }
  // erase is [begin, end>
  routes_.erase(routes_.begin() + route_start_id, routes_.begin() + end_position);
  routes_.insert(routes_.begin() + route_start_id, customers.begin(), customers.end());

  // TODO: check cargo elsewhere
  return is_energy_and_cargo_valid();
}

auto cye::Solution::get_customers() const -> std::vector<size_t> {
  return routes_ | std::views::filter([this](auto ind) { return instance_->is_customer(ind); }) |
         std::ranges::to<std::vector<size_t>>();
}

auto cye::Solution::get_customers_with_endpoints() -> std::vector<size_t> {
  const auto &customers = get_customers();
  std::vector<size_t> result;
  result.reserve(customers.size() + 2);

  result.emplace_back(instance_->depot_id());
  result.insert(result.end(), customers.begin(), customers.end());
  result.emplace_back(instance_->depot_id());

  return result;
}