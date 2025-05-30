#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <unordered_set>
#include <vector>
#include "meta/ga/common.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/local_search.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"

namespace meta::ga {

template <Individual I>
class GenerationalGA {
 public:
  GenerationalGA(std::vector<I> &&population, std::unique_ptr<GenGASelectionOperator<I>> selection_operator,
                 double n_elite, size_t max_iterations, bool verbose);

  auto optimize(RandomEngine &gen) -> void;
  inline auto add_crossover_operator(std::unique_ptr<CrossoverOperator<I>> crossover_operator) -> void {
    crossover_operators_.push_back(std::move(crossover_operator));
  }
  inline auto add_mutation_operator(std::unique_ptr<MutationOperator<I>> mutation_operator) -> void {
    mutation_operators_.push_back(std::move(mutation_operator));
  }
  inline auto add_local_search(std::unique_ptr<LocalSearch<I>> local_search) -> void {
    local_search_.push_back(std::move(local_search));
  }
  [[nodiscard]] auto best_individual() const -> I const &;

 private:
  static constexpr auto cmp_ = [](size_t x) { return x; };

  std::vector<I> population_;
  std::unordered_set<size_t, decltype(cmp_)> exists_;
  std::vector<std::unique_ptr<CrossoverOperator<I>>> crossover_operators_;
  std::vector<std::unique_ptr<MutationOperator<I>>> mutation_operators_;
  std::unique_ptr<GenGASelectionOperator<I>> selection_operator_;
  std::vector<std::unique_ptr<LocalSearch<I>>> local_search_;
  size_t n_elite_;
  size_t max_iterations_;
  bool verbose_;
};

template <Individual I>
GenerationalGA<I>::GenerationalGA(std::vector<I> &&population,
                                  std::unique_ptr<GenGASelectionOperator<I>> selection_operator, double n_elite,
                                  size_t max_iterations, bool verbose)
    : population_(std::move(population)),
      selection_operator_(std::move(selection_operator)),
      n_elite_(n_elite),
      max_iterations_(max_iterations),
      verbose_(verbose) {}

template <Individual I>
auto GenerationalGA<I>::optimize(RandomEngine &gen) -> void {
  if (crossover_operators_.empty()) {
    throw std::runtime_error("At least one crossover operator is required.");
  }
  if (mutation_operators_.empty()) {
    throw std::runtime_error("At least one mutation operator is required.");
  }
  if (n_elite_ > population_.size()) {
    throw std::runtime_error("Number of elites is larger that the population size.");
  }

  for (auto &individual : population_) {
    individual.update_cost();
  }

  std::ranges::sort(population_, [](I const &a, I const &b) { return a.cost() < b.cost(); });

  auto crossover_selection_dist = std::uniform_int_distribution(0UZ, crossover_operators_.size() - 1);
  auto mutation_selection_dist = std::uniform_int_distribution(0UZ, mutation_operators_.size() - 1);
  auto local_search_selection_dist = std::uniform_int_distribution(0UZ, local_search_.size() - 1);

  auto cur_population = population_;
  auto prev_population = std::move(population_);

  for (auto iter = 0UZ; iter < max_iterations_; ++iter) {
    // Fill the new population

    exists_.clear();

    // Elitizam
    for (auto i = 0UZ; i < n_elite_; ++i) {
      // solution is already copied in
      cur_population[i] = prev_population[i];
      exists_.insert(prev_population[i].hash());
    }

    // The common folk
    selection_operator_->prepare(prev_population);
    for (auto i = n_elite_; i < prev_population.size();) {
      auto crossover_operator_ind = crossover_selection_dist(gen);
      auto mutation_operator_ind = mutation_selection_dist(gen);
      auto local_search_ind = local_search_selection_dist(gen);

      auto [p1, p2] = selection_operator_->select(gen);
      auto child =
          crossover_operators_[crossover_operator_ind]->crossover(gen, prev_population[p1], prev_population[p2]);
      auto mutant = mutation_operators_[mutation_operator_ind]->mutate(gen, std::move(child));
      auto final = local_search_[local_search_ind]->search(gen, std::move(mutant));
      final.update_cost();

      if (!exists_.contains(final.hash())) {
        exists_.insert(final.hash());
        cur_population[i] = std::move(final);
        i++;
      }
    }

    // Sort
    std::ranges::sort(cur_population, [](I const &a, I const &b) { return a.cost() < b.cost(); });

    if (verbose_) {
      std::println("Generation: {}, Best individual: {}", iter, cur_population[0].cost());
    }

    std::swap(cur_population, prev_population);
  }

  population_ = std::move(prev_population);
}

template <Individual I>
auto GenerationalGA<I>::best_individual() const -> I const & {
  return population_[0];
}

}  // namespace meta::ga
