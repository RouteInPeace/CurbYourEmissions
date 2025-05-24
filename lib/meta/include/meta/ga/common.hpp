#pragma once

#include <concepts>
#include <iterator>

namespace meta::ga {

template <typename I>
concept Individual = requires(I individual) {
  requires std::ranges::range<decltype(individual.get_genotype())>;
  requires std::ranges::range<decltype(individual.get_mutable_genotype())>;
  { individual.fitness() } -> std::convertible_to<float>;
  { individual.update_fitness() } -> std::same_as<void>;
};

template <typename I, typename T>
concept GeneType = requires(I individual) {
  { *std::ranges::begin(individual.get_genotype()) } -> std::convertible_to<T>;
  { *std::ranges::begin(individual.get_mutable_genotype()) } -> std::convertible_to<T>;
};

template <Individual I>
using GeneT = std::ranges::range_value_t<decltype(std::declval<I>().get_genotype())>;

}  // namespace meta
