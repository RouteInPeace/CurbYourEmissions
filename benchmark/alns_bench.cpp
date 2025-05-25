#include <benchmark/benchmark.h>
#include <random>
#include "cye/destroy.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/alns/acceptance_criterion.hpp"
#include "meta/alns/alns.hpp"
#include "serial/json_archive.hpp"

// Current best on dataset/json/E-n22-k4.json 135 ms

static void BM_Alns(benchmark::State &state) {
  auto gen = std::mt19937(0);

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  meta::alns::Config<cye::Solution> config(cye::nearest_neighbor(instance));
  config.acceptance_criterion = std::make_unique<meta::alns::HillClimbingCriterion>();
  config.operator_selection = std::make_unique<meta::alns::RandomOperatorSelection>();
  config.destroy_operators = {[](cye::Solution &&solution, meta::RandomEngine &gen) {
    return cye::random_destroy(std::move(solution), gen, 0.8);
  }};
  config.repair_operators = {
      [](cye::Solution &&solution, meta::RandomEngine &gen) { return cye::regret_repair(std::move(solution), gen, 2); },
  };
  config.max_iterations = 100;
  config.verbose = false;

  for (auto _ : state) {
    auto solution = meta::alns::optimize(config, gen);
    benchmark::DoNotOptimize(solution);
  }
}

BENCHMARK(BM_Alns)->Unit(benchmark::kMillisecond);