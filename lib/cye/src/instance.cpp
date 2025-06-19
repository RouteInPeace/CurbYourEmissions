#include "cye/instance.hpp"
#include <cmath>
#include <cstddef>
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

auto cye::Instance::update_non_dominated_cs_() -> void {
  non_dominated_cs_.resize(nodes_.size(), std::vector<std::vector<size_t>>(nodes_.size(), std::vector<size_t>{depot_id()}));

  auto charging_station_ids = this->charging_station_ids();
  for (auto node_id1 = 0UZ; node_id1 < nodes_.size(); ++node_id1) {
    for (auto node_id2 = 0UZ; node_id2 < nodes_.size(); ++node_id2) {
      if (node_id1 == node_id2) continue;
      if (!is_charging_station(node_id1) || !is_charging_station(node_id2)) continue;

      for (auto cs_id : charging_station_ids) {
        auto dist1 = distance(node_id1, cs_id);
        auto dist2 = distance(node_id2, cs_id);

        bool dominated = false;
        for (auto non_dominated : non_dominated_cs_[node_id1][node_id2]) {
          auto old_dist1 = distance(node_id1, non_dominated);
          auto old_dist2 = distance(node_id2, non_dominated);
          if (dist1 >= old_dist1 && dist2 >= old_dist2) {
            dominated = true;
            break;
          }
        }
        if (dominated) continue;

        auto new_non_dominated = std::vector<size_t>{cs_id};
        for (auto non_dominated : non_dominated_cs_[node_id1][node_id2]) {
          auto old_dist1 = distance(node_id1, non_dominated);
          auto old_dist2 = distance(node_id2, non_dominated);
          if (dist1 > old_dist1 || dist2 > old_dist2) {
            new_non_dominated.emplace_back(non_dominated);
          }
        }
        non_dominated_cs_[node_id1][node_id2] = std::move(new_non_dominated);
      }
    }
  }
}