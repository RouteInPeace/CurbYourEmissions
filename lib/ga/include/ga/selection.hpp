#pragma once

#include <ranges>
#include <set>
#include <span>
#include <tuple>
#include "common.hpp"

namespace ga {

template <typename I, typename ValueT>
  requires Individual<I, ValueT>
class SelectionOperator {
 public:
  SelectionOperator() = default;
  virtual ~SelectionOperator();

  SelectionOperator(SelectionOperator const &) = delete;
  auto operator=(SelectionOperator const &) -> SelectionOperator & = delete;

  SelectionOperator(SelectionOperator &&) = default;
  auto operator=(SelectionOperator &&) -> SelectionOperator & = default;

  [[nodiscard]] virtual auto select(RandomEngine &re, std::span<I> population)
      -> std::tuple<size_t, size_t, size_t> = 0;
};

template <typename I, typename ValueT>
  requires Individual<I, ValueT>
class KWayTournamentSelectionOperator : SelectionOperator<I, ValueT> {
 public:
  inline KWayTournamentSelectionOperator(size_t k) : k_(k) {};

  [[nodiscard]] auto select(RandomEngine &re, std::span<I> population) -> std::tuple<size_t, size_t, size_t> override;

 private:
  size_t k_;
};

/* ------------------------------------- Implementation ------------------------------------- */
template <typename I, typename ValueT>
  requires Individual<I, ValueT>
[[nodiscard]] auto KWayTournamentSelectionOperator<I, ValueT>::select(RandomEngine &re, std::span<I> population)
    -> std::tuple<size_t, size_t, size_t> {
  auto comparator = [&population](size_t i, size_t j) { return population[i].fitness() < population[j].fitness(); };

  auto set = std::set<size_t, decltype(comparator)>(comparator);
  for(auto i : std::views::iota(0UZ, k_)) {
    set.insert(i);
  }

  for(auto i = k_; i < population.size(); ++i) {
    set.insert(i);
    set.erase(std::prev(set.end()));
  }

  return {*set.begin(), *std::next(set.begin()), *std::prev(set.end())};
}

}  // namespace ga