#pragma once

#include <algorithm>

namespace serial {

template <typename Value, typename T>
concept HasGet = requires(Value value) {
  { value.template get<T>() };
};

template <typename Value, typename T>
concept HasLoadConstructor = requires(Value value) { T(std::move(value)); };

template <typename Value, typename T>
concept HasLoader = HasGet<Value, T> || HasLoadConstructor<Value, T>;

}  // namespace serial