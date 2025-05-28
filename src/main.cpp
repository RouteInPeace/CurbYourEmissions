
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <ranges>
#include <vector>

#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/common.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/ga.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"
#include "serial/json_archive.hpp"
#include "cye/individual.hpp"

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

class EVRPStallHandler {
 public:
  auto operator()(meta::RandomEngine &gen, std::vector<cye::EVRPIndividual> &population, float best_fitness)
      -> std::pair<size_t, float> {
    if (state_ == State_::Begin) [[unlikely]] {
      state_ = State_::Trivial;
      return std::make_pair(20'000'000UZ, best_fitness);
    } else {
      std::cout << "Purging population...\n";

      std::sort(population.begin(), population.end(),
                [](const cye::EVRPIndividual &a, const cye::EVRPIndividual &b) { return a.fitness() < b.fitness(); });

      for (auto &ind : population) {
        for (auto x : ind.solution().routes()) {
          std::cout << x << ' ';
        }
        std::cout << '\n';
      }
      std::exit(1);
    }
    // if (state_ == State_::Trivial) {
    //   std::cout << "Population is trivial, switching to optimal...\n";
    //   auto new_best_fitness = std::numeric_limits<float>::infinity();
    //   for (auto &individual : population) {
    //     individual.switch_to_optimal();
    //     individual.update_fitness();
    //     new_best_fitness = std::min(new_best_fitness, individual.fitness());
    //   }

    //   prev_best_fitness_ = new_best_fitness;
    //   state_ = State_::Optimal;
    //   return std::make_pair(10'000UZ, new_best_fitness);

    // } else if (state_ == State_::Optimal) {
    //   if (best_fitness == prev_best_fitness_) {
    //     // Purge
    //     std::cout << "Purging population...\n";
    //     std::sort(population.begin(), population.end(),
    //               [](const EVRPIndividual &a, const EVRPIndividual &b) { return a.fitness() < b.fitness(); });
    //     for (auto i = 1UZ; i < population.size(); ++i) {
    //       std::shuffle(population[i].genotype().begin(), population[i].genotype().begin(), gen);
    //     }
    //   } else {
    //     std::cout << "Switch back to trivial...\n";
    //   }

    //   auto new_best_fitness = std::numeric_limits<float>::infinity();
    //   for (auto &individual : population) {
    //     individual.switch_to_trivial();
    //     individual.update_fitness();
    //     new_best_fitness = std::min(new_best_fitness, individual.fitness());
    //   }

    //   state_ = State_::Trivial;
    //   return std::make_pair(1'000'000UZ, new_best_fitness);
    // }

    assert(false);
    return std::make_pair(1'000'000UZ, best_fitness);
  }

 private:
  enum class State_ {
    Begin,
    Trivial,
    Optimal,
  };

  State_ state_ = State_::Begin;
  float prev_best_fitness_;
};

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/X-n1001-k43.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 2000UZ;
  auto max_iter = 1'000'000'000UZ;

  auto population = std::vector<cye::EVRPIndividual>();
  population.reserve(population_size);
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);

  for (auto i = 0UZ; i < population_size; ++i) {
    // population.emplace_back(energy_repair, cye::random_customer_permutation(gen, instance));
    population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 3));
  }

  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(5);

  auto ga = meta::ga::GeneticAlgorithm<cye::EVRPIndividual>(std::move(population), std::move(selection_operator),
                                                       EVRPStallHandler(), max_iter, true);
  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
  // ga.add_crossover_operator(std::make_unique<meta::ga::PMX<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<NeighborSwap>(3));

  auto start_t = std::chrono::steady_clock::now();

  ga.optimize(gen);

  auto end_t = std::chrono::steady_clock::now();
  std::cout << "Time: " << std::chrono::duration_cast<std::chrono::seconds>(end_t - start_t).count() << "s\n";

  auto solution = ga.best_individual().solution();
  solution.clear_patches();
  cye::patch_endpoint_depots(solution);
  cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
  energy_repair->patch(solution, 100001);
  std::cout << "Cost: " << solution.cost() << '\n';

  return 0;
}
