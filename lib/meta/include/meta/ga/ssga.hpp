#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <random>
#include <stdexcept>
#include <vector>
#include "crossover.hpp"
#include "meta/common.hpp"
#include "meta/ga/local_search.hpp"
#include "mutation.hpp"
#include "selection.hpp"

namespace meta::ga {

template <Individual I>
class SSGA {
 public:
  using StallHandler = std::function<std::pair<size_t, float>(RandomEngine &, std::vector<I> &, float)>;

  SSGA(std::vector<I> &&population, std::unique_ptr<SSGASelectionOperator<I>> selection_operator,
       std::unique_ptr<LocalSearch<I>> local_search, StallHandler stall_handler, size_t max_iterations, bool verbose);

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
  static constexpr auto cmp_ = [](size_t x) { return x; };
  std::vector<I> population_;
  std::unordered_set<size_t, decltype(cmp_)> exists_;
  std::vector<std::unique_ptr<CrossoverOperator<I>>> crossover_operators_;
  std::vector<std::unique_ptr<MutationOperator<I>>> mutation_operators_;
  std::unique_ptr<SSGASelectionOperator<I>> selection_operator_;
  std::unique_ptr<LocalSearch<I>> local_search_;
  StallHandler stall_handler_;
  size_t last_improvement_;
  size_t max_iterations_;
  bool verbose_;
};

template <Individual I>
SSGA<I>::SSGA(std::vector<I> &&population, std::unique_ptr<SSGASelectionOperator<I>> selection_operator,
              std::unique_ptr<LocalSearch<I>> local_search, StallHandler stall_handler, size_t max_iterations,
              bool verbose)
    : population_(std::move(population)),
      selection_operator_(std::move(selection_operator)),
      local_search_(std::move(local_search)),
      stall_handler_(std::move(stall_handler)),
      last_improvement_(0),
      max_iterations_(max_iterations),
      verbose_(verbose) {}

template <Individual I>
auto SSGA<I>::optimize(RandomEngine &gen) -> void {
  if (crossover_operators_.empty()) {
    throw std::runtime_error("At least one crossover operator is required.");
  }
  if (mutation_operators_.empty()) {
    throw std::runtime_error("At least one mutation operator is required.");
  }

  auto best_fitness = std::numeric_limits<float>::infinity();
  for (const auto &individual : population_) {
    exists_.insert(individual.hash());
    if (individual.fitness() < best_fitness) {
      best_fitness = individual.fitness();
    }
  }

  last_improvement_ = 0;
  auto [stall_threshold, _] = stall_handler_(gen, population_, best_fitness);

  auto crossover_selection_dist = std::uniform_int_distribution(0UZ, crossover_operators_.size() - 1);
  auto mutation_selection_dist = std::uniform_int_distribution(0UZ, mutation_operators_.size() - 1);

  for (auto iter = 0UZ; iter < max_iterations_; ++iter) {
    auto crossover_operator_ind = crossover_selection_dist(gen);
    auto mutation_operator_ind = mutation_selection_dist(gen);

    auto [p1, p2, r] = selection_operator_->select(gen, population_);
    auto child = crossover_operators_[crossover_operator_ind]->crossover(gen, population_[p1], population_[p2]);
    auto mutant = mutation_operators_[mutation_operator_ind]->mutate(gen, std::move(child));
    auto final = local_search_->search(gen, std::move(mutant));
    final.update_fitness();

    if (final.fitness() < best_fitness) {
      best_fitness = final.fitness();
      last_improvement_ = iter;
    }

    if (!exists_.contains(final.hash())) {
      exists_.insert(final.hash());
      exists_.erase(population_[r].hash());
      population_[r] = std::move(final);
    }

    if (iter - last_improvement_ == stall_threshold) {
      auto ret = stall_handler_(gen, population_, best_fitness);
      stall_threshold = ret.first;
      best_fitness = ret.second;
      last_improvement_ = iter;

      if (stall_threshold == 0) {
        std::println("Stall handler returned zero threshold, stopping optimization.");
        break;
      }
    }

    if (verbose_ && iter % 1000 == 0) {
      std::println("Iteration: {}, Best individual: {}", iter, best_fitness);
    }
  }
}

template <Individual I>
auto SSGA<I>::best_individual() const -> I const & {
  auto best = &population_[0];

  for (auto &individual : population_) {
    if (individual.fitness() < best->fitness()) {
      best = &individual;
    }
  }

  return *best;
}

}  // namespace meta::ga