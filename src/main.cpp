
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

#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/operators.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "cye/stall_handler.hpp"
#include "meta/common.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/local_search.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"
#include "meta/ga/ssga.hpp"
#include "serial/json_archive.hpp"

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 2000UZ;
  auto max_iter = 1UZ;

  auto population = std::vector<cye::EVRPIndividual>();
  population.reserve(population_size);
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);

  for (auto i = 0UZ; i < population_size; ++i) {
    // population.emplace_back(energy_repair, cye::random_customer_permutation(gen, instance));
    population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 3));
  }

  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(5);

  auto ga = meta::ga::SSGA<cye::EVRPIndividual>(std::move(population), std::move(selection_operator),
                                                std::make_unique<meta::ga::NoSearch<cye::EVRPIndividual>>(),
                                                cye::EVRPStallHandler(), max_iter, true);
  ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
  // ga.add_crossover_operator(std::make_unique<meta::ga::PMX<EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());
  ga.add_mutation_operator(std::make_unique<cye::NeighborSwap>(3));

  auto start_t = std::chrono::steady_clock::now();

  ga.optimize(gen);

  auto end_t = std::chrono::steady_clock::now();
  std::cout << "Time: " << std::chrono::duration_cast<std::chrono::seconds>(end_t - start_t).count() << "s\n";

  auto output_archive = serial::JSONArchive();
  auto root = output_archive.root();
  root.emplace("instance", *instance);
  root.emplace("solution", ga.best_individual().solution());
  output_archive.save("solution.json");

  return 0;
}
