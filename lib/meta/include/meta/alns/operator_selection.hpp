#pragma once

#include <cstddef>
#include <cstdlib>
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

}  // namespace meta::alns
