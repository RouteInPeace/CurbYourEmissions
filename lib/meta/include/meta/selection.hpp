#pragma once

#include <cstddef>
#include <random>
#include <set>
#include <span>
#include <tuple>
#include "common.hpp"

namespace meta {

template <typename I>
class SelectionOperator {
 public:
  SelectionOperator() = default;
  virtual ~SelectionOperator() = default;

  SelectionOperator(SelectionOperator const &) = delete;
  auto operator=(SelectionOperator const &) -> SelectionOperator & = delete;

  SelectionOperator(SelectionOperator &&) = default;
  auto operator=(SelectionOperator &&) -> SelectionOperator & = default;

  [[nodiscard]] virtual auto select(RandomEngine &re, std::span<I> population)
      -> std::tuple<size_t, size_t, size_t> = 0;
};

template <Individual I>
class KWayTournamentSelectionOperator : public SelectionOperator<I> {
 public:
  inline KWayTournamentSelectionOperator(size_t k) : k_(k) {};

  [[nodiscard]] auto select(RandomEngine &re, std::span<I> population) -> std::tuple<size_t, size_t, size_t> override;

 private:
  size_t k_;
};

/* ------------------------------------- Implementation ------------------------------------- */
template <Individual I>
[[nodiscard]] auto KWayTournamentSelectionOperator<I>::select(RandomEngine &re, std::span<I> population)
    -> std::tuple<size_t, size_t, size_t> {

  auto dist = std::uniform_int_distribution<size_t>(0, population.size()-1);
  auto comparator = [&population](size_t i, size_t j) { return population[i].fitness() < population[j].fitness(); };

  auto set = std::set<size_t, decltype(comparator)>(comparator);
  for(auto i = 0UZ; i < k_; ++i) {
    set.insert(dist(re));
  }

  return {*set.begin(), *std::next(set.begin()), *std::prev(set.end())};
}

}  // namespace ga