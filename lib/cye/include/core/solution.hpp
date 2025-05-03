#pragma once

#include <memory>
#include <vector>
#include "instance.hpp"

namespace cye {

// TODO: this is only temporary

class Solution {
 public:
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
           std::vector<size_t> &&unassigned_customers);

  Solution(Solution const &other) {
    instance_ = other.instance_;
    routes_ = other.routes_;
    unassigned_customers_ = other.unassigned_customers_;
  }
  Solution(Solution &&other) {
    instance_ = std::move(other.instance_);
    routes_ = std::move(other.routes_);
    unassigned_customers_ = std::move(other.unassigned_customers_);
  }

  Solution& operator=(const Solution& other) {
    if (this != &other) {
        instance_ = other.instance_;
        routes_ = other.routes_;
        unassigned_customers_ = other.unassigned_customers_;
    }
    return *this;
  }

  Solution& operator=(Solution&& other) noexcept {
    if (this != &other) {
        instance_ = std::move(other.instance_);
        routes_ = std::move(other.routes_);
        unassigned_customers_ = std::move(other.unassigned_customers_);
    }
    return *this;
}

  [[nodiscard]] auto depot_id() const { return instance_->depot_id(); }
  [[nodiscard]] auto customer_cnt() const { return instance_->customer_cnt(); }
  [[nodiscard]] auto energy_capacity() const { return instance_->energy_capacity(); }
  [[nodiscard]] auto max_cargo_capacity() const { return instance_->max_cargo_capacity(); }

  [[nodiscard]] auto instance() const { return instance_; }
  [[nodiscard]] auto &routes() const { return routes_; }
  [[nodiscard]] auto &unassigned_customers() const { return unassigned_customers_; }

  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto get_cost() const -> double;
  [[nodiscard]] auto total_node_cnt() const { return routes_.size(); }

  auto clear_unassigned_customers() -> void { unassigned_customers_.clear(); }
  auto insert_customer(size_t i, size_t customer_id) -> void;
  auto remove_customer(size_t i) -> void;
  
  auto find_charging_station(size_t node1_id, size_t node2_id, float remaining_battery)
    -> std::optional<size_t>;
  auto reorder_charging_station(size_t i) -> bool;


 private:
  std::shared_ptr<Instance> instance_;
  std::vector<size_t> routes_;
  std::vector<size_t> unassigned_customers_;
};

}  // namespace cye