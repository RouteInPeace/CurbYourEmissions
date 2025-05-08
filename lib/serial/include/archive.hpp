#pragma once

#include <algorithm>

namespace serial {

template <typename Archive, typename T>
concept HasGet = requires(Archive archive) {
  { archive.template get<T>("key") };
};

template <typename Archive, typename T>
concept HasLoadConstructor = requires(Archive archive) { T(std::move(archive)); };

template <typename Archive, typename T>
concept HasLoader = HasGet<Archive, T> || HasLoadConstructor<Archive, T>;

}  // namespace serial