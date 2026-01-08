#pragma once

#include <utility>
#include <vector>
#include "cye/solution.hpp"

namespace cye {

struct SolutionCandidate {
  double distance;
  double battery_used;
  double cargo_used;

  auto dominates(SolutionCandidate const &other) const -> bool;
  inline auto is_dominated_by(SolutionCandidate const &other) const { return other.dominates(*this); }
};


class ParetoFront {
 public:
  template <typename... Args>
  auto emplace(Args &&...args) -> void {
    front_.emplace_back(std::forward<Args>(args)...);
  }

  inline auto clear() -> void { front_.clear(); }
  [[nodiscard]] inline auto size() const { return front_.size(); }

  auto colapse() -> void;

  auto begin() { return front_.begin(); }
  auto end() { return front_.end(); }

 private:
  std::vector<SolutionCandidate> front_;
};


class OptimalRepair {
 public:
  auto patch(Solution &solution) -> void;

 private:
};

}  // namespace cye