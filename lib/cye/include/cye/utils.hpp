#pragma once

#include <Eigen/Dense>
#include <filesystem>
#include <optional>
#include <string>

namespace cye {

auto readFile(std::filesystem::path path) -> std::optional<std::string>;
auto cartesian_to_polar(float x, float y) -> std::pair<float, float>;

template <typename Derived>
auto softmax(const Eigen::MatrixBase<Derived> &x) {
  const auto max_val = x.maxCoeff();
  const auto exp_shifted = (x.array() - max_val).exp();
  const auto sum_exp = exp_shifted.sum();
  return exp_shifted / sum_exp;
}

constexpr inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

}  // namespace cye