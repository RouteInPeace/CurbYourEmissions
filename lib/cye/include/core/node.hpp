#pragma once

#include <cstdint>
#include <json_archive.hpp>
#include "archive.hpp"

namespace cye {

// DO NOT CHANGE THE ORDER!!!!!!!!!!!!
// The order is important in the instance class
enum class NodeType : uint8_t { Depot = 0, Customer = 1, ChargingStation = 2 };

struct Node {
  Node() = default;

  template <serial::Value V>
  Node(V &&value);

  NodeType type;
  float x;
  float y;
  float demand;
};

template <serial::Value V>
Node::Node(V &&value)
    : type(value["type"].template get<NodeType>()),
      x(value["x"].template get<float>()),
      y(value["y"].template get<float>()),
      demand(value["demand"].template get_or<float>(0.f)) {}

}  // namespace cye

template <>
constexpr auto serial::JSONArchive::Value::get<cye::NodeType>() const -> cye::NodeType {
  auto str = get<std::string_view>();
  if (str == "depot") return cye::NodeType::Depot;
  if (str == "customer") return cye::NodeType::Customer;
  if (str == "chargingStation") return cye::NodeType::ChargingStation;

  throw std::runtime_error("Invalid node type.");
}