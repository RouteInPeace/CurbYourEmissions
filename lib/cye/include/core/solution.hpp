#pragma once

#include <memory>
#include <vector>
#include "instance.hpp"

namespace cye {

// TODO: this is only temporary

class Solution {
 public:
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes);

  [[nodiscard]] auto instance() const { return instance_; }
  [[nodiscard]] auto &routes() const { return routes_; }
  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;

 private:
  std::shared_ptr<Instance> instance_;
  std::vector<size_t> routes_;
};

}  // namespace cye