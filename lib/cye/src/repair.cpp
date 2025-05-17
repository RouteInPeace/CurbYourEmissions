#include "cye/repair.hpp"

namespace cye {

// Will be rewriten when new cargo and energy model is implemented

// Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen) {
//   auto copy = solution;
//   auto const &unassigned_ids = copy.unassigned_customers();

//   for (auto unassigned_id : unassigned_ids) {
//     auto best_cost = std::numeric_limits<double>::max();
//     // charging stations can get reordered
//     auto best_solution = cye::Solution(nullptr, {});

//     auto update_cost = [&](cye::Solution &&new_solution) {
//       auto cost = copy.get_cost();
//       if (cost < best_cost) {
//         best_cost = cost;
//         best_solution = std::move(new_solution);
//       }
//     };

//     for (size_t i = 1; i < copy.customers().size(); ++i) {
//       auto new_copy = copy;
//       new_copy.insert_customer(i, unassigned_id);
//       if (new_copy.is_energy_and_cargo_valid()) {
//         update_cost(std::move(new_copy));
//         continue;
//       }
//       if (new_copy.reorder_charging_station(i)) {
//         update_cost(std::move(new_copy));
//       }
//     }
//     if (best_solution.routes().size()) {
//       copy = best_solution;
//     }
//   }

//   copy.clear_unassigned_customers();
//   return copy;
// }

}  // namespace cye
