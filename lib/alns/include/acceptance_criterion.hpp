#pragma once

namespace alns {

class AcceptanceCriterion {
 public:
  AcceptanceCriterion() = default;

  AcceptanceCriterion(AcceptanceCriterion const &) = delete;
  auto operator=(AcceptanceCriterion const &) -> AcceptanceCriterion & = delete;

  AcceptanceCriterion(AcceptanceCriterion &&) = default;
  auto operator=(AcceptanceCriterion &&) -> AcceptanceCriterion & = default;

  virtual ~AcceptanceCriterion() = default;

  virtual auto accept(double current, double previous, double best) -> bool = 0;
};

class HillClimbingCriterion : public AcceptanceCriterion {
 public:
  [[nodiscard]] virtual auto accept(double current, double previous, double best) -> bool override;
};

}  // namespace alns