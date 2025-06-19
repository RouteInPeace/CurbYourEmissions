#pragma once

#include <concepts>
#include <iterator>

namespace meta::ga {

template <typename I>
concept Individual = requires(I individual) {
  requires std::ranges::range<decltype(individual.genotype())>;
  { individual.cost() } -> std::convertible_to<double>;
  { individual.update_cost() } -> std::same_as<void>;
  { individual.hash() } -> std::same_as<size_t>;
};

template <typename I, typename T>
concept GeneType = requires(I individual) {
  { *std::ranges::begin(individual.genotype()) } -> std::convertible_to<T>;
};

template <Individual I>
using GeneT = std::ranges::range_value_t<decltype(std::declval<I>().genotype())>;

}  // namespace meta::ga
