
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/operators.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/ga/generational_ga.hpp"
#include "meta/ga/generational_ga.hpp"
#include "meta/ga/selection.hpp"
#include "serial/json_archive.hpp"

struct Config {
  std::filesystem::path instance_path = "dataset/json/E-n22-k4.json";
  size_t population_size = 200;
  size_t generation_cnt = 1000;
  size_t elite_cnt = 30;
  size_t energy_repair_bins = 100001;
};

auto measurement(Config const &config) -> double {
  auto archive = serial::JSONArchive(config.instance_path);
struct Config {
  std::filesystem::path instance_path = "dataset/json/E-n22-k4.json";
  size_t population_size = 200;
  size_t generation_cnt = 1000;
  size_t elite_cnt = 30;
  size_t energy_repair_bins = 100001;
};

auto measurement(Config const &config) -> double {
  auto archive = serial::JSONArchive(config.instance_path);
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);
  std::random_device rd;
  std::mt19937 gen(rd());

  auto max_evaluations_allowed = 25'000 * (1 + instance->customer_cnt() + instance->charging_station_cnt());
  auto evaluations = config.population_size * config.generation_cnt;

  std::println("{}/{} evaluations", evaluations, max_evaluations_allowed);

  if (evaluations > max_evaluations_allowed) {
    throw std::runtime_error("You are not allowed to do that many evaluations.");
  }
  auto energy_repair = std::make_shared<cye::OptimalEnergyRepair>(instance);
  std::random_device rd;
  std::mt19937 gen(rd());

  auto max_evaluations_allowed = 25'000 * (1 + instance->customer_cnt() + instance->charging_station_cnt());
  auto evaluations = config.population_size * config.generation_cnt;

  std::println("{}/{} evaluations", evaluations, max_evaluations_allowed);

  if (evaluations > max_evaluations_allowed) {
    throw std::runtime_error("You are not allowed to do that many evaluations.");
  }

  auto population = std::vector<cye::EVRPIndividual>();
  population.reserve(config.population_size);
  for (size_t i = 0; i < config.population_size; ++i) {
  population.reserve(config.population_size);
  for (size_t i = 0; i < config.population_size; ++i) {
    population.emplace_back(energy_repair, cye::stochastic_rank_nearest_neighbor(gen, instance, 3));
  }

  auto selection_operator = std::make_unique<meta::ga::RankSelection<cye::EVRPIndividual>>(1.60);
  auto selection_operator = std::make_unique<meta::ga::RankSelection<cye::EVRPIndividual>>(1.60);

  meta::ga::GenerationalGA<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                                   config.elite_cnt, config.generation_cnt, true);

  ga.add_crossover_operator(std::make_unique<cye::DistributedCrossover>());
  ga.add_mutation_operator(std::make_unique<cye::HMM>(instance));
  ga.add_mutation_operator(std::make_unique<cye::HSM>(instance));

  ga.add_local_search(std::make_unique<cye::TwoOptSearch>(instance));
  ga.add_local_search(std::make_unique<cye::SwapSearch>(instance));
  meta::ga::GenerationalGA<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                                   config.elite_cnt, config.generation_cnt, true);

  ga.add_crossover_operator(std::make_unique<cye::DistributedCrossover>());
  ga.add_mutation_operator(std::make_unique<cye::HMM>(instance));
  ga.add_mutation_operator(std::make_unique<cye::HSM>(instance));

  ga.add_local_search(std::make_unique<cye::TwoOptSearch>(instance));
  ga.add_local_search(std::make_unique<cye::SwapSearch>(instance));

  ga.optimize(gen);
  auto best_individual = ga.best_individual();
  auto best_cost = best_individual.cost();

  auto solution = best_individual.solution();
  solution.clear_patches();
  cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
  energy_repair->patch(solution, config.energy_repair_bins);

  best_cost = std::min(best_cost, solution.cost());

  return best_cost;
}

// Pybind11 module definition
PYBIND11_MODULE(cye_module, m) {
  m.doc() = "Expensive optimization function with config structs";

  // Bind the Config struct
  pybind11::class_<Config>(m, "Config")
      .def(pybind11::init<>())  // Default constructor
      .def_readwrite("instance_path", &Config::instance_path)
      .def_readwrite("population_size", &Config::population_size)
      .def_readwrite("generation_cnt", &Config::generation_cnt)
      .def_readwrite("elite_cnt", &Config::elite_cnt)
      .def_readwrite("energy_repair_bins", &Config::energy_repair_bins)
      // Optional: add a string representation for debugging
      .def("__repr__", [](const Config &c) {
        return "Config(instance_path='" + c.instance_path.string() +
               "', population_size=" + std::to_string(c.population_size) +
               ", generation_cnt=" + std::to_string(c.generation_cnt) + ", elite_cnt=" + std::to_string(c.elite_cnt) +
               ", energy_repair_bins=" + std::to_string(c.energy_repair_bins) + ")";
      });

  // Bind the main function
  m.def("run_optimization", &measurement, "Run the expensive optimization algorithm", pybind11::arg("config"));
}