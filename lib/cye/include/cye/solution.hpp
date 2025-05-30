#pragma once

#include <memory>
#include <vector>
#include "cye/patchable_vector.hpp"
#include "instance.hpp"
#include "serial/archive.hpp"

namespace cye {

class Solution {
 public:
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, PatchableVector<size_t> &&routes);
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
           std::vector<size_t> &&unassigned_customers);

  inline auto add_patch(Patch<size_t> &&patch) { routes_.add_patch(std::move(patch)); }

  inline auto squash() { routes_.squash(); }

  inline auto clear_patches() { routes_.clear_patches(); }

  inline auto pop_patch() { return routes_.pop_patch(); }
  [[nodiscard]] inline auto &get_patch(size_t ind) const { return routes_.get_patch(ind); }

  [[nodiscard]] inline auto &base() { return routes_.base(); }
  [[nodiscard]] inline auto &base() const { return routes_.base(); }

  [[nodiscard]] inline auto &instance() const { return *instance_; }
  [[nodiscard]] inline auto instance_ptr() const { return instance_; }

  [[nodiscard]] inline auto &routes() const { return routes_; }
  [[nodiscard]] inline auto visited_node_cnt() const { return routes_.size(); }

  [[nodiscard]] auto is_cargo_valid() const -> bool;
  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto cost() const -> float;

  template <serial::Value V>
  auto write(V v) const -> void {
    v.emplace("instanceName", instance_->name());
    v.emplace("routes", routes_);
  }

 private:
  std::shared_ptr<Instance> instance_;
  PatchableVector<size_t> routes_;
};

}  // namespace cye