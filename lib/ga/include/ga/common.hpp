#pragma once

#include <concepts>
#include <iterator>
#include <random>

using RandomEngine = std::mt19937;

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
