#pragma once

#include <ranges>
#include "common.hpp"

namespace meta {

template <Individual I>
class CrossoverOperator {
 public:
  CrossoverOperator() = default;
  virtual ~CrossoverOperator() = default;

  CrossoverOperator(CrossoverOperator const &) = delete;
  auto operator=(CrossoverOperator const &) -> CrossoverOperator & = delete;

  CrossoverOperator(CrossoverOperator &&) = default;
  auto operator=(CrossoverOperator &&) -> CrossoverOperator & = default;

  [[nodiscard]] virtual auto crossover(RandomEngine &re, I const &a, I const &b) -> I = 0;
};

template <Individual I> requires GeneType<I, float>
class BLXAlpha : public CrossoverOperator<I> {
 public:
  inline BLXAlpha(float alpha) : alpha_(alpha) {}

  [[nodiscard]] auto crossover(RandomEngine &re, I const &a, I const &b) -> I override;

 private:
  float alpha_;
};

/* ------------------------------------- Implementation ------------------------------------- */

template <Individual I> requires GeneType<I, float>
[[nodiscard]] auto BLXAlpha<I>::crossover(RandomEngine &re, I const &a, I const &b) -> I {
  auto dist = std::uniform_real_distribution<float>(-alpha_, 1.0 + alpha_);
  auto individual = a;

  auto a_genotype = a.get_genotype();
  auto b_genotype = b.get_genotype();
  auto r_genotype = individual.get_mutable_genotype();

  for (const auto& [ga, gb, gr] : std::views::zip(a_genotype, b_genotype, r_genotype)) {
    gr = ga + dist(re) * (gb - ga);
  }

  return individual;
}

}  // namespace ga