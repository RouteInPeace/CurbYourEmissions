#include <iostream>
#include <memory>
#include <ostream>
#include "cye/destruction_nn.hpp"
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto solution = cye::nearest_neighbor(instance);

  for (auto node_it = solution.begin(); node_it != solution.end(); ++node_it) std::cout << *node_it << ' ';
  std::cout << '\n';

  auto destruction_nn = cye::DestructionNN(gen);
  auto p = destruction_nn.eval(solution, 0);

  std::cout << p << '\n';
}
