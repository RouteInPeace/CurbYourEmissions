#include "ga/ga.hpp"
#include <gtest/gtest.h>
#include <array>
#include <cstdlib>
#include <random>
#include <span>
#include <vector>

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

    fitness_ /= static_cast<float>(dataset_->size());
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
  auto selection_operator = ga::KWayTournamentSelectionOperator<Dummy>(3);

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

  auto population_size = 100UZ;
  auto population = std::vector<QuadraticEquation>();
  population.reserve(population_size);

  for (auto i = 0UZ; i < population_size; i++) {
    population.emplace_back(&dataset, dist(re), dist(re), dist(re));
  }

  auto ga = ga::GeneticAlgorithm<QuadraticEquation>(
      std::move(population), std::make_unique<ga::BLXAlpha<QuadraticEquation>>(0.f),
      std::make_unique<ga::GaussianMutation<QuadraticEquation>>(0.02),
      std::make_unique<ga::KWayTournamentSelectionOperator<QuadraticEquation>>(5), 200000, false);

  ga.optimize(re);
  auto &best = ga.best_individual();

  EXPECT_LE(best.fitness(), 0.01f);
}