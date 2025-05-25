#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  std::cout << solution.is_valid() << ' ' << solution.cost() << '\n';
}