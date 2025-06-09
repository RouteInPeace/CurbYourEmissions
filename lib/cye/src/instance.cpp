#include "cye/instance.hpp"
#include <cmath>
#include <vector>

// auto cye::Instance::distance(size_t node1_id, size_t node2_id) const -> float {
//   if (node1_id == node2_id) return 0.f;
//   if (node1_id > node2_id) std::swap(node1_id, node2_id);

//   auto ind = node2_id * (node2_id + 1) / 2 + node1_id;
//   return distance_cache_[ind];
// }

auto cye::Instance::update_distance_cache_() -> void {
  distance_cache_.resize(nodes_.size() * (nodes_.size() + 1) / 2);

  for (auto node1_id = 0UZ; node1_id < nodes_.size(); ++node1_id) {
    for (auto node2_id = node1_id + 1UZ; node2_id < nodes_.size(); ++node2_id) {
      auto delta_x = nodes_[node1_id].x - nodes_[node2_id].x;
      auto delta_y = nodes_[node1_id].y - nodes_[node2_id].y;
      auto dist = std::sqrt(delta_x * delta_x + delta_y * delta_y);

      distance_cache_[node2_id * (node2_id + 1) / 2 + node1_id] = dist;
    }
  }
}

auto cye::Instance::update_closest_charging_station_() -> void {
  closest_charging_station_.resize(nodes_.size(), depot_id());
  auto charging_station_ids = this->charging_station_ids();

  for (auto node_id = 0UZ; node_id < nodes_.size(); ++node_id) {
    if (is_charging_station(node_id)) {
      closest_charging_station_[node_id] = node_id;
      continue;
    }

    auto min_distance = distance(node_id, depot_id());
    for (const auto cs_id : charging_station_ids) {
      auto dist = distance(node_id, cs_id);
      if (dist < min_distance) {
        min_distance = dist;
        closest_charging_station_[node_id] = cs_id;
      }
    }
  }
}