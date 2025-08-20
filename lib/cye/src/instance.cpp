#include "cye/instance.hpp"
#include <cmath>
#include <vector>


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