#pragma once

#include <random>
#include "meta/common.hpp"
#include "meta/ga/common.hpp"

namespace meta::ga {

template <Individual I>
class MutationOperator {
 public:
  MutationOperator() = default;
  virtual ~MutationOperator() = default;

  MutationOperator(MutationOperator const &) = delete;
  auto operator=(MutationOperator const &) -> MutationOperator & = delete;

  MutationOperator(MutationOperator &&) = default;
  auto operator=(MutationOperator &&) -> MutationOperator & = default;

  [[nodiscard]] virtual auto mutate(RandomEngine &re, I &&individual) -> I = 0;
};

template <Individual I>
  requires GeneType<I, double>
class GaussianMutation : public MutationOperator<I> {
 public:
  inline GaussianMutation(double sigma) : sigma_(sigma) {}

  [[nodiscard]] auto mutate(RandomEngine &re, I &&individual) -> I override;

 private:
  double sigma_;
};

template <Individual I>
class NoMutation : public MutationOperator<I> {
 public:
  [[nodiscard]] inline auto mutate(RandomEngine & /*gen*/, I &&individual) -> I override { return individual; }
};

template <Individual I>
class TwoOpt : public MutationOperator<I> {
 public:
  [[nodiscard]] auto mutate(RandomEngine &gen, I &&individual) -> I override;
};

template <Individual I>
class Swap : public MutationOperator<I> {
 public:
  [[nodiscard]] auto mutate(RandomEngine &gen, I &&individual) -> I override;
};

/* ------------------------------------- Implementation ------------------------------------- */

template <Individual I>
  requires GeneType<I, double>
[[nodiscard]] auto GaussianMutation<I>::mutate(RandomEngine &re, I &&individual) -> I {
  auto dist = std::normal_distribution<double>(0.0, sigma_);

  for (auto &g : individual.genotype()) {
    g += dist(re);
  }

  return individual;
}

template <Individual I>
auto TwoOpt<I>::mutate(RandomEngine &gen, I &&individual) -> I {
  auto &&genotype = individual.genotype();

  auto dist = std::uniform_int_distribution(0UZ, genotype.size() - 1UZ);
  auto i = dist(gen);
  auto j = dist(gen);
  if (i > j) std::swap(i, j);
  assert(i <= j);

  for (auto k = 0UZ; k <= (j - i) / 2; ++k) {
    std::swap(genotype[i + k], genotype[j - k]);
  }

  return individual;
}

template <Individual I>
auto Swap<I>::mutate(RandomEngine &gen, I &&individual) -> I {
  auto &&genotype = individual.genotype();

  auto dist = std::uniform_int_distribution(0UZ, genotype.size() - 1UZ);
  auto i = dist(gen);
  auto j = dist(gen);

  std::swap(genotype[i], genotype[j]);

  return individual;
}

}  // namespace meta::ga