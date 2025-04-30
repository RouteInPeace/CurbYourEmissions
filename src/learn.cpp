#include <iostream>
#include <memory>
#include "core/instance.hpp"
#include "heuristics/init_heuristics.hpp"

auto main() -> int {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");

  size_t i = 0;
  for (const auto &node : instance->nodes()) {
    std::cout << i++ << ' ' << node.x << ' ' << node.y << ' ' << static_cast<int>(node.type) << '\n';
  }

  std::cout << instance->node_cnt() << '\n';

  for (auto i : instance->customer_ids()) std::cout << i << ' ';
  std::cout << '\n';
  for (auto i : instance->charging_station_ids()) std::cout << i << ' ';
  std::cout << '\n';

  auto solution = cye::nearest_neighbor(instance);
}