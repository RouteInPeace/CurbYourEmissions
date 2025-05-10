#include "ga/ga.hpp"
#include <gtest/gtest.h>
#include <array>
#include <cstdlib>
#include <print>
#include <random>
#include <span>
#include <vector>
#include "ga/crossover.hpp"
#include "ga/mutation.hpp"
#include "ga/selection.hpp"

class Dummy {
 public:
  Dummy(float fitness) : fitness_(fitness) {}

  auto fitness() const -> float { return fitness_; };
  auto get_genotype() const -> std::span<const float> { return genotype_; }
  auto get_mutable_genotype() -> std::span<float> { return genotype_; }
  auto update_fitness() -> void {}

 private:
  std::array<float, 5> genotype_;
  float fitness_;
};

class QuadraticEquation {
 public:
  QuadraticEquation(std::vector<std::pair<float, float>> *dataset, float a, float b, float c)
      : dataset_(dataset), genotype_({a, b, c}), fitness_(0.f) {
    update_fitness();
  }

  auto evaluate(float x) -> float { return genotype_[0] * x * x + genotype_[1] * x + genotype_[2]; }
  auto fitness() const -> float { return fitness_; };
  auto get_genotype() const -> std::span<const float> { return genotype_; }
  auto get_mutable_genotype() -> std::span<float> { return genotype_; }
  auto update_fitness() -> void {
    fitness_ = 0.f;

    for (auto [x, y] : *dataset_) {
      fitness_ += std::abs(evaluate(x) - y);
    }
  }

  auto a() { return genotype_[0]; }
  auto b() { return genotype_[1]; }
  auto c() { return genotype_[2]; }

 private:
  std::vector<std::pair<float, float>> *dataset_;
  std::array<float, 3> genotype_;
  float fitness_;
};

TEST(GA, KWayTournamentSelection) {
  auto rd = std::random_device();
  auto re = std::mt19937(rd());

  auto dist = std::uniform_real_distribution<float>(-10.f, 10.f);
  auto selection_operator = ga::KWayTournamentSelectionOperator<Dummy, float>(3);

  auto population = std::vector<Dummy>();
  for (auto i = 0UZ; i < 1000; i++) {
    population.emplace_back(dist(re));
  }

  for (auto i = 0UZ; i < 10000; i++) {
    auto [p1, p2, r] = selection_operator.select(re, population);
    EXPECT_TRUE(population[p1].fitness() <= population[p2].fitness());
    EXPECT_TRUE(population[p2].fitness() <= population[r].fitness());
  }
}

TEST(GA, BasicRegression) {
  auto rd = std::random_device();
  auto re = std::mt19937(rd());

  auto dist = std::uniform_real_distribution<float>(-10.f, 10.f);
  auto a = dist(re);
  auto b = dist(re);
  auto c = dist(re);

  std::vector<std::pair<float, float>> dataset;

  for (auto i = 0UZ; i < 100; i++) {
    auto x = dist(re);
    auto y = a * x * x + b * x + c;
    dataset.push_back({x, y});
  }

  auto config = ga::Config<QuadraticEquation, float>();
  auto population_size = 100UZ;

  for (auto i = 0UZ; i < population_size; i++) {
    config.population.emplace_back(&dataset, dist(re), dist(re), dist(re));
  }

  config.crossover_operator = std::make_unique<ga::BLXAlpha<QuadraticEquation>>(0.5);
  config.mutation_operator = std::make_unique<ga::GaussianMutation<QuadraticEquation>>(2.0);
  config.selection_operator = std::make_unique<ga::KWayTournamentSelectionOperator<QuadraticEquation, float>>(7);
  config.max_iterations = 100000;
  config.verbose = true;

  ga::optimize(re, config);

  auto best = &config.population[0];
  for (auto &ind : config.population) {
    if (ind.fitness() < best->fitness()) {
      best = &ind;
    }
  }

  std::println("Expected: a: {}, b: {}, c: {}", a, b, c);
  std::println("Best found: a: {}, b: {}, c: {}", best->a(), best->b(), best->c());
}