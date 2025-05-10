#pragma once

#include <random>
#include "common.hpp"

namespace ga {

template <typename I, typename ValueT>
  requires Individual<I, ValueT>
class MutationOperator {
 public:
  MutationOperator() = default;
  virtual ~MutationOperator();

  MutationOperator(MutationOperator const &) = delete;
  auto operator=(MutationOperator const &) -> MutationOperator & = delete;

  MutationOperator(MutationOperator &&) = default;
  auto operator=(MutationOperator &&) -> MutationOperator & = default;

  [[nodiscard]] virtual auto mutate(RandomEngine &re, I &&individual) -> I = 0;
};

template <Individual<float> I>
class GaussianMutation : public MutationOperator<I, float> {
 public:
  inline GaussianMutation(float sigma) : sigma_(sigma) {}

  [[nodiscard]] auto mutate(RandomEngine &re, I &&individual) -> I override;

 private:
  float sigma_;
};

/* ------------------------------------- Implementation ------------------------------------- */
template <Individual<float> I>
[[nodiscard]] auto GaussianMutation<I>::mutate(RandomEngine &re, I &&individual) -> I {
  auto dist = std::normal_distribution<float>(0.0, sigma_);

  for (auto &g : individual.get_genotype()) {
    g += dist(re);
  }
}

}  // namespace ga