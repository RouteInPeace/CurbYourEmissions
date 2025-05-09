#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace serial {

auto readFile(std::filesystem::path path) -> std::optional<std::string>;


}  // namespace cye