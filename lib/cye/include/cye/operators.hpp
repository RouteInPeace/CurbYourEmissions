#pragma once

#include <set>
#include <unordered_set>

#include "cye/individual.hpp"
#include "meta/common.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/local_search.hpp"
#include "meta/ga/mutation.hpp"

namespace cye {

class NeighborSwap : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  NeighborSwap(size_t k) : k_(k) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override;

 private:
  std::set<std::pair<float, size_t>> candidates_;
  size_t k_;
};

class RouteOX1 : public meta::ga::CrossoverOperator<cye::EVRPIndividual> {
 public:
  auto crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1, cye::EVRPIndividual const &p2)
      -> cye::EVRPIndividual;

 private:
  std::unordered_set<meta::ga::GeneT<cye::EVRPIndividual>> used_;
};

class SwapSearch : public meta::ga::LocalSearch<cye::EVRPIndividual> {
 public:
  SwapSearch(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, std::shared_ptr<cye::Instance> instance)
      : energy_repair_(energy_repair), instance_(instance) {}

  [[nodiscard]] auto search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual)
      -> cye::EVRPIndividual override;

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  std::shared_ptr<cye::Instance> instance_;

  [[nodiscard]] auto neighbor_dist_(std::vector<size_t> const &base, size_t i) -> float;
};

}  // namespace cye