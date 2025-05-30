#pragma once

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>
#include "node.hpp"

namespace cye {

class Instance {
 public:
  template <serial::Value V>
  Instance(V &&value);

  template <serial::Value V>
  auto write(V value) const -> void;

  [[nodiscard]] auto &nodes() const { return nodes_; }
  [[nodiscard]] auto &node(size_t ind) const { return nodes_[ind]; }
  [[nodiscard]] auto node_cnt() const { return nodes_.size(); }
  [[nodiscard]] inline auto customer_ids() const { return std::views::iota(1UZ, customer_cnt_ + 1); }
  [[nodiscard]] inline auto charging_station_ids() const { return std::views::iota(customer_cnt_ + 1, nodes_.size()); }
  [[nodiscard]] constexpr inline auto depot_id() const -> size_t { return 0UZ; }
  [[nodiscard]] inline auto cargo_capacity() const { return cargo_capacity_; }
  [[nodiscard]] inline auto battery_capacity() const { return battery_capacity_; }
  [[nodiscard]] inline auto distance(size_t node1_id, size_t node2_id) const -> float {
    if (node1_id == node2_id) [[unlikely]] {
      return 0.f;
    }
    if (node1_id > node2_id) std::swap(node1_id, node2_id);

    auto ind = node2_id * (node2_id + 1) / 2 + node1_id;
    return distance_cache_[ind];
  }
  [[nodiscard]] inline auto energy_required(size_t node1_id, size_t node2_id) const {
    return energy_consumption_ * distance(node1_id, node2_id);
  }
  [[nodiscard]] inline auto energy_consumption() const { return energy_consumption_; }
  [[nodiscard]] inline auto customer_cnt() const { return customer_cnt_; }
  [[nodiscard]] inline auto max_range() const { return battery_capacity_ / energy_consumption_; }
  [[nodiscard]] inline auto demand(size_t ind) const { return nodes_[ind].demand; }
  [[nodiscard]] inline auto charging_station_cnt() const { return charging_station_cnt_; }
  [[nodiscard]] inline auto is_customer(size_t ind) const { return ind > 0 && ind <= customer_cnt_; }
  [[nodiscard]] inline auto is_charging_station(size_t ind) const { return ind == 0 || ind > customer_cnt_; }
  [[nodiscard]] inline auto &name() { return name_; }

 private:
  auto update_distance_cache_() -> void;

  std::string name_;
  float optimal_value_;
  size_t minimum_route_cnt_;
  float cargo_capacity_;
  float battery_capacity_;
  float energy_consumption_;
  size_t customer_cnt_;
  size_t charging_station_cnt_;

  std::vector<Node> nodes_;
  std::vector<float> distance_cache_;
};

template <serial::Value V>
Instance::Instance(V &&value)
    : name_(value["name"].template get<std::string_view>()),
      optimal_value_(value["optimalValue"].template get<float>()),
      minimum_route_cnt_(value["minimumRouteCnt"].template get<size_t>()),
      cargo_capacity_(value["cargoCapacity"].template get<float>()),
      battery_capacity_(value["batteryCapacity"].template get<float>()),
      energy_consumption_(value["energyConsumption"].template get<float>()),
      customer_cnt_(value["customerCnt"].template get<size_t>()),
      charging_station_cnt_(value["chargingStationCnt"].template get<size_t>()),
      nodes_(value["nodes"].template get<std::vector<Node>>()) {
  std::ranges::stable_sort(nodes_,
                    [](auto &n1, auto &n2) { return static_cast<uint8_t>(n1.type) < static_cast<uint8_t>(n2.type); });
  update_distance_cache_();
}

template <serial::Value V>
auto Instance::write(V v) const -> void {
  v.emplace("name", name_);
  v.emplace("optimalValue", optimal_value_);
  v.emplace("minimumRouteCnt", minimum_route_cnt_);
  v.emplace("customerCnt", customer_cnt_);
  v.emplace("chargingStationCnt", charging_station_cnt_);
  v.emplace("cargoCapacity", cargo_capacity_);
  v.emplace("batteryCapacity", battery_capacity_);
  v.emplace("energyConsumption", energy_consumption_);
  v.emplace("nodes", nodes_);
}

}  // namespace cye