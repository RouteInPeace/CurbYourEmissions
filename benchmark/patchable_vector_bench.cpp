#include <benchmark/benchmark.h>
#include <random>
#include <cye/patchable_vector.hpp>

static void BM_PatchableVectorIteration(benchmark::State &state) {
  auto gen = std::mt19937(0);

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);
  auto element_cnt = 1000UZ;
  auto patch_cnt = 5UZ;
  auto change_cnt = 50UZ;

  auto elements = std::vector<size_t>();
  for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

  auto patchable_vec = cye::PatchableVector<size_t>(std::move(elements));

  for (auto p = 0UZ; p < patch_cnt; ++p) {
    auto insertion_dist = std::uniform_int_distribution(0UZ, patchable_vec.size());
    std::vector<std::pair<size_t, size_t>> changes;

    for (auto i = 0UZ; i < change_cnt; ++i) {
      changes.emplace_back(insertion_dist(gen), dist(gen));
    }
    std::ranges::sort(changes);
   
    auto patch = cye::Patch<size_t>();
    for (auto [ind, value] : changes) {
      patch.add_change(ind, value);
    }
    patchable_vec.add_patch(std::move(patch));
  }

  for (auto _ : state) {
    for(auto x : patchable_vec) {
      benchmark::DoNotOptimize(x);
    }
  }
}
BENCHMARK(BM_PatchableVectorIteration);

BENCHMARK_MAIN();