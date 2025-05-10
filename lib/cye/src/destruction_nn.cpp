#include "cye/destruction_nn.hpp"

#include <iostream>
#include "cye/utils.hpp"

cye::DestructionNN::DestructionNN(std::mt19937 &gen)
    : w1_(genotype_.data() + w1_offset_),
      a1_(genotype_.data() + a1_offset_),
      w2_(genotype_.data() + w2_offset_),
      a2_(genotype_.data() + a2_offset_) {
  std::normal_distribution<float> norm_dist(0.f, 1.f);
  for (size_t i = 0; i < param_cnt_; i++) {
    genotype_[i] = norm_dist(gen);
  }
  std::cout << param_cnt_ << '\n';
}

auto cye::DestructionNN::eval(const Solution &solution, size_t node_id) const -> float {
  auto input_mat = construct_input_mat_(solution, node_id);

  // Compute node embeddings
  auto node_embeddings = input_mat * w1_;

  // Compute attention
  auto h1 = node_embeddings.unaryExpr([](auto x) { return std::max(0.f, x); });
  auto weights = h1 * a1_;
  auto attention = softmax(weights);

  // Compute feature vector
  auto weighted_embeddings = node_embeddings.array().colwise() * attention.array();
  Eigen::RowVector<float, NODE_EMBEDDING_DIM> graph_features = weighted_embeddings.colwise().sum();

  // Fully connected layer
  auto h2 = graph_features * w2_;
  float h3 = h2 * a2_;

  return sigmoid(h3);
}

auto cye::DestructionNN::construct_input_mat_(const Solution &solution, size_t ind_in_route) const
    -> Eigen::Matrix<float, Eigen::Dynamic, NODE_FEATURE_CNT> {
  auto &instance = solution.instance();
  auto &query_node = instance.node(solution.routes()[ind_in_route]);

  auto input_mat =
      Eigen::Matrix<float, Eigen::Dynamic, NODE_FEATURE_CNT>(solution.visited_node_cnt(), NODE_FEATURE_CNT);

  auto route_id = -1.f;
  auto in_route_ind = 0.f;

  for (const auto [idx, node_id] : std::views::enumerate(solution.routes())) {
    auto &node = instance.node(node_id);
    if (node.type == NodeType::Depot) {
      route_id += 1.f;
      in_route_ind = 0.f;
    }

    auto [r, theta] = cye::cartesian_to_polar(node.x - query_node.x, node.y - query_node.y);
    input_mat(idx, DIST) = r / instance.max_range();
    input_mat(idx, THETA) = theta;
    input_mat(idx, I_D) = node.type == NodeType::Depot ? 1.f : 0.f;
    input_mat(idx, I_C) = node.type == NodeType::Customer ? 1.f : 0.f;
    input_mat(idx, I_CS) = node.type == NodeType::ChargingStation ? 1.f : 0.f;
    input_mat(idx, DEMAND) = node.demand / instance.max_cargo_capacity();
    input_mat(idx, SAME_ROUTE) = route_id;
    input_mat(idx, PROGRESS) = in_route_ind;

    in_route_ind += 1.f;
  }

  auto query_node_route_id = input_mat(ind_in_route, SAME_ROUTE);
  auto norm = 1.f;
  for (auto i = solution.visited_node_cnt(); i--;) {
    input_mat(i, SAME_ROUTE) = input_mat(i, SAME_ROUTE) == query_node_route_id ? 1.f : 0.f;
    input_mat(i, PROGRESS) /= norm;

    if (i > 0 && input_mat(i, I_D) == 1.f) {
      norm = input_mat(i - 1, PROGRESS);
    }
  }

  return input_mat;
}