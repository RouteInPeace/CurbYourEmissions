#include <iostream>
#include "cye.hpp"

auto main() -> int {
  auto instance = cye::Instance("dataset/json/E-n22-k4.json");
  std::cout << "Hello\n";
}