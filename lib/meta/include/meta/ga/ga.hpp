#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <print>
#include <random>
#include <stdexcept>
#include <vector>
#include "crossover.hpp"
#include "meta/common.hpp"
#include "mutation.hpp"
#include "selection.hpp"

namespace meta::ga {

template <Individual I>
class GeneticAlgorithm {
 public:
  GeneticAlgorithm(std::vector<I> &&population, std::unique_ptr<SelectionOperator<I>> selection_operator,
                   size_t max_iterations, bool verbose);

  auto optimize(RandomEngine &re) -> void;

  [[nodiscard]] inline auto &population() const { return population_; }
  [[nodiscard]] inline auto &population() { return population_; }
  [[nodiscard]] auto best_individual() const -> I const &;

  inline auto add_crossover_operator(std::unique_ptr<CrossoverOperator<I>> crossover_operator) -> void {
    crossover_operators_.push_back(std::move(crossover_operator));
  }
  inline auto add_mutation_operator(std::unique_ptr<MutationOperator<I>> mutation_operator) -> void {
    mutation_operators_.push_back(std::move(mutation_operator));
  }

 private:
  std::vector<I> population_;
  std::vector<std::unique_ptr<CrossoverOperator<I>>> crossover_operators_;
  std::vector<std::unique_ptr<MutationOperator<I>>> mutation_operators_;
  std::unique_ptr<SelectionOperator<I>> selection_operator_;
  size_t max_iterations_;
  bool verbose_;
};

template <Individual I>
GeneticAlgorithm<I>::GeneticAlgorithm(std::vector<I> &&population,
                                      std::unique_ptr<SelectionOperator<I>> selection_operator, size_t max_iterations,
                                      bool verbose)
    : population_(std::move(population)),
      selection_operator_(std::move(selection_operator)),
      max_iterations_(max_iterations),
      verbose_(verbose) {}

template <Individual I>
auto GeneticAlgorithm<I>::optimize(RandomEngine &gen) -> void {
  if (crossover_operators_.empty()) {
    throw std::runtime_error("At least one crossover operator is required.");
  }
  if (mutation_operators_.empty()) {
    throw std::runtime_error("At least one mutation operator is required.");
  }

  auto best_fitness = std::numeric_limits<float>::infinity();
  for (const auto &individual : population_) {
    if (individual.fitness() < best_fitness) {
      best_fitness = individual.fitness();
    }
  }

  auto crossover_selection_dist = std::uniform_int_distribution(0UZ, crossover_operators_.size() - 1);
  auto mutation_selection_dist = std::uniform_int_distribution(0UZ, mutation_operators_.size() - 1);

  for (auto iter = 0UZ; iter < max_iterations_; ++iter) {
    auto mutation_operator_ind = mutation_selection_dist(gen);
    auto crossover_operator_ind = crossover_selection_dist(gen);

    auto [p1, p2, r] = selection_operator_->select(gen, population_);
    auto child = crossover_operators_[crossover_operator_ind]->crossover(gen, population_[p1], population_[p2]);
    auto mutant = mutation_operators_[mutation_operator_ind]->mutate(gen, std::move(child));
    mutant.update_fitness();
    if (mutant.fitness() < best_fitness) {
      best_fitness = mutant.fitness();
    }

    population_[r] = std::move(mutant);

    if (verbose_ && iter % 100 == 0) {
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

}  // namespace meta::ga