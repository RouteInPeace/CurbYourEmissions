#include "serial/json_archive.hpp"

#include <format>
#include <stdexcept>
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "serial/utils.hpp"
#include <fstream>

serial::JSONArchive::JSONArchive() : document_(rapidjson::kObjectType) {}

serial::JSONArchive::JSONArchive(std::filesystem::path path) {
  auto data = readFile(path);
  if (!data) {
    throw std::runtime_error(std::format("Could not open file {}", path.c_str()));
  }

  document_.Parse(data->c_str());
}

auto serial::JSONArchive::to_string() const -> std::string {
  std::ostringstream oss;
  rapidjson::OStreamWrapper osw(oss);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
  document_.Accept(writer);
  return oss.str();
}

auto serial::JSONArchive::save(std::filesystem::path path) const -> void {
  std::ofstream ofs(path);
  if (!ofs.is_open()) {
    throw std::runtime_error("Failed to open file for writing");
  }

  rapidjson::OStreamWrapper osw(ofs);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
  document_.Accept(writer);
}