#include "meta/ga/ssga.hpp"
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
  auto genotype() const -> std::span<const float> { return genotype_; }
  auto genotype() -> std::span<float> { return genotype_; }
  auto update_fitness() -> void {}
  auto hash() const { return 0UZ; }

 private:
  std::array<float, 5> genotype_;
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

  [[nodiscard]] inline auto &genotype() const { return genotype_; }
  [[nodiscard]] inline auto &genotype() { return genotype_; }
  [[nodiscard]] inline auto fitness() { return 0.f; }
  [[nodiscard]] inline auto update_fitness() {}
  [[nodiscard]] auto hash() const { return 0UZ; }

 private:
  std::string genotype_;
};

class IntIndividual {
 public:
  IntIndividual(size_t size) : genotype_(std::views::iota(0UZ, size) | std::ranges::to<std::vector>()) {}

  [[nodiscard]] inline auto &genotype() const { return genotype_; }
  [[nodiscard]] inline auto &genotype() { return genotype_; }
  [[nodiscard]] inline auto fitness() { return 0.f; }
  [[nodiscard]] inline auto update_fitness() {}
  [[nodiscard]] auto hash() const { return 0UZ; }

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
  EXPECT_EQ(child.genotype(), expected);
}

TEST(GA, PMXCrossoverStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto p1 = IntIndividual(1000UZ);
  auto p2 = IntIndividual(1000UZ);
  std::ranges::shuffle(p2.genotype(), gen);

  auto crossover = meta::ga::PMX<IntIndividual>();

  for (auto i = 0UZ; i < 1000UZ; ++i) {
    auto child = crossover.crossover(gen, p1, p2);

    std::ranges::sort(child.genotype());

    for (auto [x, y] : std::views::zip(child.genotype(), p1.genotype())) {
      EXPECT_EQ(x, y);
    }
  }
}

TEST(GA, OX1CrossoverBasic) {
  auto re = std::mt19937(40);

  auto p1 = StringIndividual("abcdefgh");
  auto p2 = StringIndividual("cgeafhbd");
  auto expected = std::string("cgadefhb");

  auto crossover = meta::ga::OX1<StringIndividual>();
  auto child = crossover.crossover(re, p1, p2);
  EXPECT_EQ(child.genotype(), expected);
}

TEST(GA, OX1CrossoverStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto p1 = IntIndividual(1000UZ);
  auto p2 = IntIndividual(1000UZ);
  std::ranges::shuffle(p2.genotype(), gen);

  auto crossover = meta::ga::OX1<IntIndividual>();

  for (auto i = 0UZ; i < 1000UZ; ++i) {
    auto child = crossover.crossover(gen, p1, p2);

    std::ranges::sort(child.genotype());

    for (auto [x, y] : std::views::zip(child.genotype(), p1.genotype())) {
      EXPECT_EQ(x, y);
    }
  }
}

TEST(GA, TwoOptMutationSimple) {
  auto gen = std::mt19937(0);

  auto parent = StringIndividual("abcdefgh");
  auto mutation = meta::ga::TwoOpt<StringIndividual>();

  auto m1 = mutation.mutate(gen, std::move(parent));
  EXPECT_EQ(m1.genotype(), std::string("abcdefgh"));

  auto m2 = mutation.mutate(gen, std::move(m1));
  EXPECT_EQ(m2.genotype(), std::string("abcdegfh"));

  auto m3 = mutation.mutate(gen, std::move(m2));
  EXPECT_EQ(m3.genotype(), std::string("abcdfgeh"));
}

TEST(GA, TwoOptMutationStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto parent = IntIndividual(1000UZ);
  auto mutation = meta::ga::TwoOpt<IntIndividual>();

  for (auto i = 0UZ; i < 1000UZ; ++i) {
    auto copy = parent;
    auto child = mutation.mutate(gen, std::move(copy));

    std::ranges::sort(child.genotype());

    for (auto [x, y] : std::views::zip(child.genotype(), parent.genotype())) {
      EXPECT_EQ(x, y);
    }
  }
}