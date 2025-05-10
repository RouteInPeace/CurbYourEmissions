#pragma once

#include <memory>
#include <vector>
#include "instance.hpp"

namespace cye {

class Solution {
 public:
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
           std::vector<size_t> &&unassigned_customers);

  [[nodiscard]] auto &instance() const { return *instance_; }
  [[nodiscard]] auto &routes() const { return routes_; }
  [[nodiscard]] auto &unassigned_customers() const { return unassigned_customers_; }
  [[nodiscard]] auto visited_node_cnt() const { return routes_.size(); }

  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto get_cost() const -> double;

  auto clear_unassigned_customers() -> void { unassigned_customers_.clear(); }
  auto insert_customer(size_t i, size_t customer_id) -> void;
  auto remove_customer(size_t i) -> void;

  // TODO: this doe not belong here
  auto find_charging_station(size_t node1_id, size_t node2_id, float remaining_battery) -> std::optional<size_t>;
  auto reorder_charging_station(size_t i) -> bool;

 private:
  std::shared_ptr<Instance> instance_;
  std::vector<size_t> routes_;
  std::vector<size_t> unassigned_customers_;
};

}  // namespace cye