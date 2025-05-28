#pragma once

#include <cstddef>
#include <random>
#include <set>
#include <span>
#include <tuple>
#include "meta/common.hpp"
#include "meta/ga/common.hpp"

namespace meta::ga {

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
  inline KWayTournamentSelectionOperator(size_t k) : set_(cmp_), k_(k) {};

  [[nodiscard]] auto select(RandomEngine &re, std::span<I> population) -> std::tuple<size_t, size_t, size_t> override;

 private:
  constexpr static auto cmp_ = [](std::pair<size_t, float> const &a, std::pair<size_t, float> const &b) {
    if (a.second == b.second) {
      return a.first < b.first;
    }

    return a.second < b.second;
  };
  std::set<std::pair<size_t, float>, decltype(cmp_)> set_;

  size_t k_;
};

/* ------------------------------------- Implementation ------------------------------------- */
template <Individual I>
[[nodiscard]] auto KWayTournamentSelectionOperator<I>::select(RandomEngine &re, std::span<I> population)
    -> std::tuple<size_t, size_t, size_t> {
  auto dist = std::uniform_int_distribution<size_t>(0UZ, population.size() - 1);

  set_.clear();
  while (set_.size() < k_) {
    auto ind = dist(re);
    float fitness = population[ind].fitness();

    set_.insert({ind, fitness});
  }

  return {set_.begin()->first, std::next(set_.begin())->first, std::prev(set_.end())->first};
}

}  // namespace meta::ga