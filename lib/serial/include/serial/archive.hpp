#pragma once

#include <algorithm>
#include <string_view>

namespace serial {

template <typename T>
concept Value = requires(T t) {
  { t.template get<int>() } -> std::same_as<int>;
  { t.template get_or<int>(0) } -> std::same_as<int>;
  { t.emplace("tmp") } -> std::same_as<T>;
  { t.emplace("tmp", 0) } -> std::same_as<void>;
};

template <typename Value, typename T>
concept HasGet = requires(Value value) {
  { value.template get<T>() };
};

template <typename Value, typename T>
concept HasLoadConstructor = requires(Value value) { T(std::move(value)); };

template <typename Value, typename T>
concept HasWriteFunction = requires(T t, Value v) {
  { t.write(v) } -> std::same_as<void>;
};

template <typename Value, typename T>
concept HasLoader = HasGet<Value, T> || HasLoadConstructor<Value, T>;

template <typename T>
concept StringLike =
    requires(T s) { requires std::convertible_to<T, std::string_view> || std::is_convertible_v<T, const char *>; };

}  // namespace serial