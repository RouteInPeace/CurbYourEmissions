#include "cye/instance.hpp"
#include <cmath>

auto cye::Instance::distance(size_t node1_id, size_t node2_id) const -> float {
  auto delta_x = nodes_[node1_id].x - nodes_[node2_id].x;
  auto delta_y = nodes_[node1_id].y - nodes_[node2_id].y;

  return std::sqrt(delta_x * delta_x + delta_y * delta_y);
}
