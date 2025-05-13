#include "serial/utils.hpp"

#include <fstream>

auto serial::readFile(std::filesystem::path path) -> std::optional<std::string> {
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
