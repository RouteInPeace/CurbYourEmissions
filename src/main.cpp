
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <vector>

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
    auto routes = std::vector<size_t>();
    routes.reserve(customers_.size() + 2);
    routes.push_back(instance_->depot_id());
    for (auto c : customers_) routes.push_back(c);
    routes.push_back(instance_->depot_id());

    auto solution = cye::Solution(instance_, std::move(routes));
    cye::patch_cargo_trivially(solution);
    cye::patch_energy_trivially(solution);

    assert(solution.is_valid());
    fitness_ = solution.get_cost();
  }

  EVRPIndividual(std::shared_ptr<cye::Instance> instance, std::vector<size_t> &&customers)
      : instance_(instance), customers_(customers) {
    update_fitness();
  }

  auto fitness() const { return fitness_; }
  auto &get_genotype() const { return customers_; }
  auto &get_mutable_genotype() { return customers_; }

 private:
  std::shared_ptr<cye::Instance> instance_;
  float fitness_;
  std::vector<size_t> customers_;
};

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/E-n101-k8.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto population_size = 100000UZ;
  auto max_iter = 100000000UZ;

  auto population = std::vector<EVRPIndividual>();
  population.reserve(population_size);

  for (auto i = 0UZ; i < population_size; ++i) {
    auto customers = instance->customer_ids() | std::ranges::to<std::vector<size_t>>();
    std::ranges::shuffle(customers, gen);
    population.emplace_back(instance, std::move(customers));
  }

  auto crossover_operator = std::make_unique<meta::ga::PMX<EVRPIndividual>>();
  auto mutation_operator = std::make_unique<meta::ga::TwoOpt<EVRPIndividual>>();
  auto selection_operator = std::make_unique<meta::ga::KWayTournamentSelectionOperator<EVRPIndividual>>(11);

  auto ga = meta::ga::GeneticAlgorithm<EVRPIndividual>(std::move(population), std::move(crossover_operator),
                                                   std::move(mutation_operator), std::move(selection_operator),
                                                   max_iter, true);

  ga.optimize(gen);

  return 0;
}
