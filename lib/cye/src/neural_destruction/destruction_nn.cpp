#include "destruction_nn.hpp"
#include <iostream>
#include <ranges>

cye::DestructionNN::DestructionNN(std::mt19937 &gen) : w1_(genotype_.data()) {
  std::normal_distribution<float> norm_dist(0.f, 1.f);
  for (size_t i = 0; i < GENE_CNT; i++) {
    genotype_[i] = norm_dist(gen);
  }
}

auto cye::DestructionNN::eval(const Solution &solution, size_t node_id) -> float {
  auto instance = solution.instance();

  auto input_mat = Eigen::Matrix<float, Eigen::Dynamic, NODE_FEATURE_CNT>(instance->node_cnt(), NODE_FEATURE_CNT);

  auto route_id = -1.f;
  auto in_route_ind = 0.f;

  for (const auto [idx, node] : std::views::enumerate(instance->nodes())) {
    if (node.type == NodeType::Depot) {
      route_id += 1.f;
      in_route_ind = 0.f;
    }

    input_mat(idx, 0) = node.x;
    input_mat(idx, 1) = node.y;
    input_mat(idx, 2) = node.type == NodeType::Depot ? 1.f : 0.f;
    input_mat(idx, 3) = node.type == NodeType::Customer ? 1.f : 0.f;
    input_mat(idx, 4) = node.type == NodeType::ChargingStation ? 1.f : 0.f;
    input_mat(idx, 5) = node.demand;
    input_mat(idx, 6) = route_id;
    input_mat(idx, 7) = in_route_ind;

    in_route_ind += 1.f;
  }

  std::cout << input_mat << '\n';
}
