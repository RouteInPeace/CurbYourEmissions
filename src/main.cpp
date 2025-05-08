#include <iostream>
#include "core/instance.hpp"
#include "json_archive.hpp"

auto main() -> int { 
  auto archive = serial::JSONArchive("dataset/json/test.json"); 
  auto instance = cye::Instance(archive);
}