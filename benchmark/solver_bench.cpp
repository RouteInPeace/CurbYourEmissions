#include <benchmark/benchmark.h>
#include <algorithm>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/mutations.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "cye/stall_handler.hpp"
#include "meta/ga/crossover.hpp"
#include "meta/ga/ga.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"
#include "serial/json_archive.hpp"

static std::mutex stats_mutex;
static std::vector<double> global_best_costs;

class SwapSearch : public meta::ga::LocalSearch<cye::EVRPIndividual> {
 public:
  SwapSearch(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, std::shared_ptr<cye::Instance> instance)
      : energy_repair_(energy_repair), instance_(instance) {}

  [[nodiscard]] auto search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual)
      -> cye::EVRPIndividual override {
    auto &solution = individual.solution();
    auto base = solution.base();
    solution.clear_patches();
    cye::patch_endpoint_depots(solution);
    cye::patch_cargo_trivially(solution);

    const auto &cargo_patch = solution.get_patch(1);

    for (auto i = 0UZ; i <= cargo_patch.size(); i++) {
      auto route_begin = i == 0 ? 0 : cargo_patch.changes()[i - 1].ind;
      auto route_end = i == cargo_patch.size() ? base.size() - 1 : cargo_patch.changes()[i].ind - 1;

      auto stop = false;
      while (!stop) {
        stop = true;
        for (auto l = route_begin; l < route_end; l++) {
          for (auto k = l + 1; k <= route_end; k++) {
            auto prev_dist = neighbor_dist_(base, l) + neighbor_dist_(base, k);
            std::swap(base[l], base[k]);
            auto new_dist = neighbor_dist_(base, l) + neighbor_dist_(base, k);

            if (new_dist < prev_dist) {
              stop = false;
            } else {
              std::swap(base[l], base[k]);
            }
          }
        }
      }
    }
    cye::patch_energy_trivially(solution);

    return individual;
  }

 private:
  std::shared_ptr<cye::OptimalEnergyRepair> energy_repair_;
  std::shared_ptr<cye::Instance> instance_;

  [[nodiscard]] auto neighbor_dist_(std::span<size_t> base, size_t i) -> float {
    auto d = 0.f;
    if (i > 0) {
      d += instance_->distance(base[i - 1], base[i]);
    }
    if (i < base.size() - 1) {
      d += instance_->distance(base[i], base[i + 1]);
    }

    return d;
  }
};

class RouteOX1 : public meta::ga::CrossoverOperator<cye::EVRPIndividual> {
 public:
  auto crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1, cye::EVRPIndividual const &p2)
      -> cye::EVRPIndividual {
    auto child = p1;

    auto &&p1_genotype = p1.genotype();
    auto &&p2_genotype = p2.genotype();
    auto &&child_genotype = child.genotype();

    auto &depot_patch = p1.solution().get_patch(1);
    auto dist = std::uniform_int_distribution(0UZ, depot_patch.size());
    auto ind = dist(gen);
    auto j = ind == depot_patch.size() ? p1_genotype.size() - 1 : depot_patch.changes()[ind].ind - 1;
    auto i = ind == 0 ? 0 : depot_patch.changes()[ind - 1].ind;

    assert(p1_genotype.size() == p2_genotype.size());

    assert(i <= j);

    used_.clear();
    for (auto k = i; k <= j; ++k) {
      used_.emplace(p1_genotype[k]);
    }

    auto l = 0UZ;
    for (auto k = 0UZ; k < p1_genotype.size(); ++k) {
      // Skip the copied section
      if (k == i) {
        k = j;
        continue;
      }

      while (used_.contains(p2_genotype[l])) {
        ++l;
      }

      child_genotype[k] = p2_genotype[l];
      ++l;
    }

    return child;
  }

 private:
  std::unordered_set<meta::ga::GeneT<cye::EVRPIndividual>> used_;
};

std::shared_ptr<cye::Instance> load_instance() {
  auto archive = serial::JSONArchive("dataset/json/E-n76-k7.json");
  return std::make_shared<cye::Instance>(archive.root());
}

static void BM_GA_Optimization(benchmark::State &state) {
  auto instance = load_instance();
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);
  std::random_device rd;
  std::mt19937 gen(rd());
  auto population_size = 1000UZ;

  std::vector<double> local_best_costs;

  for (auto _ : state) {
    auto population = std::vector<cye::EVRPIndividual>();
    population.reserve(population_size);
    for (size_t i = 0; i < population_size; ++i) {
      population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 3));
    }

    auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(5);

    meta::ga::GeneticAlgorithm<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                                       std::make_unique<SwapSearch>(energy_repair, instance),
                                                       cye::EVRPStallHandler(), 1'000'000'000UZ, true);

    ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
    // ga.add_crossover_operator(std::make_unique<RouteOX1>());
    ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());

    ga.optimize(gen);
    auto best_individual = ga.best_individual();
    auto best_cost = best_individual.fitness();

    auto solution = best_individual.solution();
    solution.clear_patches();
    cye::patch_endpoint_depots(solution);
    cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
    energy_repair->patch(solution, 100001);

    local_best_costs.push_back(std::min(best_cost, solution.cost()));
  }

  {
    std::lock_guard<std::mutex> lock(stats_mutex);
    global_best_costs.insert(global_best_costs.end(), local_best_costs.begin(), local_best_costs.end());
  }

  if (state.thread_index() == 0) {
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
BENCHMARK(BM_GA_Optimization)->Iterations(1)->Unit(benchmark::kMillisecond)->Threads(8);
