#pragma once

#include <memory>
#include <vector>
#include "cye/patchable_vector.hpp"
#include "instance.hpp"

namespace cye {

class Solution {
 public:
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, PatchableVector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
           std::vector<size_t> &&unassigned_customers);

  inline auto add_patch(Patch<size_t> &&patch) { routes_.add_patch(std::move(patch)); }

  [[nodiscard]] inline auto &instance() const { return *instance_; }
  [[nodiscard]] inline auto instance_ptr() const { return instance_; }

  [[nodiscard]] inline auto &routes() const { return routes_; }
  [[nodiscard]] inline auto &routes() { return routes_; }
  [[nodiscard]] inline auto &unassigned_customers() { return unassigned_customers_; }
  [[nodiscard]] inline auto visited_node_cnt() const { return routes_.size(); }

  [[nodiscard]] auto is_cargo_valid() const -> bool;
  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto get_cost() const -> double;

  inline auto clear_unassigned_customers() -> void { unassigned_customers_.clear(); }

 private:
  std::shared_ptr<Instance> instance_;
  PatchableVector<size_t> routes_;
  std::vector<size_t> unassigned_customers_;
};

}  // namespace cye