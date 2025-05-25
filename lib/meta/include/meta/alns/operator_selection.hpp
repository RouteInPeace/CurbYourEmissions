#pragma once

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <tuple>

#include "meta/common.hpp"

namespace meta::alns {

class OperatorSelection {
 public:
  OperatorSelection() = default;

  OperatorSelection(OperatorSelection const &) = delete;
  auto operator=(OperatorSelection const &) -> OperatorSelection & = delete;

  OperatorSelection(OperatorSelection &&) = default;
  auto operator=(OperatorSelection &&) -> OperatorSelection & = default;

  virtual ~OperatorSelection() = default;

  virtual auto set_operator_cnt(size_t destroy_operator_cnt, size_t repair_operator_cnt) -> void = 0;
  virtual auto select_operators(RandomEngine &gen) -> std::tuple<size_t, size_t> = 0;
  virtual auto update(double current, double previous, double best) -> void = 0;
  virtual auto print_weights() const -> void {
    std::cout << "OperatorSelection: No weights to print.\n";
  }
};

class RandomOperatorSelection : public OperatorSelection {
 public:
  inline auto set_operator_cnt(size_t destroy_operator_cnt, size_t repair_operator_cnt) -> void override {
    destroy_operator_cnt_ = destroy_operator_cnt;
    repair_operator_cnt_ = repair_operator_cnt;
  }
  inline auto select_operators(RandomEngine &gen) -> std::tuple<size_t, size_t> override {
    std::uniform_int_distribution<size_t> destroy_dist(0, destroy_operator_cnt_ - 1);
    std::uniform_int_distribution<size_t> repair_dist(0, repair_operator_cnt_ - 1);
    return {destroy_dist(gen), repair_dist(gen)};
  };
  inline auto update(double /* current */, double /* previous */, double /* best */) -> void override {};

 private:
  size_t destroy_operator_cnt_ = 0;
  size_t repair_operator_cnt_ = 0;
};

class RouletteWheelOperatorSelection : public OperatorSelection {
 public:
  RouletteWheelOperatorSelection(double decay = 0.2,
                                 double best_score = 5.0,
                                 double improvement_score = 3.0,
                                 double accepted_score = 1.0,
                                 double rejected_score = 0.0)
      : decay_(decay), scores_{best_score, improvement_score, accepted_score, rejected_score} {}

  void set_operator_cnt(size_t destroy_operator_cnt, size_t repair_operator_cnt) override {
    destroy_weights_.assign(destroy_operator_cnt, 1.0);
    repair_weights_.assign(repair_operator_cnt, 1.0);
  }

  std::tuple<size_t, size_t> select_operators(RandomEngine& gen) override {
    std::discrete_distribution<size_t> destroy_dist(destroy_weights_.begin(), destroy_weights_.end());
    std::discrete_distribution<size_t> repair_dist(repair_weights_.begin(), repair_weights_.end());

    last_destroy_ = destroy_dist(gen);
    last_repair_ = repair_dist(gen);

    return {last_destroy_, last_repair_};
  }

  void update(double current, double previous, double best) override {
    double reward = 0.0;
    if (current < best) {
      reward = scores_[0];
    } else if (current < previous) {
      reward = scores_[1];
    } else if (current == previous) {
      reward = scores_[2];
    } else {
      reward = scores_[3];
    }

    update_weight(destroy_weights_, last_destroy_, reward);
    update_weight(repair_weights_, last_repair_, reward);
  }

  void print_weights() const override {
    std::cout << "Destroy weights: ";
    for (double w : destroy_weights_) std::cout << w << " ";
    std::cout << "\nRepair weights: ";
    for (double w : repair_weights_) std::cout << w << " ";
    std::cout << "\n";
  }

 private:
  size_t last_destroy_ = 0;
  size_t last_repair_ = 0;

  std::vector<double> destroy_weights_;
  std::vector<double> repair_weights_;

  double decay_;
  std::array<double, 4> scores_;  // [best, improvement, accepted, rejected]

  void update_weight(std::vector<double>& weights, size_t index, double score) {
    weights[index] = (1.0 - decay_) * weights[index] + decay_ * score;
  }
};

}  // namespace alns
