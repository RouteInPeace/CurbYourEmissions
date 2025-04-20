#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>
#include "nodes.hpp"

namespace cye {

class Instance {
 public:
  Instance(std::filesystem::path path);

 private:
  std::string name_;
  float optimal_value_;
  size_t minumim_route_cnt_;
  float cargo_capacity_;
  float energy_capacity_;
  float energy_consumption_;
  std::vector<std::variant<Customer, Depot, ChargingStation>> nodes_;
};

}  // namespace cye