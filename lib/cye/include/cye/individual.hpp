#pragma once

#include "cye/repair.hpp"
#include "cye/solution.hpp"

namespace cye {

class EVRPIndividual {
 public:
  EVRPIndividual(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, cye::Solution &&solution);

  [[nodiscard]] inline auto fitness() const { return cost_; }
  [[nodiscard]] inline auto genotype() const { return solution_.base(); }
  [[nodiscard]] inline auto genotype() { return solution_.base(); }
  [[nodiscard]] inline auto &solution() const { return solution_; }
  [[nodiscard]] inline auto is_trivial() const { return trivial_; }
  [[nodiscard]] inline auto hash() const -> size_t { return hash_; }

  inline auto switch_to_optimal() { trivial_ = false; }
  inline auto switch_to_trivial() { trivial_ = true; }
  auto update_fitness() -> void;

 private:
  static constexpr size_t fnv_prime_ = 1099511628211u;

  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  cye::Solution solution_;
  bool trivial_{true};
  float cost_;
  size_t hash_;
};

}  // namespace cye