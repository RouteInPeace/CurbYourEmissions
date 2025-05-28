#include <benchmark/benchmark.h>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "serial/json_archive.hpp"

static void BM_Repair_PatchCargoTrivially(benchmark::State &state) {
  auto archive = serial::JSONArchive("dataset/json/X-n916-k207.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  solution.pop_patch();

  for (auto _ : state) {
    solution.pop_patch();
    cye::patch_cargo_trivially(solution);
    benchmark::DoNotOptimize(solution);
  }
}

static void BM_Repair_PatchCargoOptimally(benchmark::State &state) {
  auto archive = serial::JSONArchive("dataset/json/X-n916-k207.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  solution.pop_patch();

  for (auto _ : state) {
    solution.pop_patch();
    cye::patch_cargo_optimally(solution);
    benchmark::DoNotOptimize(solution);
  }
}

static void BM_Repair_PatchEnergyTrivially(benchmark::State &state) {
  auto archive = serial::JSONArchive("dataset/json/X-n916-k207.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);

  for (auto _ : state) {
    solution.pop_patch();
    cye::patch_energy_trivially(solution);
    benchmark::DoNotOptimize(solution);
  }
}

static void BM_Repair_PatchEnergyOptimally(benchmark::State &state) {
  auto archive = serial::JSONArchive("dataset/json/X-n916-k207.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  auto energy_repair = cye::OptimalEnergyRepair(instance);

  for (auto _ : state) {
    solution.pop_patch();
    energy_repair.patch(solution, 101u);
    benchmark::DoNotOptimize(solution);
  }
}

BENCHMARK(BM_Repair_PatchCargoTrivially)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Repair_PatchCargoOptimally)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Repair_PatchEnergyTrivially)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Repair_PatchEnergyOptimally)->Unit(benchmark::kMillisecond);
