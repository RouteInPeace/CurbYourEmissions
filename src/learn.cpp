#include <iostream>
#include <memory>
#include <ostream>
#include "core/instance.hpp"
#include "heuristics/destruction_nn.hpp"
#include "heuristics/init_heuristics.hpp"

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  auto solution = cye::nearest_neighbor(instance);

  for(auto node_ind : solution.routes()) std::cout << node_ind << ' ';
  std::cout << '\n';

  auto destruction_nn = cye::DestructionNN(gen);
  auto p = destruction_nn.eval(solution, 0);

  std::cout << p << '\n';
}