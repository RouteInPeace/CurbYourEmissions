#pragma once

#include <cstdint>
#include <json_archive.hpp>

namespace cye {

// DO NOT CHANGE THE ORDER!!!!!!!!!!!!
// The order is important in the instance class
enum class NodeType : uint8_t { Depot = 0, Customer = 1, ChargingStation = 2 };

struct Node {
  Node() = default;

  template <typename Archive>
  Node(Archive &&archive);

  NodeType type;
  float x;
  float y;
  float demand;
};

template <typename Archive>
Node::Node(Archive &&archive)
    : type(archive.get("name")), x(archive.get["x"]), y(archive.get("y")), demand(archive.get("demand")) {}

}  // namespace cye

template <>
auto serial::JSONArchive::get<cye::NodeType>(std::string_view name) -> cye::NodeType;
