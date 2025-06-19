#pragma once

#include <utility>
#include <vector>
#include "cye/individual.hpp"
#include "meta/common.hpp"

namespace cye {

class EVRPStallHandler {
 public:
  auto operator()(meta::RandomEngine & /*gen*/, std::vector<cye::EVRPIndividual> & /*population*/, double best_fitness)
      -> std::pair<size_t, double> {
    if (!started_) {
      started_ = true;
      return std::make_pair(1'000'000UZ, best_fitness);
    }

    return std::make_pair(0UZ, best_fitness);
  }

 private:
  bool started_ = false;
};

}  // namespace cye