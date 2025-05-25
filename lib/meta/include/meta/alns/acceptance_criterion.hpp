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

// starting is usually 5%-10%
// final is usually 0.5%-2%
class ThresholdAcceptance : public AcceptanceCriterion {
 public:
  explicit ThresholdAcceptance(double starting_threshold, double final_threshold, size_t num_iter) : threshold_(starting_threshold) {
    decay_ = (starting_threshold - final_threshold) / num_iter;
  }
  [[nodiscard]] auto accept(double current, double previous, double best, RandomEngine &gen) -> bool override;

 private:
  double threshold_;
  double decay_;
};

class RecordToRecordTravel : public AcceptanceCriterion {
 public:
  explicit RecordToRecordTravel(double starting_threshold, double final_threshold, size_t num_iter) : threshold_(starting_threshold) {
    decay_ = (starting_threshold - final_threshold) / num_iter;
  }
  [[nodiscard]] auto accept(double current, double previous, double best, RandomEngine &gen) -> bool override;

 private:
  double threshold_;
  double decay_;
};

class SimulatedAnnealing : public AcceptanceCriterion {
 public:
  SimulatedAnnealing(double initial_temp, double cooling_rate)
      : temperature_(initial_temp), cooling_rate_(cooling_rate) {}
  [[nodiscard]] auto accept(double current, double previous, double best, RandomEngine &gen) -> bool override;
  void cool() { temperature_ *= cooling_rate_; }  // Call this after each iteration

 private:
  double temperature_;
  double cooling_rate_;
};

}  // namespace alns
