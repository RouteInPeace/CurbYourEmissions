#include "core/node.hpp"
#include <string_view>
#include "exceptions.hpp"

template <>
auto serial::JSONArchive::get<cye::NodeType>(std::string_view name) -> cye::NodeType {
  auto str = get<std::string_view>(name);
  if(str == "depot") return cye::NodeType::Depot;
  if(str == "customer") return cye::NodeType::Customer;
  if(str == "chargingStation") return cye::NodeType::ChargingStation;

  throw serial::InvalidValue(name);
}