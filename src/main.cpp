#include <iostream>
#include <string_view>
#include "core/instance.hpp"
#include "json_archive.hpp"

auto main() -> int {
  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");

  auto value = static_cast<serial::JSONArchive::Value>(archive);
  auto instance = cye::Instance(value);
}