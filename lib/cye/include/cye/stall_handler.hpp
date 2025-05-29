#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include "cye/individual.hpp"

namespace cye {

class EVRPStallHandler {
 public:
  auto operator()(meta::RandomEngine &gen, std::vector<cye::EVRPIndividual> &population, float best_fitness)
      -> std::pair<size_t, float> {
    if (state_ == State_::Begin) {
      state_ = State_::Trivial;
      return std::make_pair(1'000'000UZ, best_fitness);
    }
    // else if (state_ == State_::Trivial) {
    //   state_ = State_::Optimal;
    //   for (auto &individual : population) {
    //     individual.switch_to_optimal();
    //     individual.update_fitness();
    //     best_fitness = std::min(best_fitness, individual.fitness());
    //   }

    //   return std::make_pair(20'000UZ, best_fitness);
    // } else {
    //   return std::make_pair(0UZ, best_fitness);
    // }

    return std::make_pair(0UZ, best_fitness);
  }

 private:
  enum class State_ { Begin, Trivial, Optimal };

  State_ state_ = State_::Begin;
};

}  // namespace cye