#pragma once

#include "meta/common.hpp"
#include "meta/ga/common.hpp"

namespace meta::ga {

template <Individual I>
class LocalSearch {
 public:
  LocalSearch() = default;
  virtual ~LocalSearch() = default;

  LocalSearch(LocalSearch const &) = delete;
  auto operator=(LocalSearch const &) -> LocalSearch & = delete;

  LocalSearch(LocalSearch &&) = default;
  auto operator=(LocalSearch &&) -> LocalSearch & = default;

  [[nodiscard]] virtual auto search(RandomEngine &gen, I &&individual) -> I = 0;
};

template <Individual I>
class NoSearch : public LocalSearch<I> {
 public:
  [[nodiscard]] auto search(RandomEngine & /*gen*/, I &&individual) -> I override {
    individual.update_cost();
    return individual;
  }
};

}  // namespace meta::ga