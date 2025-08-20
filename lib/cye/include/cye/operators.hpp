#pragma once

#include <set>
#include <unordered_set>

#include "cye/individual.hpp"
#include "cye/repair.hpp"
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

class TwoOptSearch : public meta::ga::LocalSearch<cye::EVRPIndividual> {
 public:
  TwoOptSearch(std::shared_ptr<cye::Instance> instance)
      : energy_repair_(std::make_shared<cye::OptimalEnergyRepair>(instance)), instance_(instance) {}

  [[nodiscard]] auto search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual)
      -> cye::EVRPIndividual override;

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  std::shared_ptr<cye::Instance> instance_;
};

class SwapSearch : public meta::ga::LocalSearch<cye::EVRPIndividual> {
 public:
  SwapSearch(std::shared_ptr<cye::Instance> instance)
      : energy_repair_(std::make_shared<cye::OptimalEnergyRepair>(instance)), instance_(instance) {}

  [[nodiscard]] auto search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual)
      -> cye::EVRPIndividual override;

 private:
  [[nodiscard]] auto neighbor_dist_(std::vector<size_t> const &base, size_t i) -> float;

  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  std::shared_ptr<cye::Instance> instance_;
};

class SATwoOptSearch : public meta::ga::LocalSearch<cye::EVRPIndividual> {
 public:
  SATwoOptSearch(std::shared_ptr<cye::Instance> instance) : instance_(instance) {}

  [[nodiscard]] auto search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual)
      -> cye::EVRPIndividual override;

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  std::shared_ptr<cye::Instance> instance_;
};

class HSM : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  HSM(std::shared_ptr<cye::Instance> instance) : instance_(std::move(instance)) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override;
  std::shared_ptr<cye::Instance> instance_;
};

class HMM : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  HMM(std::shared_ptr<cye::Instance> instance) : instance_(std::move(instance)) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override;
  std::shared_ptr<cye::Instance> instance_;
};

class DistributedCrossover : public meta::ga::CrossoverOperator<cye::EVRPIndividual> {
 public:
  [[nodiscard]] cye::EVRPIndividual crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1,
                                              cye::EVRPIndividual const &p2) override;
  std::optional<cye::EVRPIndividual> other_;
  std::unordered_set<size_t> customers1_set_;
  std::unordered_set<size_t> customers2_set_;
};

class CrossRouteScramble : public meta::ga::MutationOperator<EVRPIndividual> {
 public:
  CrossRouteScramble(double mutation_rate);

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, EVRPIndividual &&individual) -> EVRPIndividual override;

 private:
  double mutation_rate_;
};

}  // namespace cye