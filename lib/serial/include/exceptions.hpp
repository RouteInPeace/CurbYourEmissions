#pragma once

#include <stdexcept>
#include <format>

namespace serial {

class InvalidValue : public std::runtime_error {
 public:
  inline explicit InvalidValue(std::string_view key)
      : std::runtime_error(std::format("Invalid value for key: {}" , key)), key_(key) {}

  [[nodiscard]] inline auto key() const noexcept -> const std::string & { return key_; }

 private:
  std::string key_;
};

}  // namespace serial