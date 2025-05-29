#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <utility>
#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "meta/common.hpp"
#include "meta/ga/ga.hpp"
#include "meta/ga/local_search.hpp"
#include "serial/json_archive.hpp"

static void BM_GA(benchmark::State &state) {
  auto gen = std::mt19937(0);

  auto archive = serial::JSONArchive("dataset/json/E-n101-k8.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 10000UZ;
  auto max_iter = 10000UZ;

  auto population = std::vector<cye::EVRPIndividual>();
  population.reserve(population_size);

  for (auto i = 0UZ; i < population_size; ++i) {
    population.emplace_back(nullptr, cye::random_customer_permutation(gen, instance));
  }

  auto crossover_operator = std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>();
  auto mutation_operator = std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>();
  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(5);

  auto ga = meta::ga::GeneticAlgorithm<cye::EVRPIndividual>(
      std::move(population), std::move(selection_operator), std::make_unique<meta::ga::NoSearch<cye::EVRPIndividual>>(),
      [](meta::RandomEngine &, std::vector<cye::EVRPIndividual> &, float best) {
        return std::make_pair(100000000UZ, best);
      },
      max_iter, false);

  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());

  for (auto _ : state) {
    ga.optimize(gen);
    auto individual = ga.best_individual();
    benchmark::DoNotOptimize(individual);
  }
}

BENCHMARK(BM_GA)->Unit(benchmark::kMillisecond);
