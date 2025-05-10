#include "cye/utils.hpp"

#include <cmath>
#include <fstream>

auto cye::readFile(std::filesystem::path path) -> std::optional<std::string> {
  std::ifstream f(path);

  if (!f.is_open()) {
    return {};
  }

  f.seekg(0, std::ios::end);
  auto size = f.tellg();
  std::string buffer(size, ' ');
  f.seekg(0);
  f.read(&buffer[0], size);

  return buffer;
}

auto cye::cartesian_to_polar(float x, float y) -> std::pair<float, float> {
  double r = std::sqrt(x * x + y * y);
  double theta = std::atan2(y, x);
  return {r, theta};
}
