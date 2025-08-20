#pragma once

#include <cstdint>
#include <serial/json_archive.hpp>
#include "serial/archive.hpp"

namespace cye {

// DO NOT CHANGE THE ORDER!!!!!!!!!!!!
// The order is important in the instance class
enum class NodeType : uint8_t { Depot = 0, Customer = 1, ChargingStation = 2 };

struct Node {
  Node() = default;

  template <serial::Value V>
  Node(V &&value);

  template <serial::Value V>
  auto write(V value) const -> void;

  NodeType type;
  double x;
  double y;
  double demand;
};

template <serial::Value V>
Node::Node(V &&value)
    : type(value["type"].template get<NodeType>()),
      x(value["x"].template get<double>()),
      y(value["y"].template get<double>()),
      demand(value["demand"].template get_or<double>(0.f)) {}

template <serial::Value V>
auto Node::write(V v) const -> void {
  v.emplace("x", x);
  v.emplace("y", y);
  v.emplace("demand", demand);
  switch (type) {
    case NodeType::Depot:
      v.emplace("type", "depot");
      break;
    case NodeType::Customer:
      v.emplace("type", "customer");
      break;
    case NodeType::ChargingStation:
      v.emplace("type", "chargingStation");
      break;
  }
}

}  // namespace cye

template <>
constexpr auto serial::JSONArchive::Value::get<cye::NodeType>() const -> cye::NodeType {
  auto str = get<std::string_view>();
  if (str == "depot") return cye::NodeType::Depot;
  if (str == "customer") return cye::NodeType::Customer;
  if (str == "chargingStation") return cye::NodeType::ChargingStation;

  throw std::runtime_error("Invalid node type.");
}
