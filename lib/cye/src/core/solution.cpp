#include "core/solution.hpp"
#include <algorithm>
#include <cstddef>
#include <unordered_set>

cye::Solution::Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes)
    : instance_(instance), routes_(std::move(routes)) {}

auto cye::Solution::is_energy_and_cargo_valid() const -> bool {
  auto energy = instance_->energy_capacity();
  auto cargo = instance_->max_cargo_capacity();

  for (auto i = 1UZ; i < routes_.size(); i++) {
    energy -= instance_->energy_required(routes_[i - 1], routes_[i]);
    if (energy < 0) {
      return false;
    }

    auto &node = instance_->node(routes_[i]);
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
  if (routes_[0] != instance_->depot_id() || routes_.back() != instance_->depot_id()) return false;

  auto is_customer = [this](size_t ind) { return instance_->node(ind).type == NodeType::Customer; };
  size_t customer_cnt = std::ranges::count_if(routes_, is_customer);
  auto customers_on_route = std::ranges::to<std::unordered_set<size_t>>(routes_ | std::views::filter(is_customer));

  if (instance_->customer_cnt() != customer_cnt || customers_on_route.size() != customer_cnt) return false;

  return is_energy_and_cargo_valid();
}