#pragma once

#include <concepts>
#include <iterator>
#include <random>

using RandomEngine = std::mt19937;

template <typename I, typename ValueT>
concept Individual = requires(I individual) {
  requires std::ranges::range<decltype(individual.get_gentype())>;
  { *std::ranges::begin(individual.get_gentype()) } -> std::convertible_to<ValueT>;
  { individual.fitness() } -> std::convertible_to<float>;
};