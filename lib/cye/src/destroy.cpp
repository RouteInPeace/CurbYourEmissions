#include "cye/destroy.hpp"

#include <set>

namespace cye {

Solution random_destroy(Solution &&solution, alns::RandomEngine &gen) {
  auto &instance = solution.instance();

  std::uniform_int_distribution<int> dist(0, instance.customer_cnt() - 1);
  int n_to_remove = dist(gen);
  std::set<size_t> removed_ids;

  auto customer_ids = std::ranges::to<std::vector<size_t>>(instance.customer_ids());
  std::vector<size_t> unassigned;
  std::sample(customer_ids.begin(), customer_ids.end(), std::back_inserter(unassigned), n_to_remove, gen);

  removed_ids.insert(unassigned.begin(), unassigned.end());

  auto &routes = solution.routes();
  std::vector<size_t> new_routes;
  for (auto id : routes) {
    if (removed_ids.find(id) == removed_ids.end()) {
      new_routes.push_back(id);
    }
  }
  std::shuffle(unassigned.begin(), unassigned.end(), gen);
  auto destroyed_solution =
      cye::Solution(std::make_shared<Instance>(instance), std::move(new_routes), std::move(unassigned));
  return destroyed_solution;
}

}  // namespace cye