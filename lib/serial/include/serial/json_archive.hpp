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
    constexpr auto operator[](std::string_view name) -> Value {
      return value_->HasMember(name.data()) ? Value(&(*value_)[name.data()]) : nullptr;
    };

    template <typename T>
    [[nodiscard]] constexpr auto get() const -> T = delete;

    template <>
    [[nodiscard]] constexpr auto get<int>() const -> int;

    template <>
    [[nodiscard]] constexpr auto get<float>() const -> float;

    template <>
    [[nodiscard]] constexpr auto get<size_t>() const -> size_t;

    template <>
    [[nodiscard]] constexpr auto get<std::string_view>() const -> std::string_view;

    template <typename T>
      requires std::is_same_v<T, std::vector<typename T::value_type>>
    [[nodiscard]] constexpr auto get() const -> T;

    template <typename T>
    [[nodiscard]] constexpr auto get_or(T) const noexcept -> T;

    friend class JSONArchive;

   private:
    constexpr Value(rapidjson::Value *value) noexcept : value_(value) {};
    rapidjson::Value *value_;
  };

  [[nodiscard]] inline auto root() { return Value(&document_); }

 private:
  rapidjson::Document document_;
};

/* -------------------------------------------------------------------------------------------- */
/* ------------------------------------------ Detail ------------------------------------------ */
/* -------------------------------------------------------------------------------------------- */

template <>
constexpr auto serial::JSONArchive::Value::get<int>() const -> int {
  if (value_ == nullptr || !value_->IsInt()) throw std::runtime_error("Value is not an int.");
  return value_->GetInt();
}

template <>
constexpr auto serial::JSONArchive::Value::get<float>() const -> float {
  if (value_ == nullptr || !value_->IsFloat()) throw std::runtime_error("Value is not a float.");
  return value_->GetFloat();
}

template <>
constexpr auto serial::JSONArchive::Value::get<std::string_view>() const -> std::string_view {
  if (value_ == nullptr || !value_->IsString()) throw std::runtime_error("Value is not a string.");
  return std::string_view(value_->GetString(), value_->GetStringLength());
}

template <>
constexpr auto serial::JSONArchive::Value::get<size_t>() const -> size_t {
  if constexpr (sizeof(size_t) == 8) {
    if (value_ == nullptr || !value_->IsUint64()) throw std::runtime_error("Value is not an unsigned integer.");
    return value_->GetUint64();
  } else {
    if (value_ == nullptr || !value_->IsUint()) throw std::runtime_error("Value is not an unsigned integer.");
    return value_->GetUint();
  }
}

template <>
constexpr auto serial::JSONArchive::Value::get_or<int>(int v) const noexcept -> int {
  return value_ != nullptr && value_->IsInt() ? value_->GetInt() : v;
}

template <>
constexpr auto serial::JSONArchive::Value::get_or<float>(float v) const noexcept -> float {
  return value_ != nullptr && value_->IsFloat() ? value_->GetFloat() : v;
}

template <>
constexpr auto serial::JSONArchive::Value::get_or<std::string_view>(std::string_view v) const noexcept
    -> std::string_view {
  return value_ != nullptr && value_->IsString() ? std::string_view(value_->GetString(), value_->GetStringLength()) : v;
}

template <>
constexpr auto serial::JSONArchive::Value::get_or<size_t>(size_t v) const noexcept -> size_t {
  if constexpr (sizeof(size_t) == 8) {
    return value_ != nullptr && value_->IsUint64() ? value_->GetUint64() : v;
  } else {
    return value_ != nullptr && value_->IsUint() ? value_->GetUint() : v;
  }
}

template <typename T>
  requires std::is_same_v<T, std::vector<typename T::value_type>>
constexpr auto JSONArchive::Value::get() const -> T {
  using ElementType = typename T::value_type;
  static_assert(HasLoader<Value, ElementType>, "Vector element is not loadable by this archive.");
  if (value_ == nullptr || !value_->IsArray()) throw std::runtime_error("Value is not an array.");

  auto array = value_->GetArray();
  auto vec = T();
  vec.reserve(array.Size());

  if constexpr (HasGet<Value, ElementType>) {
    for (auto &x : array) {
      vec.push_back(Value(&x).get<ElementType>());
    }
  } else {
    for (auto &x : array) {
      vec.push_back(ElementType(Value(&x)));
    }
  }

  return vec;
}

}  // namespace serial
