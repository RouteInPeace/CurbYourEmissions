#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <print>
#include <vector>
#include "crossover.hpp"
#include "meta/common.hpp"
#include "mutation.hpp"
#include "selection.hpp"

namespace meta::ga {

template <Individual I>
class GeneticAlgorithm {
 public:
  GeneticAlgorithm(std::vector<I> &&population, std::unique_ptr<CrossoverOperator<I>> crossover_operator,
                   std::unique_ptr<MutationOperator<I>> mutation_operator,
                   std::unique_ptr<SelectionOperator<I>> selection_operator, size_t max_iterations, bool verbose);

  auto optimize(RandomEngine &re) -> void;

  [[nodiscard]] inline auto &population() const { return population_; }
  [[nodiscard]] auto best_individual() const -> I const &;

 private:
  std::vector<I> population_;
  std::unique_ptr<CrossoverOperator<I>> crossover_operator_;
  std::unique_ptr<MutationOperator<I>> mutation_operator_;
  std::unique_ptr<SelectionOperator<I>> selection_operator_;
  size_t max_iterations_;
  bool verbose_;
};

template <Individual I>
GeneticAlgorithm<I>::GeneticAlgorithm(std::vector<I> &&population,
                                      std::unique_ptr<CrossoverOperator<I>> crossover_operator,
                                      std::unique_ptr<MutationOperator<I>> mutation_operator,
                                      std::unique_ptr<SelectionOperator<I>> selection_operator, size_t max_iterations,
                                      bool verbose)
    : population_(std::move(population)),
      crossover_operator_(std::move(crossover_operator)),
      mutation_operator_(std::move(mutation_operator)),
      selection_operator_(std::move(selection_operator)),
      max_iterations_(max_iterations),
      verbose_(verbose) {}

template <Individual I>
auto GeneticAlgorithm<I>::optimize(RandomEngine &re) -> void {
  auto best_fitness = std::numeric_limits<float>::infinity();
  for (const auto &individual : population_) {
    if (individual.fitness() < best_fitness) {
      best_fitness = individual.fitness();
    }
  }

  for (auto iter = 0UZ; iter < max_iterations_; ++iter) {
    auto [p1, p2, r] = selection_operator_->select(re, population_);
    auto child = crossover_operator_->crossover(re, population_[p1], population_[p2]);
    auto mutant = mutation_operator_->mutate(re, std::move(child));
    mutant.update_fitness();
    if (mutant.fitness() < best_fitness) {
      best_fitness = mutant.fitness();
    }

    population_[r] = std::move(mutant);

    if (verbose_ && iter % 10000 == 0) {
      std::println("Iteration: {}, Best individual: {}", iter, best_fitness);
    }
  }
}

template <Individual I>
auto GeneticAlgorithm<I>::best_individual() const -> I const & {
  auto best = &population_[0];

  for (auto &individual : population_) {
    if (individual.fitness() < best->fitness()) {
      best = &individual;
    }
  }

  return *best;
}

}  // namespace meta