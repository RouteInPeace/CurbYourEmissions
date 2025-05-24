#include "cye/destroy.hpp"

#include <random>
#include "cye/solution.hpp"

namespace cye {

Solution random_destroy(Solution &&solution, meta::RandomEngine &gen, double max_destroy_rate) {
  auto &instance = solution.instance();

  auto dist = std::uniform_real_distribution(0.0, 1.0);
  auto destroy_rate_dist = std::uniform_real_distribution(0.0, max_destroy_rate);
  auto destroy_rate = destroy_rate_dist(gen);

  std::vector<size_t> new_routes;
  std::vector<size_t> unassigned;
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id)) {
      if (dist(gen) < destroy_rate) {
        unassigned.push_back(node_id);
      } else {
        new_routes.push_back(node_id);
      }
    }
  }

  return cye::Solution(solution.instance_ptr(), std::move(new_routes), std::move(unassigned));
}

Solution random_range_destroy(Solution &&solution, meta::RandomEngine &gen, double max_destroy_rate) {
  auto &instance = solution.instance();

  auto max_range_len = static_cast<size_t>(max_destroy_rate * static_cast<double>(solution.visited_node_cnt()));
  auto start_dist = std::uniform_int_distribution(0UZ, solution.visited_node_cnt());
  auto start = start_dist(gen);
  auto end_dist = std::uniform_int_distribution(start, std::min(solution.visited_node_cnt(), start + max_range_len));
  auto end = end_dist(gen);

  std::vector<size_t> new_routes;
  std::vector<size_t> unassigned;
  size_t i = 0;
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id)) {
      if (i >= start && i < end) {
        unassigned.push_back(node_id);
      } else {
        new_routes.push_back(node_id);
      }
    }
    ++i;
  }

  return cye::Solution(solution.instance_ptr(), std::move(new_routes), std::move(unassigned));
}

}  // namespace cye