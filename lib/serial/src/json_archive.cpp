#include "json_archive.hpp"
#include <cstddef>
#include <string_view>
#include <vector>
#include "archive.hpp"
#include "utils.hpp"

serial::JSONArchive::JSONArchive(std::filesystem::path path) {
  auto data = readFile(path);
  if (!data) {
    throw std::runtime_error(std::format("Could not open file {}", path.c_str()));
  }

  auto document = std::make_shared<rapidjson::Document>();
  document->Parse(data->c_str());

  value_ = document.get();
}

serial::JSONArchive::JSONArchive(std::shared_ptr<rapidjson::Document> document, rapidjson::Value *value)
    : document_(document), value_(value) {}

auto serial::JSONArchive::operator[](std::string_view name) -> JSONArchive {
  return JSONArchive(document_, &(*value_)[name.data()]);
}

template <>
auto serial::JSONArchive::get<float>(std::string_view name) -> float {
  return (*value_)[name.data()].GetFloat();
}

template <>
auto serial::JSONArchive::get<std::string_view>(std::string_view name) -> std::string_view {
  return (*value_)[name.data()].GetString();
}

template <>
auto serial::JSONArchive::get<size_t>(std::string_view name) -> size_t {
  if constexpr (sizeof(size_t) == 8) {
    return (*value_)[name.data()].GetUint64();
  } else {
    return (*value_)[name.data()].GetUint();
  }
}


template <typename T> requires std::is_same_v<T, std::vector<typename T::value_type>>
auto serial::JSONArchive::get(std::string_view name) -> T {
  using ElementType = typename T::value_type;

  static_assert(HasLoader<JSONArchive, ElementType>, "vector element is not loadable by this archive.");

  auto array = (*value_)[name.data()].GetArray();
  auto vec = T();
  vec.reserve(array.Size());

  if constexpr (HasGet<JSONArchive, ElementType>) {
    for (const auto &x : array) {
      vec.push_back(get<ElementType>())
    }
  }

}