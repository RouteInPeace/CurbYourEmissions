
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
#include "caliper/caliper.hpp"
#include "cye/individual.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/operators.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/ga/generational_ga.hpp"
#include "meta/ga/selection.hpp"
#include "serial/json_archive.hpp"

struct Config {
  std::filesystem::path instance_path = "dataset/json/E-n22-k4.json";
  size_t population_size = 10;
  size_t generation_cnt = 300;
  size_t elite_cnt = 2;
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

  auto population = std::vector<cye::EVRPIndividual>();
  population.reserve(config.population_size);
  population.emplace_back(energy_repair, cye::nearest_neighbor(instance));
  population.back().switch_to_optimal();
  for (size_t i = 1; i < config.population_size; ++i) {
    population.emplace_back(energy_repair, cye::stochastic_rank_nearest_neighbor(gen, instance, 2));
    population.back().switch_to_optimal();

  }

  auto selection_operator = std::make_unique<meta::ga::RankSelection<cye::EVRPIndividual>>(1.60);

  meta::ga::GenerationalGA<cye::EVRPIndividual> ga(std::move(population), std::move(selection_operator),
                                                   config.elite_cnt, config.generation_cnt, true);

  ga.add_crossover_operator(std::make_unique<cye::DistributedCrossover>());
  ga.add_mutation_operator(std::make_unique<cye::HMM>(instance));
  ga.add_mutation_operator(std::make_unique<cye::HSM>(instance));
  ga.add_local_search(std::make_unique<cye::SOTASearch>(instance));

  ga.optimize(gen);
  auto best_individual = ga.best_individual();
  auto best_cost = best_individual.cost();

  auto solution = best_individual.solution();
  solution.clear_patches();
  cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
  energy_repair->patch(solution, config.energy_repair_bins);

  return std::min(best_cost, solution.cost());
}

struct Stats {
  double min;
  double max;
  double mean;
  double std;
};

auto calculate_stats(std::vector<double> const &data) -> Stats {
  if (data.empty()) {
    throw std::invalid_argument("Cannot calculate stats for empty vector");
  }

  auto stats = Stats();
  stats.min = std::numeric_limits<double>::max();
  stats.max = std::numeric_limits<double>::lowest();
  auto sum = 0.0;
  auto sum_sq = 0.0;

  for (double value : data) {
    if (value < stats.min) stats.min = value;
    if (value > stats.max) stats.max = value;
    sum += value;
    sum_sq += value * value;
  }

  stats.mean = sum / data.size();
  double variance = (sum_sq / data.size()) - (stats.mean * stats.mean);
  stats.std = std::sqrt(variance);

  return stats;
}

auto stat_measurement(Config const &config, size_t sample_cnt) -> Stats {
  auto caliper = cali::Caliper<Config, double>(measurement);

  caliper.add_measurement(sample_cnt, config);

  auto tasks = caliper.run(std::thread::hardware_concurrency());
  auto result = tasks[0].results;

  return calculate_stats(result);
}

// Pybind11 module definition
PYBIND11_MODULE(cye_module, m) {
  m.doc() = "Electric vehicle routing solver";

  pybind11::class_<Config>(m, "Config")
      .def(pybind11::init<>())
      .def_readwrite("instance_path", &Config::instance_path)
      .def_readwrite("population_size", &Config::population_size)
      .def_readwrite("generation_cnt", &Config::generation_cnt)
      .def_readwrite("elite_cnt", &Config::elite_cnt)
      .def_readwrite("energy_repair_bins", &Config::energy_repair_bins);

  pybind11::class_<Stats>(m, "Stat")
      .def(pybind11::init<>())
      .def_readwrite("min", &Stats::min)
      .def_readwrite("max", &Stats::max)
      .def_readwrite("mean", &Stats::mean)
      .def_readwrite("std", &Stats::std);

  // Bind the main function
  m.def("measuremnt", &measurement, "Measure once", pybind11::arg("config"));
  m.def("stat_measurement", &stat_measurement, "Measure stat", pybind11::arg("config"), pybind11::arg("sample_cnt"));
}