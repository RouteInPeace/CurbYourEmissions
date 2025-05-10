#pragma once

#include <ranges>
#include "common.hpp"

namespace ga {

template <typename I, typename ValueT>
  requires Individual<I, ValueT>
class CrossoverOperator {
 public:
  CrossoverOperator() = default;
  virtual ~CrossoverOperator();

  CrossoverOperator(CrossoverOperator const &) = delete;
  auto operator=(CrossoverOperator const &) -> CrossoverOperator & = delete;

  CrossoverOperator(CrossoverOperator &&) = default;
  auto operator=(CrossoverOperator &&) -> CrossoverOperator & = default;

  [[nodiscard]] virtual auto crossover(RandomEngine &re, I const &a, I const &b) -> I = 0;
};

template <Individual<float> I>
class BLXAlpha : public CrossoverOperator<I, float> {
 public:
  inline BLXAlpha(float alpha) : alpha_(alpha) {}

  [[nodiscard]] auto crossover(RandomEngine &re, I const &a, I const &b) -> I override;

 private:
  float alpha_;
};

/* ------------------------------------- Implementation ------------------------------------- */

template <Individual<float> I>
[[nodiscard]] auto BLXAlpha<I>::crossover(RandomEngine &re, I const &a, I const &b) -> I {
  auto dist = std::uniform_real_distribution<float>(-alpha_, 1.0 + alpha_);
  I individual;

  for (auto &[ga, gb, gr] : std::views::zip(a.get_genotype(), b.get_genotype(), individual.get_genotype())) {
    gr = ga + dist(re) * (gb - ga);
  }

  return individual;
}

}  // namespace ga