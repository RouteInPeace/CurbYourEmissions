#include "meta/ga/ga.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <random>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
#include "meta/ga/crossover.hpp"
#include "meta/ga/mutation.hpp"
#include "meta/ga/selection.hpp"

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
  auto selection_operator = meta::ga::KWayTournamentSelectionOperator<Dummy>(3);

  auto population = std::vector<Dummy>();
  for (auto i = 0UZ; i < 10000; i++) {
    population.emplace_back(dist(re));
  }

  for (auto i = 0UZ; i < 100000UZ; i++) {
    auto [p1, p2, r] = selection_operator.select(re, population);

    EXPECT_LE(population[p1].fitness(), population[p2].fitness());
    EXPECT_LE(population[p2].fitness(), population[r].fitness());
  }
}

class StringIndividual {
 public:
  StringIndividual(std::string_view genotype) : genotype_(genotype) {}

  [[nodiscard]] inline auto &get_genotype() const { return genotype_; }
  [[nodiscard]] inline auto &get_mutable_genotype() { return genotype_; }
  [[nodiscard]] inline auto fitness() { return 0.f; }
  [[nodiscard]] inline auto update_fitness() {}

 private:
  std::string genotype_;
};

class IntIndividual {
 public:
  IntIndividual(size_t size) : genotype_(std::views::iota(0UZ, size) | std::ranges::to<std::vector>()) {}

  [[nodiscard]] inline auto &get_genotype() const { return genotype_; }
  [[nodiscard]] inline auto &get_mutable_genotype() { return genotype_; }
  [[nodiscard]] inline auto fitness() { return 0.f; }
  [[nodiscard]] inline auto update_fitness() {}

 private:
  std::vector<size_t> genotype_;
};

TEST(GA, PMXCrossoverBasic) {
  auto re = std::mt19937(40);

  auto p1 = StringIndividual("abcdefgh");
  auto p2 = StringIndividual("cgeafhbd");
  auto expected = std::string("cghdefba");

  auto crossover = meta::ga::PMX<StringIndividual>();
  auto child = crossover.crossover(re, p1, p2);
  EXPECT_EQ(child.get_genotype(), expected);
}

TEST(GA, PMXCrossoverStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto p1 = IntIndividual(1000UZ);
  auto p2 = IntIndividual(1000UZ);
  std::ranges::shuffle(p2.get_mutable_genotype(), gen);

  auto crossover = meta::ga::PMX<IntIndividual>();

  for (auto i = 0UZ; i < 1000UZ; ++i) {
    auto child = crossover.crossover(gen, p1, p2);

    std::ranges::sort(child.get_mutable_genotype());

    for (auto [x, y] : std::views::zip(child.get_genotype(), p1.get_genotype())) {
      EXPECT_EQ(x, y);
    }
  }
}

TEST(GA, TwoOptMutationSimple) {
  auto gen = std::mt19937(0);

  auto parent = StringIndividual("abcdefgh");
  auto mutation = meta::ga::TwoOpt<StringIndividual>();

  auto m1 = mutation.mutate(gen, std::move(parent));
  EXPECT_EQ(m1.get_genotype(), std::string("abcdefgh"));

  auto m2 = mutation.mutate(gen, std::move(m1));
  EXPECT_EQ(m2.get_genotype(), std::string("abcdegfh"));

  auto m3 = mutation.mutate(gen, std::move(m2));
  EXPECT_EQ(m3.get_genotype(), std::string("abcdfgeh"));
}

TEST(GA, TwoOptMutationStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto parent = IntIndividual(1000UZ);
  auto mutation = meta::ga::TwoOpt<IntIndividual>();

  for (auto i = 0UZ; i < 1000UZ; ++i) {
    auto copy = parent;
    auto child = mutation.mutate(gen, std::move(copy));

    std::ranges::sort(child.get_mutable_genotype());

    for (auto [x, y] : std::views::zip(child.get_genotype(), parent.get_genotype())) {
      EXPECT_EQ(x, y);
    }
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

  auto ga = meta::ga::GeneticAlgorithm<QuadraticEquation>(
      std::move(population), std::make_unique<meta::ga::BLXAlpha<QuadraticEquation>>(0.f),
      std::make_unique<meta::ga::GaussianMutation<QuadraticEquation>>(0.02),
      std::make_unique<meta::ga::KWayTournamentSelectionOperator<QuadraticEquation>>(5), 200000, false);

  ga.optimize(re);
  auto &best = ga.best_individual();

  EXPECT_LE(best.fitness(), 0.01f);
}