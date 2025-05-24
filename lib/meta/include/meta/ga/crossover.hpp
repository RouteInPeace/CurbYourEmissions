#pragma once

#include <cassert>
#include <random>
#include <ranges>
#include <unordered_map>
#include "common.hpp"
#include "meta/common.hpp"

namespace meta::ga {

template <Individual I>
class CrossoverOperator {
 public:
  CrossoverOperator() = default;
  virtual ~CrossoverOperator() = default;

  CrossoverOperator(CrossoverOperator const &) = delete;
  auto operator=(CrossoverOperator const &) -> CrossoverOperator & = delete;

  CrossoverOperator(CrossoverOperator &&) = default;
  auto operator=(CrossoverOperator &&) -> CrossoverOperator & = default;

  [[nodiscard]] virtual auto crossover(RandomEngine &re, I const &p1, I const &p2) -> I = 0;
};

template <Individual I>
  requires GeneType<I, float>
class BLXAlpha : public CrossoverOperator<I> {
 public:
  inline BLXAlpha(float alpha) : alpha_(alpha) {}

  [[nodiscard]] auto crossover(RandomEngine &re, I const &p1, I const &p2) -> I override;

 private:
  float alpha_;
};

template <Individual I>
class PMX : public CrossoverOperator<I> {
 public:
  [[nodiscard]] auto crossover(RandomEngine &re, I const &p1, I const &p2) -> I override;

 private:
  std::unordered_map<GeneT<I>, GeneT<I>> mapping_;
};

/* ------------------------------------- Implementation ------------------------------------- */

template <Individual I>
  requires GeneType<I, float>
[[nodiscard]] auto BLXAlpha<I>::crossover(RandomEngine &re, I const &p1, I const &p2) -> I {
  auto dist = std::uniform_real_distribution<float>(-alpha_, 1.0 + alpha_);
  auto child = p1;

  auto &&p1_genotype = p1.get_genotype();
  auto &&p2_genotype = p2.get_genotype();
  auto &&child_genotype = child.get_mutable_genotype();

  for (const auto &[ga, gb, gr] : std::views::zip(p1_genotype, p2_genotype, child_genotype)) {
    gr = ga + dist(re) * (gb - ga);
  }

  return child;
}

template <Individual I>
auto PMX<I>::crossover(RandomEngine &re, I const &p1, I const &p2) -> I {
  auto child = p1;

  auto &&p1_genotype = p1.get_genotype();
  auto &&p2_genotype = p2.get_genotype();
  auto &&child_genotype = child.get_mutable_genotype();

  assert(p1_genotype.size() == p2_genotype.size());

  auto dist = std::uniform_int_distribution(0UZ, p1_genotype.size() - 1UZ);
  auto i = dist(re);
  auto j = dist(re);
  if (i > j) std::swap(i, j);

  assert(i <= j);

  mapping_.clear();
  for (auto k = i; k <= j; ++k) {
    mapping_.emplace(p1_genotype[k], p2_genotype[k]);
  }

  for (auto k = 0UZ; k < p1_genotype.size(); ++k) {
    // Skip the copied section
    if (k == i) {
      k = j;
      continue;
    }

    auto gene = p2_genotype[k];
    while (mapping_.contains(gene)) {
      gene = mapping_[gene];
    }

    child_genotype[k] = gene;
  }

  return child;
}

}  // namespace meta