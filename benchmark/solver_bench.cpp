#include <benchmark/benchmark.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/operators.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "cye/stall_handler.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"
#include "meta/ga/ssga.hpp"
#include "serial/json_archive.hpp"

static std::mutex stats_mutex;
static std::vector<double> global_best_costs;
static std::atomic<int> instance_counter(0);

static void BM_GA_Optimization(benchmark::State &state) {
  auto archive = serial::JSONArchive("dataset/json/X-n143-k7.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);
  std::random_device rd;
  std::mt19937 gen(rd());
  auto population_size = 1000UZ;

  std::vector<double> local_best_costs;

  for (auto _ : state) {
    auto population = std::vector<cye::EVRPIndividual>();
    population.reserve(population_size);
    for (size_t i = 0; i < population_size; ++i) {
      population.emplace_back(energy_repair, cye::stochastic_rank_nearest_neighbor(gen, instance, 3));
    }

    auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(3);

    meta::ga::SSGA<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                           std::make_unique<cye::SwapSearch>(instance), cye::EVRPStallHandler(),
                                           1'000'000'000UZ, true);

    // ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
    //  ga.add_crossover_operator(std::make_unique<RouteOX1>());
    ga.add_crossover_operator(std::make_unique<cye::DistributedCrossover>());
    // ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());
    ga.add_mutation_operator(std::make_unique<cye::HSM>(instance));
    ga.add_mutation_operator(std::make_unique<cye::HMM>(instance));

    ga.optimize(gen);
    auto best_individual = ga.best_individual();
    auto best_cost = best_individual.cost();

    auto solution = best_individual.solution();
    solution.clear_patches();
    cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
    energy_repair->patch(solution, 100001);

    local_best_costs.push_back(std::min(best_cost, solution.cost()));
  }

  {
    std::lock_guard<std::mutex> lock(stats_mutex);
    global_best_costs.insert(global_best_costs.end(), local_best_costs.begin(), local_best_costs.end());
  }

  instance_counter++;
  if (state.thread_index() == 0) {
    while (instance_counter < state.threads()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!global_best_costs.empty()) {
      std::sort(global_best_costs.begin(), global_best_costs.end());
      double sum = std::accumulate(global_best_costs.begin(), global_best_costs.end(), 0.0);
      double average = sum / global_best_costs.size();
      double min = *std::min_element(global_best_costs.begin(), global_best_costs.end());
      double max = *std::max_element(global_best_costs.begin(), global_best_costs.end());
      double median = global_best_costs.size() % 2 == 0 ? (global_best_costs[global_best_costs.size() / 2 - 1] +
                                                           global_best_costs[global_best_costs.size() / 2]) /
                                                              2
                                                        : global_best_costs[global_best_costs.size() / 2];

      state.counters["average"] = average;
      state.counters["min"] = min;
      state.counters["max"] = max;
      state.counters["median"] = median;
    }

    global_best_costs.clear();
  }
}
BENCHMARK(BM_GA_Optimization)->Iterations(1)->Unit(benchmark::kMillisecond)->Threads(1);
