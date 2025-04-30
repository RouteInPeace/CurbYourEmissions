#pragma once

#include <cstdint>

namespace cye {

// DO NOT CHANGE THE ORDER!!!!!!!!!!!!
// The order is important in the instance class
enum class NodeType : uint8_t { Depot = 0, Customer = 1, ChargingStation = 2 };

struct Node {
  NodeType type;
  float x;
  float y;
  float demand;
};

}  // namespace cye