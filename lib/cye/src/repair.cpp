#include "cye/repair.hpp"
#include <cstddef>

namespace cye {

namespace {

template <typename Callback>
void FindBestInsertion(Solution &copy, size_t unassigned_id, Callback callback) {
  for (size_t j = 1; j < copy.routes().size(); ++j) {
    auto new_copy = copy;
    new_copy.insert_customer(j, unassigned_id);
    if (new_copy.is_energy_and_cargo_valid()) {
      callback(std::move(new_copy));
      continue;
    }
    if (new_copy.reorder_charging_station(j)) {
      callback(std::move(new_copy));
    }
  }
}

struct Insertion {
    size_t route_id;
    size_t customer_id;
    double cost = std::numeric_limits<double>::max();

    friend bool operator<(Insertion const &x, Insertion const &y) {
      return x.cost < y.cost;
    }
};

Solution InsertCustomer(Solution &&copy, Insertion const &insertion) {
  copy.insert_customer(insertion.route_id, insertion.customer_id);
  if (copy.is_energy_and_cargo_valid()) {
    return copy;
  }
  if (copy.reorder_charging_station(insertion.route_id)) {
    return copy;
  }
  assert(false);
}
}  // namespace


Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto const &unassigned_ids = copy.unassigned_customers();

  for (auto unassigned_id : unassigned_ids) {
    auto best_cost = std::numeric_limits<double>::max();
    // charging stations can get reordered
    auto best_solution = cye::Solution(nullptr, {});

    auto update_cost = [&](cye::Solution &&new_solution) {
      auto cost = new_solution.get_cost();
      if (cost < best_cost) {
        best_cost = cost;
        best_solution = std::move(new_solution);
      }
    };
    FindBestInsertion(copy, unassigned_id, update_cost);
    if (best_solution.routes().size()) {
      copy = best_solution;
    }
  }

  copy.clear_unassigned_customers();
  return copy;
}

Solution greedy_repair_best_first(Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto unassigned_ids = copy.unassigned_customers();

  while (unassigned_ids.size()) {
    auto best_cost = std::numeric_limits<double>::max();
    int best_unassigned_id = -1;
    auto best_solution = cye::Solution(nullptr, {});
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      auto unassigned_id = unassigned_ids[i];

      auto update_cost = [&](cye::Solution &&new_solution) {
        auto cost = new_solution.get_cost();
        if (cost < best_cost) {
          best_cost = cost;
          best_solution = std::move(new_solution);
          best_unassigned_id = i;
        }
      };
      FindBestInsertion(copy, unassigned_id, update_cost);
    }
    unassigned_ids.erase(unassigned_ids.begin() + best_unassigned_id);
    if (best_unassigned_id != -1) {
      copy = best_solution;
    }
  }
  copy.clear_unassigned_customers();
  return copy;
}

Solution regret_repair(Solution &&solution, alns::RandomEngine &gen) {
  auto copy = solution;
  auto unassigned_ids = copy.unassigned_customers();

  while (unassigned_ids.size()) {
    std::vector<std::vector<Insertion>> insertions;
    insertions.resize(unassigned_ids.size());
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      auto unassigned_id = unassigned_ids[i];

      auto update_cost = [&](cye::Solution &&new_solution) {
        auto cost = new_solution.get_cost();
        insertions[i].push_back({i, unassigned_id, cost});
      };
      FindBestInsertion(copy, unassigned_id, update_cost);
    }
    for (auto &insertions_vec : insertions) {
      std::sort(insertions_vec.begin(), insertions_vec.end());
    }
    Insertion best_insertion{};
    for (size_t i = 0; i < unassigned_ids.size(); ++i) {
      if (insertions[i].size() >= 2) {
        Insertion new_insert = Insertion{insertions[i][0].route_id, insertions[i][0].customer_id,
                                      insertions[i][0].cost - insertions[i][1].cost};
        best_insertion = std::min(best_insertion, new_insert);
      } else if (insertions[i].size() == 1) {
        best_insertion = std::min(best_insertion, insertions[i][0]);
      }
    }
    InsertCustomer(std::move(copy), best_insertion);
    unassigned_ids.erase(std::find(unassigned_ids.begin(), unassigned_ids.end(), best_insertion.customer_id));
  }
  copy.clear_unassigned_customers();
  return copy;
}

}  // namespace cye