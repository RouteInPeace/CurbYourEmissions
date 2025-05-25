
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <random>
#include <ranges>
#include <vector>

#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/ga.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"
#include "serial/json_archive.hpp"

class EVRPIndividual {
 public:
  auto update_fitness() {
    solution_.clear_patches();
    cye::patch_endpoint_depots(solution_);
    if (trivial_) {
      cye::patch_cargo_trivially(solution_);
      cye::patch_energy_trivially(solution_);
    } else {
      cye::patch_cargo_optimally(solution_, static_cast<unsigned>(solution_.instance().cargo_capacity()) + 1u);
      energy_repair_->patch(solution_, 101u);
    }
    assert(solution_.is_valid());
  }

  EVRPIndividual(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, cye::Solution &&solution)
      : energy_repair_(energy_repair), solution_(std::move(solution)) {
    update_fitness();
  }

  inline auto fitness() const { return solution_.cost(); }
  inline auto genotype() const { return solution_.base(); }
  inline auto genotype() { return solution_.base(); }
  inline auto &solution() const { return solution_; }
  inline auto switch_to_optimal() { trivial_ = false; }

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  cye::Solution solution_;
  bool trivial_ = true;
};

class NeighborSwap : public meta::ga::MutationOperator<EVRPIndividual> {
 public:
  NeighborSwap(size_t k) : k_(k) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, EVRPIndividual &&individual) -> EVRPIndividual override {
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

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/X-n143-k7.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 4000UZ;
  auto max_iter = 3000000UZ;

  auto population = std::vector<EVRPIndividual>();
  population.reserve(population_size);
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);

  for (auto i = 0UZ; i < population_size; ++i) {
    // population.emplace_back(energy_repair, cye::random_customer_permutation(gen, instance));
    population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 3));
  }

  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<EVRPIndividual>>(5);

  auto ga =
      meta::ga::GeneticAlgorithm<EVRPIndividual>(std::move(population), std::move(selection_operator), max_iter, true);
  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<EVRPIndividual>>());
  // ga.add_crossover_operator(std::make_unique<meta::ga::PMX<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<NeighborSwap>(3));

  auto start_t = std::chrono::steady_clock::now();

  ga.optimize(gen);
  for (auto &individual : ga.population()) {
    individual.switch_to_optimal();
    individual.update_fitness();
  }
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
