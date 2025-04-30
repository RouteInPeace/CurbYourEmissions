#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace cye {

auto readFile(std::filesystem::path path) -> std::optional<std::string>;

}