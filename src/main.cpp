
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
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
    cye::patch_cargo_optimally(solution_, static_cast<unsigned>(solution_.instance().cargo_capacity()) + 1u);
    energy_repair_->patch(solution_, 101u);
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

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  cye::Solution solution_;
};

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/X-n143-k7.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 2000UZ;
  auto max_iter = 1000000UZ;

  auto population = std::vector<EVRPIndividual>();
  population.reserve(population_size);
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);

  for (auto i = 0UZ; i < population_size / 2; ++i) {
    population.emplace_back(energy_repair, cye::random_customer_permutation(gen, instance));
    population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 4));
  }

  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<EVRPIndividual>>(5);

  auto ga =
      meta::ga::GeneticAlgorithm<EVRPIndividual>(std::move(population), std::move(selection_operator), max_iter, true);
  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<EVRPIndividual>>());
  // ga.add_crossover_operator(std::make_unique<meta::ga::PMX<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<EVRPIndividual>>());
  // ga.add_mutation_operator(std::make_unique<meta::ga::Swap<EVRPIndividual>>());

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
