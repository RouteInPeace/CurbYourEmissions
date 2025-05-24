#pragma once

#include "meta/common.hpp"

namespace meta::alns {

class AcceptanceCriterion {
 public:
  AcceptanceCriterion() = default;

  AcceptanceCriterion(AcceptanceCriterion const &) = delete;
  auto operator=(AcceptanceCriterion const &) -> AcceptanceCriterion & = delete;

  AcceptanceCriterion(AcceptanceCriterion &&) = default;
  auto operator=(AcceptanceCriterion &&) -> AcceptanceCriterion & = default;

  virtual ~AcceptanceCriterion() = default;

  virtual auto accept(double current, double previous, double best, RandomEngine &gen) -> bool = 0;
};

class HillClimbingCriterion : public AcceptanceCriterion {
 public:
  [[nodiscard]] auto accept(double current, double previous, double best, RandomEngine &gen) -> bool override;
};

}  // namespace alns
