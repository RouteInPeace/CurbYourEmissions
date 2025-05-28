#pragma once

#include <set>

#include "cye/individual.hpp"
#include "cye/instance.hpp"
#include "cye/solution.hpp"
#include "meta/common.hpp"
#include "meta/ga/mutation.hpp"

namespace cye {

class NeighborSwap : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  NeighborSwap(size_t k) : k_(k) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override {
    candidates_.clear();
    auto genotype = individual.genotype();
    auto &instance = individual.solution().instance();

    auto dist = std::uniform_int_distribution(0UZ, genotype.size() - 1);
    auto ind1 = dist(gen);
    auto node1_id = genotype[ind1];

    for (auto i = 0UZ; i < genotype.size(); ++i) {
      if (i == ind1) {
        continue;
      }
      auto node2_id = genotype[i];
      auto distance = instance.distance(node1_id, node2_id);

      candidates_.emplace(distance, i);
      if (candidates_.size() > k_) {
        candidates_.erase(--candidates_.end());
      }
    }

    auto dist2 = std::uniform_int_distribution(0UZ, k_ - 1);
    auto candidate_ind = std::min(dist2(gen), candidates_.size() - 1);
    auto it = candidates_.begin();
    while (candidate_ind > 0) {
      ++it;
      --candidate_ind;
    }
    auto ind2 = it->second;

    std::swap(genotype[ind1], genotype[ind2]);
    return individual;
  }

 private:
  std::set<std::pair<float, size_t>> candidates_;
  size_t k_;
};

}  // namespace cye