#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <utility>
#include <vector>
#include "cye/individual.hpp"

namespace cye {

class EVRPStallHandler {
 public:
  auto operator()(meta::RandomEngine &gen, std::vector<cye::EVRPIndividual> &population, float best_fitness)
      -> std::pair<size_t, float> {
    static size_t kTrivial = 500'000UZ;
    static size_t kOptimal = 20'000UZ;

    // Improvement
    if (prev_best_fitness_ > best_fitness) {
      prev_best_fitness_ = best_fitness;

      if (state_ == State_::Optimal) {
        for (auto &individual : population) {
          individual.switch_to_trivial();
          individual.update_fitness();
        }
      }
      state_ = State_::Trivial;
      return std::make_pair(kTrivial, prev_best_fitness_);
    } else if (state_ == State_::Optimal) {
      // No improvement at all
      std::cout << "No improvement in optimal state, ending...\n";
      return std::make_pair(0UZ, prev_best_fitness_);
    }

    if (state_ == State_::Begin) {
      std::cout << "Switching to trivial state...\n";
      state_ = State_::Trivial;
      return std::make_pair(kTrivial, prev_best_fitness_);
    } else if (state_ == State_::Trivial) {
      state_ = State_::Purge;
      std::cout << "Purging population...\n";
      std::sort(population.begin(), population.end(),
                [](const cye::EVRPIndividual &a, const cye::EVRPIndividual &b) { return a.fitness() < b.fitness(); });

      for (auto i = 1UZ; i < population.size(); ++i) {
        std::shuffle(population[i].genotype().begin(), population[i].genotype().begin(), gen);
      }
      return std::make_pair(kTrivial, prev_best_fitness_);
    } else {
      std::cout << "Switching to optimal state...\n";
      state_ = State_::Optimal;
      for (auto &individual : population) {
        individual.switch_to_optimal();
        individual.update_fitness();
        prev_best_fitness_ = std::min(prev_best_fitness_, individual.fitness());
      }

      return std::make_pair(kOptimal, prev_best_fitness_);
    }
  }

 private:
  enum class State_ { Begin, Trivial, Purge, Optimal };

  State_ state_ = State_::Begin;
  float prev_best_fitness_{std::numeric_limits<float>::infinity()};
};

}  // namespace cye