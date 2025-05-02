#include "heuristics/init_heuristics.hpp"
#include <cassert>
#include <limits>
#include <optional>
#include <ranges>
#include <unordered_set>
#include <vector>
#include "core/instance.hpp"

auto find_charging_station(const cye::Instance &instance, size_t node1_id, size_t node2_id, float remaining_battery)
    -> std::optional<size_t> {
  auto best_station_id = std::optional<size_t>{};
  auto min_distance = std::numeric_limits<float>::infinity();

  if (node1_id != instance.depot_id() && node2_id != instance.depot_id() &&
      remaining_battery >= instance.energy_required(node1_id, instance.depot_id())) {
    min_distance = instance.distance(node1_id, instance.depot_id()) + instance.distance(instance.depot_id(), node2_id);
    best_station_id = instance.depot_id();
  }

  for (const auto station_id : instance.charging_station_ids()) {
    if (station_id == node1_id || station_id == node2_id) continue;
    if (remaining_battery < instance.energy_required(node1_id, station_id)) continue;

    auto distance = instance.distance(node1_id, station_id) + instance.distance(station_id, node2_id);
    if (distance < min_distance) {
      min_distance = distance;
      best_station_id = station_id;
    }
  }

  return best_station_id;
}

auto cye::nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution {
  auto remaining_customer_ids = std::ranges::to<std::unordered_set<size_t>>(instance->customer_ids());
  std::vector<size_t> routes;
  routes.push_back(instance->depot_id());

  auto cargo_capacity = instance->max_cargo_capacity();

  // Nearest neighbor
  while (!remaining_customer_ids.empty()) {
    auto best_customer_id = 0UZ;
    auto min_distance = std::numeric_limits<float>::infinity();

    for (const auto customer_id : remaining_customer_ids) {
      auto distance = instance->distance(routes.back(), customer_id);

      if (distance < min_distance) {
        min_distance = distance;
        best_customer_id = customer_id;
      }
    }

    cargo_capacity -= instance->node(best_customer_id).demand;
    if (cargo_capacity < 0) {
      routes.push_back(instance->depot_id());
      cargo_capacity = instance->max_cargo_capacity() - instance->node(best_customer_id).demand;
    }

    routes.push_back(best_customer_id);
    remaining_customer_ids.erase(best_customer_id);
  }

  routes.push_back(instance->depot_id());

  // Fix battery constrint violations
  auto energy = instance->energy_capacity();
  for (auto i = 1UZ; i < routes.size(); i++) {
    if (energy < instance->energy_required(routes[i - 1], routes[i])) {
      auto charging_station_id = find_charging_station(*instance, routes[i - 1], routes[i], energy);
      while (!charging_station_id.has_value()) {
        i -= 1;
        assert(i > 0);
        energy += instance->energy_required(routes[i - 1], routes[i]);
        charging_station_id = find_charging_station(*instance, routes[i - 1], routes[i], energy);
      }

      auto it = routes.begin() + i;
      routes.insert(it, *charging_station_id);
      energy = instance->energy_capacity();
    } else {
      if (routes[i] == instance->depot_id()) {
        energy = instance->energy_capacity();
      } else {
        energy -= instance->energy_required(routes[i - 1], routes[i]);
      }
    }
  }

  return Solution(instance, std::move(routes));
}