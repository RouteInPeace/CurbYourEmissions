#pragma once

#include <cstdint>
#include <json_archive.hpp>

namespace cye {

// DO NOT CHANGE THE ORDER!!!!!!!!!!!!
// The order is important in the instance class
enum class NodeType : uint8_t { Depot = 0, Customer = 1, ChargingStation = 2 };

struct Node {
  Node() = default;

  template <typename Value>
  Node(Value &&value);

  NodeType type;
  float x;
  float y;
  float demand;
};

template <typename Value>
Node::Node(Value &&value)
    : type(value["type"].template get<NodeType>()),
      x(value["x"].template get<float>()),
      y(value["y"].template get<float>()),
      demand(value["demand"].template get_or<float>(0.f)) {}

}  // namespace cye

template <>
auto serial::JSONArchive::Value::get<cye::NodeType>() -> cye::NodeType;
