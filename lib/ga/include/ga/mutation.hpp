#pragma once

#include <random>
#include "common.hpp"

namespace ga {

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

}  // namespace ga