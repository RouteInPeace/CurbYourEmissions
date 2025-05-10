#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <print>
#include "crossover.hpp"
#include "ga/common.hpp"
#include "mutation.hpp"
#include "selection.hpp"

namespace ga {

template <typename I, typename T>
  requires Individual<I, T>
struct Config {
  std::vector<I> population;
  std::unique_ptr<CrossoverOperator<I, T>> crossover_operator;
  std::unique_ptr<MutationOperator<I, T>> mutation_operator;
  std::unique_ptr<SelectionOperator<I, T>> selection_operator;
  size_t max_iterations;
  bool verbose;
};

template <typename I, typename T>
  requires Individual<I, T>
auto optimize(RandomEngine &re, Config<I, T> &config) {
  auto &population = config.population;

  auto best_fitness = std::numeric_limits<float>::infinity();
  for (const auto &individual : population) {
    if (individual.fitness() < best_fitness) {
      best_fitness = individual.fitness();
    }
  }

  for (auto iter = 0UZ; iter < config.max_iterations; ++iter) {
    auto [p1, p2, r] = config.selection_operator->select(re, population);
    auto child = config.crossover_operator->crossover(re, population[p1], population[p2]);
    auto mutant = config.mutation_operator->mutate(re, std::move(child));
    mutant.update_fitness();
    if(mutant.fitness() < best_fitness) {
      best_fitness = mutant.fitness();
    }

    population[r] = std::move(mutant);

    if (config.verbose && iter % 100 == 0) {
      std::println("Iteration: {}, Best individual: {}", iter, best_fitness);
    }
  }
}

}  // namespace ga