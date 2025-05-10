#pragma once

#include <concepts>
#include <iterator>
#include <random>

using RandomEngine = std::mt19937;

template <typename I, typename ValueT>
concept Individual = requires(I individual) {
  requires std::ranges::range<decltype(individual.get_genotype())>;
  { *std::ranges::begin(individual.get_genotype()) } -> std::convertible_to<ValueT>;
  requires std::ranges::range<decltype(individual.get_mutable_genotype())>;
  { *std::ranges::begin(individual.get_mutable_genotype()) } -> std::convertible_to<ValueT>;
  { individual.fitness() } -> std::convertible_to<float>;
  { individual.update_fitness() } -> std::same_as<void>;
};