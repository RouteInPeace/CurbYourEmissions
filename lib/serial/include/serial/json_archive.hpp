#pragma once

#include <filesystem>
#include <vector>
#include "archive.hpp"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

namespace serial {
class JSONArchive {
 public:
  JSONArchive();
  JSONArchive(std::filesystem::path path);

  class Value {
   public:
    constexpr auto operator[](std::string_view name) -> Value {
      return value_->HasMember(name.data()) ? Value(&(*value_)[name.data()], allocator_) : Value(nullptr, allocator_);
    };

    template <typename T>
    [[nodiscard]] constexpr auto get() const -> T = delete;

    template <typename T>
      requires std::is_same_v<T, std::vector<typename T::value_type>>
    [[nodiscard]] constexpr auto get() const -> T;

    template <typename T>
    [[nodiscard]] constexpr auto get_or(T) const noexcept -> T;

    constexpr auto emplace(std::string_view name) -> Value {
      auto rj_name = rapidjson::Value();
      rj_name.SetString(name.data(), name.length());
      auto v = rapidjson::Value(rapidjson::kObjectType);
      value_->AddMember(rj_name, v, allocator_);

      return Value(&(*value_)[name.data()], allocator_);
    }

    template <std::ranges::input_range R>
      requires(!StringLike<R>)
    constexpr auto emplace(std::string_view name, R &&range) -> void {
      using value_type = std::ranges::range_value_t<R>;

      auto rj_name = rapidjson::Value();
      rj_name.SetString(name.data(), name.length());

      auto array = rapidjson::Value(rapidjson::kArrayType);
      for (auto x : range) {
        if constexpr (HasWriteFunction<Value, value_type>) {
          auto obj = rapidjson::Value(rapidjson::kObjectType);
          x.write(Value(&obj, allocator_));
          array.PushBack(std::move(obj), allocator_);
        } else {
          array.PushBack(x, allocator_);
        }
      }

      value_->AddMember(rj_name, std::move(array), allocator_);
    }

    template <typename T>
      requires(!HasWriteFunction<JSONArchive::Value, T> && (StringLike<T> || !std::ranges::input_range<T>))
    constexpr auto emplace(std::string_view name, T &&v) -> void {
      auto rj_name = rapidjson::Value();
      rj_name.SetString(name.data(), name.length());
      value_->AddMember(rj_name, std::forward<T>(v), allocator_);
    }

    template <typename T>
      requires(HasWriteFunction<JSONArchive::Value, T>)
    constexpr auto emplace(std::string_view name, T&& t) -> void {
      auto rj_name = rapidjson::Value();
      rj_name.SetString(name.data(), name.length());
      auto obj = rapidjson::Value(rapidjson::kObjectType);
      t.write(Value(&obj, allocator_));
      value_->AddMember(rj_name, std::move(obj), allocator_);
    }

    friend class JSONArchive;

   private:
    constexpr Value(rapidjson::Value *value, rapidjson::Document::AllocatorType &allocator) noexcept
        : value_(value), allocator_(allocator) {};
    rapidjson::Value *value_;
    rapidjson::Document::AllocatorType &allocator_;
  };

  [[nodiscard]] inline auto root() { return Value(&document_, document_.GetAllocator()); }
  [[nodiscard]] auto to_string() const -> std::string;

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
      vec.push_back(Value(&x, allocator_).get<ElementType>());
    }
  } else {
    for (auto &x : array) {
      vec.push_back(ElementType(Value(&x, allocator_)));
    }
  }

  return vec;
}

}  // namespace serial