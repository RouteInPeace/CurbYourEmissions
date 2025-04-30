#include "core/instance.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <stdexcept>
#include "core/node.hpp"
#include "core/utils.hpp"
#include "rapidjson/document.h"

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
  minimum_route_cnt_ =
      document.HasMember("vehicles") && document["vehicles"].IsUint64() ? document["vehicles"].GetUint64() : 0;

  max_cargo_capacity_ = document["capacity"].GetFloat();
  energy_capacity_ = document["energyCapacity"].GetFloat();
  energy_consumption_ = document["energyConsumption"].GetFloat();
  customer_cnt_ = 0;
  charging_station_cnt_ = 0;

  auto nodes_array = document["nodes"].GetArray();
  nodes_.reserve(nodes_array.Size());

  for (const auto &node : nodes_array) {
    if (!node.HasMember("id") || !node["id"].IsString()) throw std::runtime_error("Node is missing an id.");
    if (!node.HasMember("type") || !node["type"].IsString()) throw std::runtime_error("Node type is not specified.");
    if (!node.HasMember("x") || !node["x"].IsFloat()) throw std::runtime_error("Node is missing the x coordinate.");
    if (!node.HasMember("y") || !node["y"].IsFloat()) throw std::runtime_error("Node is missing the y coordinate.");

    auto type = NodeType::Depot;
    auto x = node["x"].GetFloat();
    auto y = node["y"].GetFloat();
    auto demand = 0.f;

    auto node_type = node["type"].GetString();
    if (strcmp(node_type, "depot") == 0) {
      type = NodeType::Depot;
    } else if (strcmp(node_type, "customer") == 0) {
      if (!node.HasMember("demand") || !node["demand"].IsFloat())
        throw std::runtime_error("Demand not specified for a customer node.");
      demand = node["demand"].GetFloat();
      type = NodeType::Customer;
      customer_cnt_++;
    } else if (strcmp(node_type, "chargingStation") == 0) {
      type = NodeType::ChargingStation;
      charging_station_cnt_++;
    } else {
      throw std::runtime_error("Invalid node type.");
    }

    nodes_.emplace_back(type, x, y, demand);
  }

  std::ranges::sort(nodes_,
                    [](auto &n1, auto &n2) { return static_cast<uint8_t>(n1.type) < static_cast<uint8_t>(n2.type); });
}

auto cye::Instance::distance(size_t node1_id, size_t node2_id) const -> float {
  auto delta_x = nodes_[node1_id].x - nodes_[node2_id].x;
  auto delta_y = nodes_[node1_id].y - nodes_[node2_id].y;

  return std::sqrt(delta_x * delta_x + delta_y * delta_y);
}