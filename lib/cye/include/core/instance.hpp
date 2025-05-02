#pragma once

#include <filesystem>
#include <ranges>
#include <string>
#include <vector>
#include "node.hpp"

namespace cye {

class Instance {
 public:
  Instance(std::filesystem::path path);

  [[nodiscard]] auto &nodes() const { return nodes_; }
  [[nodiscard]] auto &node(size_t ind) const { return nodes_[ind]; }
  [[nodiscard]] auto node_cnt() const { return nodes_.size(); }
  [[nodiscard]] inline auto customer_ids() const { return std::views::iota(1UZ, customer_cnt_ + 1); }
  [[nodiscard]] inline auto charging_station_ids() const { return std::views::iota(customer_cnt_ + 1, nodes_.size()); }
  [[nodiscard]] constexpr inline auto depot_id() const -> size_t { return 0UZ; }
  [[nodiscard]] inline auto max_cargo_capacity() const { return max_cargo_capacity_; }
  [[nodiscard]] inline auto energy_capacity() const { return energy_capacity_; }
  [[nodiscard]] auto distance(size_t node1_id, size_t node2_id) const -> float;
  [[nodiscard]] inline auto energy_required(size_t node1_id, size_t node2_id) const {
    return energy_consumption_ * distance(node1_id, node2_id);
  }
  [[nodiscard]] inline auto customer_cnt() const { return customer_cnt_; }
  [[nodiscard]] inline auto max_range() const { return energy_capacity_ / energy_consumption_; }

 private:
  std::string name_;
  float optimal_value_;
  size_t minimum_route_cnt_;
  float max_cargo_capacity_;
  float energy_capacity_;
  float energy_consumption_;
  size_t customer_cnt_;
  size_t charging_station_cnt_;

  std::vector<Node> nodes_;
};

}  // namespace cye