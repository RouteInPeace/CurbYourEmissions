#pragma once

#include <functional>
#include <optional>
#include <print>
#include "common.hpp"
namespace meta {

template <typename Solution>
struct Config {
  Config(Solution &&init) : initial_solution(std::move(init)) {}

  Solution initial_solution;
  std::function<Solution(RandomEngine &gen, Solution const &)> get_neighbour;
  std::function<std::optional<double>(size_t)> get_temperature;
  bool verbose;
};

inline auto create_linear_schedule(size_t max_iter, double initial_temp, double final_temp) {
  return [max_iter, initial_temp, final_temp](size_t iter) -> std::optional<double> {
    if (iter > max_iter) return {};
    return {initial_temp + (final_temp - initial_temp) * static_cast<double>(iter) / static_cast<double>(max_iter)};
  };
}

inline auto create_geometric_schedule(double initial_temp, double final_temp, double alpha) {
  auto temp = initial_temp;
  return [temp, final_temp, alpha](size_t /*iter*/) mutable -> std::optional<double> {
    temp *= alpha;
    if (temp < final_temp) return {};
    return {temp};
  };
}

template <typename Solution>
auto optimize(RandomEngine &gen, Config<Solution> const &config) -> Solution {
  auto current_solution = std::move(config.initial_solution);
  double current_cost = current_solution.get_cost();

  auto best_solution = current_solution;
  auto best_cost = current_cost;

  auto dist = std::uniform_real_distribution<double>(0.0, 1.0);

  for (auto iter = 0UZ;; ++iter) {
    auto temp_opt = config.get_temperature(iter);
    if (!temp_opt) {
      break;
    }
    auto temp = *temp_opt;

    auto new_solution = config.get_neighbour(gen, current_solution);
    auto new_solution_cost = new_solution.get_cost();
    double cost_diff = new_solution_cost - current_cost;

    if (cost_diff < 0 || dist(gen) < exp(-cost_diff / temp)) {
      current_solution = std::move(new_solution);
      current_cost = new_solution_cost;
      if (current_cost < best_cost) {
        best_solution = current_solution;
        best_cost = current_cost;
      }
    }

    if (iter % 10 == 0) {
      std::println("Iter: {}, Current cost: {}, Best cost: {}, Temp: {}", iter, current_cost, best_cost, temp);
    }
  }

  return best_solution;
}

}  // namespace meta