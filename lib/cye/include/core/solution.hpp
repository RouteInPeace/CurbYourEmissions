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

  Solution(Solution const &other) = default;

  [[nodiscard]] auto unassigned_customers() const { return unassigned_customers_; }
  [[nodiscard]] auto depot_id() const { return instance_->depot_id(); }
  [[nodiscard]] auto customer_cnt() const { return instance_->customer_cnt(); }
  [[nodiscard]] auto energy_capacity() const { return instance_->energy_capacity(); }
  [[nodiscard]] auto max_cargo_capacity() const { return instance_->max_cargo_capacity(); }

  [[nodiscard]] auto instance() const { return instance_; }
  [[nodiscard]] auto &routes() const { return routes_; }
  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto get_cost() const -> double;
  [[nodiscard]] auto total_node_cnt() const { return routes_.size(); }

 private:
  std::shared_ptr<Instance> instance_;
  std::vector<size_t> routes_;
  std::vector<size_t> unassigned_customers_;
};

}  // namespace cye