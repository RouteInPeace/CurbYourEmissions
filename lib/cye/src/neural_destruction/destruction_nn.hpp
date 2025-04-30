#pragma once

#include <Eigen/Dense>
#include <array>
#include <cstddef>
#include <random>
#include "core/solution.hpp"

namespace cye {

class DestructionNN {
 public:
  constexpr static size_t GENE_CNT = 10;
  constexpr static size_t NODE_FEATURE_CNT = 8;

  DestructionNN(std::mt19937 &gen);

  [[nodiscard]] auto eval(const Solution &solution, size_t node_id) -> float;

 private:
  std::array<float, GENE_CNT> genotype_;

  Eigen::Map<Eigen::Matrix<float, 2, 3, Eigen::RowMajor>> w1_;
};

}  // namespace cye