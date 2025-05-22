#include "cye/destroy.hpp"

#include <random>
#include "cye/solution.hpp"

namespace cye {

Solution random_destroy(Solution &&solution, alns::RandomEngine &gen, double max_destroy_rate) {
  auto &instance = solution.instance();

  auto dist = std::uniform_real_distribution(0.0, 1.0);
  auto destroy_rate_dist = std::uniform_real_distribution(0.0, max_destroy_rate);
  auto destroy_rate = destroy_rate_dist(gen);

  std::vector<size_t> new_routes;
  std::vector<size_t> unassigned;
  new_routes.push_back(instance.depot_id());
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id)) {
      if (dist(gen) < destroy_rate) {
        unassigned.push_back(node_id);
      } else {
        new_routes.push_back(node_id);
      }
    }
  }
  new_routes.push_back(instance.depot_id());

  return cye::Solution(solution.instance_ptr(), std::move(new_routes), std::move(unassigned));
}

}  // namespace cye