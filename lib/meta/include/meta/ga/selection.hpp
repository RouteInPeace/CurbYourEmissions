#pragma once

#include <algorithm>
#include <cstddef>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "meta/common.hpp"
#include "meta/ga/common.hpp"

namespace meta::ga {

template <typename I>
class SSGASelectionOperator {
 public:
  SSGASelectionOperator() = default;
  virtual ~SSGASelectionOperator() = default;

  SSGASelectionOperator(SSGASelectionOperator const &) = delete;
  auto operator=(SSGASelectionOperator const &) -> SSGASelectionOperator & = delete;

  SSGASelectionOperator(SSGASelectionOperator &&) = default;
  auto operator=(SSGASelectionOperator &&) -> SSGASelectionOperator & = default;

  [[nodiscard]] virtual auto select(RandomEngine &gen, std::span<I> population)
      -> std::tuple<size_t, size_t, size_t> = 0;
};

template <typename I>
class GenGASelectionOperator {
 public:
  GenGASelectionOperator() = default;
  virtual ~GenGASelectionOperator() = default;

  GenGASelectionOperator(GenGASelectionOperator const &) = delete;
  auto operator=(GenGASelectionOperator const &) -> GenGASelectionOperator & = delete;

  GenGASelectionOperator(GenGASelectionOperator &&) = default;
  auto operator=(GenGASelectionOperator &&) -> GenGASelectionOperator & = default;

  virtual auto prepare(std::span<I> population) -> void = 0;
  [[nodiscard]] virtual auto select(RandomEngine &gen) -> std::pair<size_t, size_t> = 0;
};

template <Individual I>
class KWayTournamentSelectionOperator : public SSGASelectionOperator<I> {
 public:
  inline KWayTournamentSelectionOperator(size_t k) : set_(cmp_), k_(k) {};

  [[nodiscard]] auto select(RandomEngine &re, std::span<I> population) -> std::tuple<size_t, size_t, size_t> override {
    auto dist = std::uniform_int_distribution<size_t>(0UZ, population.size() - 1);

    set_.clear();
    while (set_.size() < k_) {
      auto ind = dist(re);
      float cost = population[ind].cost();

      set_.insert({ind, cost});
    }

    return {set_.begin()->first, std::next(set_.begin())->first, std::prev(set_.end())->first};
  }

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

template <typename I>
class RouletteWheelSelection : public GenGASelectionOperator<I> {
 public:
  auto prepare(std::span<I> population) -> void override {
    dist_.resize(population.size());

    for (auto i = 0UZ; i < population.size(); ++i) {
      dist_[i] = i == 0 ? 0.0 : dist_[i - 1];
      dist_[i] += population.back().cost() - population[i].cost();
    }
  }

  [[nodiscard]] auto select(RandomEngine &gen) -> std::pair<size_t, size_t> override {
    auto p_dist = std::uniform_real_distribution(0.0, dist_.back());
    auto p1 = p_dist(gen);
    auto p2 = p_dist(gen);

    auto parent1 = std::ranges::lower_bound(dist_, p1) - dist_.begin();
    auto parent2 = std::ranges::lower_bound(dist_, p2) - dist_.begin();

    return {parent1, parent2};
  }

 private:
  std::vector<double> dist_;
};

template <typename I>
class RankSelection : public GenGASelectionOperator<I> {
 public:
  RankSelection(double sp) : sp_(sp) {
    if (sp < 1 || sp > 2) {
      throw std::runtime_error("Selectio pressure has to be between 1 and 2.");
    }
  }

  auto prepare(std::span<I> population) -> void override {
    dist_.resize(population.size());

    auto c = 2.0 * (sp_ - 1.0) / static_cast<double>(population.size() - 1);

    for (auto i = 0UZ; i < population.size(); ++i) {
      dist_[i] = i == 0 ? 0.0 : dist_[i - 1];
      dist_[i] += 2.0 - sp_ + c * static_cast<double>(population.size() - i - 1);
    }
  }

  [[nodiscard]] auto select(RandomEngine &gen) -> std::pair<size_t, size_t> override {
    auto p_dist = std::uniform_real_distribution(0.0, dist_.back());
    auto p1 = p_dist(gen);
    auto p2 = p_dist(gen);

    auto parent1 = std::ranges::lower_bound(dist_, p1) - dist_.begin();
    auto parent2 = std::ranges::lower_bound(dist_, p2) - dist_.begin();

    return {parent1, parent2};
  }

 private:
  double sp_;
  std::vector<double> dist_;
};
}  // namespace meta::ga