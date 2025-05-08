#pragma once

#include <filesystem>
#include <vector>
#include "archive.hpp"
#include "rapidjson/document.h"

namespace serial {

class JSONArchive {
 public:
  JSONArchive(std::filesystem::path path);

  class Value {
   public:
    auto operator[](std::string_view name) -> Value;

    template <typename T>
    auto get() -> T = delete;

    template <>
    auto get<float>() -> float;

    template <>
    auto get<size_t>() -> size_t;

    template <>
    auto get<std::string_view>() -> std::string_view;

    template <typename T>
      requires std::is_same_v<T, std::vector<typename T::value_type>>
    auto get() -> T;

    template <typename T>
    auto get_or(T) -> T;

    friend class JSONArchive;

   private:
    Value(rapidjson::Value &value);
    rapidjson::Value &value_;
  };

  inline operator Value() { return Value(document_); }

 private:
  rapidjson::Document document_;
};

template <typename T>
  requires std::is_same_v<T, std::vector<typename T::value_type>>
auto JSONArchive::Value::get() -> T {
  using ElementType = typename T::value_type;

  static_assert(HasLoader<Value, ElementType>, "Vector element is not loadable by this archive.");

  auto array = value_.GetArray();
  auto vec = T();
  vec.reserve(array.Size());

  if constexpr (HasGet<Value, ElementType>) {
    for (auto &x : array) {
      vec.push_back(Value(x).get<ElementType>());
    }
  } else {
    for (auto &x : array) {
      vec.push_back(ElementType(Value(x)));
    }
  }

  return vec;
}

}  // namespace serial