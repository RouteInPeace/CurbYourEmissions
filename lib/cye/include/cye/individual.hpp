#pragma once

#include <cassert>
#include "cye/repair.hpp"
#include "cye/solution.hpp"

namespace cye {

class EVRPIndividual {
 public:
  EVRPIndividual(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, cye::Solution &&solution);

  [[nodiscard]] inline auto cost() const {
    assert(valid_);
    return cost_;
  }
  [[nodiscard]] inline auto &genotype() const { return solution_.base(); }
  [[nodiscard]] inline auto &genotype() {
    valid_ = false;
    return solution_.base();
  }

  [[nodiscard]] inline auto &solution() const { return solution_; }
  [[nodiscard]] inline auto &solution() {
    valid_ = false;
    return solution_;
  }
  [[nodiscard]] inline auto is_trivial() const { return trivial_; }
  [[nodiscard]] inline auto hash() const -> size_t {
    assert(valid_);
    return hash_;
  }

  inline auto set_valid() { valid_ = true; }

  inline auto switch_to_optimal() {
    trivial_ = false;
    valid_ = false;
  }
  inline auto switch_to_trivial() {
    trivial_ = true;
    valid_ = false;
  }
  auto update_cost() -> void;

 private:
  static constexpr size_t fnv_prime_ = 1099511628211u;

  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  cye::Solution solution_;
  bool trivial_{true};
  float cost_;
  size_t hash_;
  bool valid_;
};

}  // namespace cye