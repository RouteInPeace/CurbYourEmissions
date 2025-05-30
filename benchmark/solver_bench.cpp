#include <benchmark/benchmark.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
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

namespace {
  static auto find_route_(cye::Patch<size_t> const &patch, size_t index) -> std::pair<size_t, size_t> {
    auto it = std::ranges::upper_bound(patch.changes(), index, std::less{}, [&](auto const &el) { return el.ind; });

    auto route_end = it->ind;
    auto route_begin = (--it)->ind;

    return {route_begin, route_end};
  }
}

class HSM : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  HSM(std::shared_ptr<cye::Instance> instance)
      : instance_(std::move(instance)) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override {
    
    auto &solution = individual.solution();

    solution.clear_patches();
    cye::patch_cargo_trivially(solution);

    auto &customers = individual.genotype();
    auto dist = std::uniform_int_distribution<size_t>(0, customers.size() - 1);
    auto index = dist(gen);
    auto customer = customers[index];

    auto [route_begin, route_end] = find_route_(solution.get_patch(0), index);

    size_t best_id = 0;
    double best_dist = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < customers.size(); ++i) {
      if (i >= route_begin && i < route_end) continue;

      auto dist = instance_->distance(customer, customers[i]);
      if (dist < best_dist) {
        best_dist = dist;
        best_id = i;
      }
    }
    std::swap(customers[index], customers[best_id]);
    return individual;
  }
  std::shared_ptr<cye::Instance> instance_;
};

class HMM : public meta::ga::MutationOperator<cye::EVRPIndividual> {
 public:
  HMM(std::shared_ptr<cye::Instance> instance)
      : instance_(std::move(instance)) {}

  [[nodiscard]] auto mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual override {
    
    auto &solution = individual.solution();

    solution.clear_patches();
    cye::patch_cargo_trivially(solution);

    auto &customers = individual.genotype();
    auto dist = std::uniform_int_distribution<size_t>(0, customers.size() - 1);
    auto index = dist(gen);
    auto customer = customers[index];

    auto [route_begin, route_end] = find_route_(solution.get_patch(0), index);

    size_t best_id = 0;
    double best_dist = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < customers.size(); ++i) {
      if (i >= route_begin && i < route_end) continue;

      auto dist = instance_->distance(customer, customers[i]);
      if (dist < best_dist) {
        best_dist = dist;
        best_id = i;
      }
    }
    customers.erase(customers.begin() + index);

    if (best_id > index) {
      --best_id;
    }
    customers.insert(customers.begin() + best_id, customer);
    return individual;
  }
  std::shared_ptr<cye::Instance> instance_;
};

class DistributedCrossover : public meta::ga::CrossoverOperator<cye::EVRPIndividual> {
 public:
  [[nodiscard]] cye::EVRPIndividual crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1,
                                              cye::EVRPIndividual const &p2) {
    if (other_) {
      auto ret = std::move(*other_);
      other_ = std::nullopt;
      return ret;
    }

    customers1_set_.clear();
    customers2_set_.clear();

    auto const &customers1 = p1.genotype();
    auto const &customers2 = p2.genotype();

    auto dist = std::uniform_int_distribution(0UZ, customers1.size() - 1);
    auto index = dist(gen);
    // remove this
    index = 4;
    auto random_customer = customers1[index];

    const auto &route1_cargo_patch = p1.solution().get_patch(0);
    auto [route1_begin, route1_end] = find_route_(route1_cargo_patch, index);

    auto customer_location = std::ranges::find(customers2, random_customer) - customers2.begin();
    const auto &route_2_cargo_patch = p2.solution().get_patch(0);
    auto [route2_begin, route2_end] = find_route_(route_2_cargo_patch, customer_location);

    for (size_t i = route1_begin; i < route1_end; ++i) {
      customers1_set_.insert(customers1[i]);
    }
    for (size_t i = route2_begin; i < route2_end; ++i) {
      customers2_set_.insert(customers2[i]);
    }

    auto child1 = p1;
    auto child2 = p2;

    auto k = route1_begin;
    auto l = route2_begin;
    std::vector<size_t> insertions;
    for (auto &id : child1.genotype()) {
      auto contained = customers1_set_.contains(id) || customers2_set_.contains(id);
      if (!contained) continue;

      while (l < route2_end && customers1_set_.contains(customers2[l])) {
        l++;
      }

      if (l < route2_end) {
        id = customers2[l];
        insertions.push_back(customers2[l]);
        l++;
      } else {
        id = customers1[k];
        insertions.push_back(customers1[k]);
        k++;
      }
    }

    auto it = insertions.rbegin();
    for (auto &id : child2.genotype()) {
      auto contained = customers1_set_.contains(id) || customers2_set_.contains(id);
      if (!contained) continue;
      id = *it;
      ++it;
    }
    other_ = std::move(child2);

    return child1;
  }

  std::optional<cye::EVRPIndividual> other_;
  std::unordered_set<size_t> customers1_set_;
  std::unordered_set<size_t> customers2_set_;
};

// static void test(benchmark::State &state) {
//   auto archive = serial::JSONArchive("dataset/json/E-n76-k7.json");
//   auto instance = std::make_shared<cye::Instance>(archive.root());
//   auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);
//   std::random_device rd;
//   std::mt19937 gen(rd());

//   auto population = std::vector<cye::EVRPIndividual>();
//   std::vector<size_t> route1 = {5, 9, 8, 4, 2, 3, 6, 1, 7};
//   std::vector<size_t> route2 = {6, 2, 3, 9, 4, 8, 7, 1, 5};
//   cye::Patch<size_t> patch1;
//   patch1.add_change(0, 0);
//   patch1.add_change(4, 0);
//   patch1.add_change(6, 0);
//   patch1.add_change(9, 0);

//   cye::Patch<size_t> patch2;
//   patch2.add_change(0, 0);
//   patch2.add_change(4, 0);
//   patch2.add_change(6, 0);
//   patch2.add_change(9, 0);

//   cye::EVRPIndividual individual1(energy_repair, cye::Solution(instance, std::move(route1)));
//   cye::EVRPIndividual individual2(energy_repair, cye::Solution(instance, std::move(route2)));

//   individual1.solution().pop_patch();
//   individual2.solution().pop_patch();
//   individual1.solution().pop_patch();
//   individual2.solution().pop_patch();

//   individual1.solution().add_patch(std::move(patch1));
//   individual2.solution().add_patch(std::move(patch2));

//   auto crossover_operator = std::make_unique<DistributedCrossover>();
//   auto child1 = crossover_operator->crossover(gen, individual1, individual2);
//   auto child2 = crossover_operator->crossover(gen, individual1, individual2);

//   std::cout << "Bok";
// }
// BENCHMARK(test)->Iterations(1)->Unit(benchmark::kMillisecond);

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
      population.emplace_back(energy_repair, cye::stochastic_nearest_neighbor(gen, instance, 3));
    }

    auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<cye::EVRPIndividual>>(3);

    meta::ga::SSGA<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                           std::make_unique<cye::SwapSearch>(energy_repair, instance),
                                           cye::EVRPStallHandler(), 1'000'000'000UZ, true);

    //ga.add_crossover_operator(std::make_unique<meta::ga::OX1<cye::EVRPIndividual>>());
    // ga.add_crossover_operator(std::make_unique<RouteOX1>());
    ga.add_crossover_operator(std::make_unique<DistributedCrossover>());
    //ga.add_mutation_operator(std::make_unique<meta::ga::TwoOpt<cye::EVRPIndividual>>());
    ga.add_mutation_operator(std::make_unique<HSM>(instance));
    ga.add_mutation_operator(std::make_unique<HMM>(instance));

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
BENCHMARK(BM_GA_Optimization)->Iterations(1)->Unit(benchmark::kMillisecond)->Threads(8);
