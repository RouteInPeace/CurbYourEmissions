#include <benchmark/benchmark.h>
#include <iostream>
#include <random>
#include <utility>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/common.hpp"
#include "meta/ga/ga.hpp"
#include "serial/json_archive.hpp"

class EVRPIndividual {
 public:
  auto update_fitness() {
    solution_.clear_patches();
    cye::patch_endpoint_depots(solution_);
    cye::patch_cargo_trivially(solution_);
    cye::patch_energy_trivially(solution_);
    assert(solution_.is_valid());
  }

  EVRPIndividual(cye::Solution &&solution) : solution_(std::move(solution)) { update_fitness(); }

  inline auto fitness() const { return solution_.cost(); }
  inline auto genotype() const { return solution_.base(); }
  inline auto genotype() { return solution_.base(); }
  inline auto &solution() const { return solution_; }

 private:
  cye::Solution solution_;
};

static void BM_GA(benchmark::State &state) {
  auto gen = std::mt19937(0);

  auto archive = serial::JSONArchive("dataset/json/E-n101-k8.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 10000UZ;
  auto max_iter = 10000UZ;

  auto population = std::vector<EVRPIndividual>();
  population.reserve(population_size);

  for (auto i = 0UZ; i < population_size; ++i) {
    population.emplace_back(cye::random_customer_permutation(gen, instance));
  }

  auto crossover_operator = std::make_unique<meta::ga::OX1<EVRPIndividual>>();
  auto mutation_operator = std::make_unique<meta::ga::TwoOpt<EVRPIndividual>>();
  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<EVRPIndividual>>(5);

  auto ga = meta::ga::GeneticAlgorithm<EVRPIndividual>(
      std::move(population), std::move(selection_operator),
      [](meta::RandomEngine &, std::vector<EVRPIndividual> &, float best) { return std::make_pair(100000000UZ, best); },
      max_iter, false);

  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<EVRPIndividual>>());

  for (auto _ : state) {
    ga.optimize(gen);
    auto individual = ga.best_individual();
    benchmark::DoNotOptimize(individual);
  }
}

BENCHMARK(BM_GA)->Unit(benchmark::kMillisecond);
