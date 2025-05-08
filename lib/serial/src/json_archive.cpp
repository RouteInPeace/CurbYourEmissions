#include "json_archive.hpp"
#include <stdexcept>
#include <string_view>
#include "utils.hpp"

serial::JSONArchive::JSONArchive(std::filesystem::path path) {
  auto data = readFile(path);
  if (!data) {
    throw std::runtime_error(std::format("Could not open file {}", path.c_str()));
  }

  document_.Parse(data->c_str());
}

serial::JSONArchive::Value::Value(rapidjson::Value &value) : value_(value) {}

auto serial::JSONArchive::Value::operator[](std::string_view name) -> Value { return Value(value_[name.data()]); }

template <>
auto serial::JSONArchive::Value::get<float>() -> float {
  if (!value_.IsFloat()) throw std::runtime_error("Value is not a float.");
  return value_.GetFloat();
}

template <>
auto serial::JSONArchive::Value::get<std::string_view>() -> std::string_view {
  if (!value_.IsString()) throw std::runtime_error("Value is not a string.");
  return std::string_view(value_.GetString(), value_.GetStringLength());
}

template <>
auto serial::JSONArchive::Value::get<size_t>() -> size_t {
  if constexpr (sizeof(size_t) == 8) {
    if (!value_.IsUint64()) throw std::runtime_error("Value is not an unsigned integer.");
    return value_.GetUint64();
  } else {
    if (!value_.IsUint()) throw std::runtime_error("Value is not an unsigned integer.");
    return value_.GetUint();
  }
}

template <>
auto serial::JSONArchive::Value::get_or<float>(float v) -> float {
  return value_.IsFloat() ? value_.GetFloat() : v;
}

template <>
auto serial::JSONArchive::Value::get_or<std::string_view>(std::string_view v) -> std::string_view {
  return value_.IsString() ? std::string_view(value_.GetString(), value_.GetStringLength()) : v;
}

template <>
auto serial::JSONArchive::Value::get_or<size_t>(size_t v) -> size_t {
  if constexpr (sizeof(size_t) == 8) {
    return value_.IsUint64() ? value_.GetUint64() : v;
  } else {
    return value_.IsUint() ? value_.GetUint() : v;
  }
}
