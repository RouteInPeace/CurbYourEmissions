#pragma once

#include <random>
#include "common.hpp"

namespace meta {

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
  requires GeneType<I, float>
class GaussianMutation : public MutationOperator<I> {
 public:
  inline GaussianMutation(float sigma) : sigma_(sigma) {}

  [[nodiscard]] auto mutate(RandomEngine &re, I &&individual) -> I override;

 private:
  float sigma_;
};

template <Individual I>
class TwoOpt : public MutationOperator<I> {
 public:
  [[nodiscard]] auto mutate(RandomEngine &gen, I &&individual) -> I override;
};

/* ------------------------------------- Implementation ------------------------------------- */

template <Individual I>
  requires GeneType<I, float>
[[nodiscard]] auto GaussianMutation<I>::mutate(RandomEngine &re, I &&individual) -> I {
  auto dist = std::normal_distribution<float>(0.0, sigma_);

  for (auto &g : individual.get_mutable_genotype()) {
    g += dist(re);
  }

  return individual;
}

template <Individual I>
auto TwoOpt<I>::mutate(RandomEngine &gen, I &&individual) -> I {
  auto &&genotype = individual.get_mutable_genotype();

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

}  // namespace meta