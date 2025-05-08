#include "core/node.hpp"
#include <stdexcept>
#include <string_view>

template <>
auto serial::JSONArchive::Value::get<cye::NodeType>() -> cye::NodeType {
  auto str = get<std::string_view>();
  if(str == "depot") return cye::NodeType::Depot;
  if(str == "customer") return cye::NodeType::Customer;
  if(str == "chargingStation") return cye::NodeType::ChargingStation;

  throw std::runtime_error("Invalid node type.");
}