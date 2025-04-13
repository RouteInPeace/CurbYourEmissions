#include "instance.hpp"
#include <filesystem>
#include <format>
#include <stdexcept>
#include "nodes.hpp"
#include "rapidjson/document.h"
#include "utils.hpp"

cye::Instance::Instance(std::filesystem::path path) {
  auto data = readFile(path);
  if (!data) {
    throw std::runtime_error(std::format("Could not open file {}", path.c_str()));
  }

  auto document = rapidjson::Document();
  document.Parse(data->c_str());

  // check if mandatory information is specified
  if (!document.HasMember("capacity") || !document["capacity"].IsFloat())
    throw std::runtime_error("Cargo capacity is not specified.");
  if (!document.HasMember("energyCapacity") || !document["energyCapacity"].IsFloat())
    throw std::runtime_error("Energy capacity is not specified.");
  if (!document.HasMember("energyConsumption") || !document["energyConsumption"].IsFloat())
    throw std::runtime_error("Energy consumption is not specified.");
  if (!document.HasMember("nodes") || !document["nodes"].IsArray())
    throw std::runtime_error("The list of nodes is missing.");

  name_ = document.HasMember("name") && document["name"].IsString() ? document["name"].GetString() : "";
  optimal_value_ = document.HasMember("optimalValue") && document["optimalValue"].IsFloat()
                       ? document["optimalValue"].GetFloat()
                       : -1.0f;
  minumim_route_cnt_ =
      document.HasMember("vehicles") && document["vehicles"].IsUint64() ? document["vehicles"].GetUint64() : 0;

  cargo_capacity_ = document["capacity"].GetFloat();
  energy_capacity_ = document["energyCapacity"].GetFloat();
  energy_consumption_ = document["energyConsumption"].GetFloat();

  auto nodes_array = document["nodes"].GetArray();
  nodes_.reserve(nodes_array.Size());

  for (const auto &node : nodes_array) {
    if (!node.HasMember("id") || !node["id"].IsString()) throw std::runtime_error("Node is missing an id.");
    if (!node.HasMember("type") || !node["type"].IsString()) throw std::runtime_error("Node type is not specified.");
    if (!node.HasMember("x") || !node["x"].IsFloat()) throw std::runtime_error("Node is missing the x coordinate.");
    if (!node.HasMember("y") || !node["y"].IsFloat()) throw std::runtime_error("Node is missing the y coordinate.");

    auto x = node["x"].GetFloat();
    auto y = node["y"].GetFloat();

    auto node_type = node["type"].GetString();
    if (strcmp(node_type, "depot") == 0) {
      nodes_.emplace_back(Depot{x, y});
    } else if (strcmp(node_type, "customer") == 0) {
      if (!node.HasMember("demand") || !node["demand"].IsFloat())
        throw std::runtime_error("Demand not specified for a customer node.");

      auto demand = node["demand"].GetFloat();
      nodes_.emplace_back(Customer{x, y, demand});
    } else if (strcmp(node_type, "chargingStation") == 0) {
      nodes_.emplace_back(ChargingStation{x, y});
    } else {
      throw std::runtime_error("Invalid node type.");
    }
  }
}