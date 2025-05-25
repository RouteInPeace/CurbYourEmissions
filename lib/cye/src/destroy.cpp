#include "cye/destroy.hpp"

#include <cstddef>
#include <random>
#include <unordered_set>
#include "cye/repair.hpp"
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

Solution worst_destroy(Solution &&solution, meta::RandomEngine &gen, double max_destroy_rate) {
  auto &instance = solution.instance();
  solution.clear_patches();

  auto dist = std::uniform_real_distribution(0.0, 1.0);
  auto destroy_rate_dist = std::uniform_real_distribution(0.0, max_destroy_rate);
  auto destroy_rate = destroy_rate_dist(gen);
  size_t destroy_count = static_cast<size_t>(destroy_rate * static_cast<double>(solution.visited_node_cnt()));

  cye::patch_endpoint_depots(solution);
  auto it = solution.routes().begin();
  auto previous_id = *it;

  std::vector<std::pair<size_t, double>> customer_costs;
  for (auto it = ++solution.routes().begin(); it != solution.routes().end(); ++it) {
    auto current_id = *it;
    if (instance.is_customer(current_id)) {
      // if its customers next always exists (depot at end)
      auto next_id = *(std::next(it, 1));
      auto cost = instance.distance(previous_id, current_id) + instance.distance(current_id, next_id);
      customer_costs.emplace_back(current_id, cost);
    }
    previous_id = current_id;
  }
  std::sort(customer_costs.begin(), customer_costs.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  
  std::vector<size_t> unassigned;
  std::unordered_set<size_t> unassigned_set;
  unassigned.reserve(destroy_count);
  
  for (size_t i = 0; i < destroy_count; ++i) {
    unassigned.emplace_back(customer_costs[i].first);
    unassigned_set.insert(customer_costs[i].first);
  }

  std::vector<size_t> new_routes;
  new_routes.reserve(solution.visited_node_cnt() - destroy_count);
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id) && !unassigned_set.contains(node_id)) {
      new_routes.emplace_back(node_id);
    }
  }
  assert(solution.visited_node_cnt() - 2 == new_routes.size() + unassigned.size());
  return cye::Solution(solution.instance_ptr(), std::move(new_routes), std::move(unassigned));
}

Solution shaw_removal(Solution&& solution, meta::RandomEngine& gen, double max_destroy_rate) {
  auto& instance = solution.instance();
  solution.clear_patches();

  auto destroy_rate_dist = std::uniform_real_distribution(0.0, max_destroy_rate);
  double destroy_rate = destroy_rate_dist(gen);
  size_t destroy_count = static_cast<size_t>(destroy_rate * static_cast<double>(solution.unassigned_customers().size() + solution.routes().size()));

  std::vector<size_t> customers;
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id)) {
      customers.push_back(node_id);
    }
  }

  if (customers.empty()) {
    return std::move(solution);
  }

  auto similarity = [&](size_t a, size_t b) -> double {
    double spatial = instance.distance(a, b);
    double demand_diff = std::abs(instance.demand(a) - instance.demand(b));
    return spatial;
  };

  std::unordered_set<size_t> to_remove;
  std::vector<size_t> removal_candidates;

  std::uniform_int_distribution<size_t> customer_index_dist(0, customers.size() - 1);
  size_t seed_customer = customers[customer_index_dist(gen)];
  to_remove.insert(seed_customer);
  removal_candidates.push_back(seed_customer);

  while (to_remove.size() < destroy_count && to_remove.size() < customers.size()) {
    // Find the most similar customer not already chosen
    size_t best_candidate = 0;
    double best_score = std::numeric_limits<double>::max();
    for (auto cand : customers) {
      if (to_remove.contains(cand)) continue;

      for (auto selected : removal_candidates) {
        double score = similarity(cand, selected);
        if (score < best_score) {
          best_score = score;
          best_candidate = cand;
        }
      }
    }
    to_remove.insert(best_candidate);
    removal_candidates.push_back(best_candidate);
  }

  std::vector<size_t> new_routes;
  std::vector<size_t> unassigned;
  for (auto node_id : solution.routes()) {
    if (instance.is_customer(node_id) && to_remove.contains(node_id)) {
      unassigned.push_back(node_id);
    } else {
      new_routes.push_back(node_id);
    }
  }

  return cye::Solution(solution.instance_ptr(), std::move(new_routes), std::move(unassigned));
}

}  // namespace cye