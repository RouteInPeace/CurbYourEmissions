#include "serial/json_archive.hpp"

#include <format>
#include <stdexcept>
#include "serial/utils.hpp"

serial::JSONArchive::JSONArchive(std::filesystem::path path) {
  auto data = readFile(path);
  if (!data) {
    throw std::runtime_error(std::format("Could not open file {}", path.c_str()));
  }

  document_.Parse(data->c_str());
}