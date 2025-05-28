#include "cye/solution.hpp"
#include <cassert>
#include <cstddef>
#include <unordered_set>
#include <vector>

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes)
    : instance_(instance), routes_(std::move(routes)) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, PatchableVector<size_t> &&routes)
    : instance_(instance), routes_(routes) {}

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
                        std::vector<size_t> &&unassigned_customers)
    : instance_(instance), routes_(std::move(routes)), unassigned_customers_(std::move(unassigned_customers)) {}

auto cye::Solution::is_cargo_valid() const -> bool {
  auto cargo = instance_->cargo_capacity();

  for (auto node_id : routes_) {
    const auto &node = instance_->node(node_id);
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

  auto previous_node_id = *routes_.begin();
  for (auto it = ++routes_.begin(); it != routes_.end(); ++it) {
    auto current_node_id = *it;
    energy -= instance_->energy_required(previous_node_id, current_node_id);
    if (energy < 0) {
      return false;
    }

    auto &node = instance_->node(current_node_id);
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
    previous_node_id = current_node_id;
  }

  return true;
}

auto cye::Solution::is_valid() const -> bool {
  if (*routes_.begin() != instance_->depot_id() || *(routes_.rbegin()) != instance_->depot_id()) return false;

  auto is_customer = [this](size_t ind) { return instance_->node(ind).type == NodeType::Customer; };
  size_t customer_cnt = std::ranges::count_if(routes_, is_customer);
  auto customers_on_route = std::ranges::to<std::unordered_set<size_t>>(routes_ | std::views::filter(is_customer));

  if (instance_->customer_cnt() != customer_cnt || customers_on_route.size() != customer_cnt) return false;

  return is_energy_and_cargo_valid();
}

auto cye::Solution::cost() const -> float {
  auto cost = 0.f;
  auto previous_node_id = *routes_.begin();
  for (auto it = ++routes_.begin(); it != routes_.end(); ++it) {
    auto current_node_id = *it;
    cost += instance_->distance(previous_node_id, current_node_id);
    previous_node_id = current_node_id;
  }
  return cost;
}
