#pragma once

#include <functional>
#include <memory>
#include <print>
#include <vector>

#include "acceptance_criterion.hpp"
#include "operator_selection.hpp"
#include "meta/common.hpp"

namespace meta::alns {

template <typename Solution>
struct Config {
  Config() = default;
  inline Config(Solution &&sol) : initial_solution(sol) {}

  std::unique_ptr<AcceptanceCriterion> acceptance_criterion;
  std::unique_ptr<OperatorSelection> operator_selection;
  std::vector<std::function<Solution(Solution &&, RandomEngine &)>> destroy_operators;
  std::vector<std::function<Solution(Solution &&, RandomEngine &)>> repair_operators;
  Solution initial_solution;
  size_t max_iterations;
  bool verbose;
};

template <typename Solution>
auto optimize(Config<Solution> const &config, RandomEngine &gen) -> Solution {
  config.operator_selection->set_operator_cnt(config.destroy_operators.size(), config.repair_operators.size());

  auto current_solution = config.initial_solution;
  auto best_solution = config.initial_solution;

  for (size_t i = 0; i < config.max_iterations; ++i) {
    auto [destroy_operator_id, repair_operator_id] = config.operator_selection->select_operators(gen);

    auto old_cost = current_solution.cost();

    auto destroyed_solution = config.destroy_operators[destroy_operator_id](std::move(current_solution), gen);
    auto repaired_solution = config.repair_operators[repair_operator_id](std::move(destroyed_solution), gen);

    auto new_cost = repaired_solution.cost();
    auto best_cost = best_solution.cost();

    if (config.acceptance_criterion->accept(new_cost, old_cost, best_cost, gen)) {
      if (new_cost < best_cost) {
        best_solution = repaired_solution;
      }
      current_solution = std::move(repaired_solution);
    }
    config.operator_selection->update(new_cost, old_cost, best_cost);

    if (config.verbose && i % 100 == 0) {
      std::println("Iteration: {}, Current cost: {}, Best const: {}", i, current_solution.cost(),
                   best_solution.cost());
    }
  }

  return best_solution;
}

};  // namespace alns
