#pragma once

#include <Eigen/Dense>
#include <array>
#include <cstddef>
#include <random>
#include "solution.hpp"

namespace cye {

class DestructionNN {
 public:
  constexpr static size_t NODE_FEATURE_CNT = 8;
  constexpr static size_t NODE_EMBEDDING_DIM = 32;
  constexpr static size_t HIDDEN_DIM1 = 16;

  enum Feature : size_t { DIST, THETA, I_D, I_C, I_CS, DEMAND, SAME_ROUTE, PROGRESS };

  DestructionNN(std::mt19937 &gen);

  [[nodiscard]] auto eval(const Solution &solution, size_t node_id) const -> float;

 private:
  [[nodiscard]] auto construct_input_mat_(const Solution &solution, size_t ind_in_route) const
      -> Eigen::Matrix<float, Eigen::Dynamic, NODE_FEATURE_CNT>;

  constexpr static size_t w1_offset_ = 0;
  constexpr static size_t a1_offset_ = w1_offset_ + NODE_FEATURE_CNT * NODE_EMBEDDING_DIM;
  constexpr static size_t w2_offset_ = a1_offset_ + NODE_EMBEDDING_DIM;
  constexpr static size_t a2_offset_ = w2_offset_ + NODE_EMBEDDING_DIM * HIDDEN_DIM1;
  constexpr static size_t param_cnt_ = a2_offset_ + HIDDEN_DIM1;

  std::array<float, param_cnt_> genotype_;

  Eigen::Map<Eigen::Matrix<float, NODE_FEATURE_CNT, NODE_EMBEDDING_DIM, Eigen::RowMajor>> w1_;
  Eigen::Map<Eigen::Vector<float, NODE_EMBEDDING_DIM>> a1_;
  Eigen::Map<Eigen::Matrix<float, NODE_EMBEDDING_DIM, HIDDEN_DIM1, Eigen::RowMajor>> w2_;
  Eigen::Map<Eigen::Vector<float, HIDDEN_DIM1>> a2_;
};

}  // namespace cye