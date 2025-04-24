import evrp
import random


def find_charging_station(instance: evrp.Instance, node1_id: int, node2_id: int, battery: float) -> int | None:
    min_distance = float("inf")
    best_station_id = None

    if node1_id != instance.depot_id and node2_id != instance.depot_id and battery >= instance.energy_required(node1_id, instance.depot_id):
        min_distance = instance.distance_matrix[node1_id][instance.depot_id] + \
            instance.distance_matrix[instance.depot_id][node2_id]
        best_station_id = instance.depot_id

    for station_id in instance.charging_station_ids:
        if station_id == node1_id or station_id == node2_id or battery < instance.energy_required(node1_id, station_id):
            continue

        distance = instance.distance_matrix[node1_id][station_id] + \
            instance.distance_matrix[station_id][node2_id]
        if distance < min_distance:
            min_distance = distance
            best_station_id = station_id

    return best_station_id


def nearest_neighbor(instance: evrp.Instance):
    customer_ids = instance.customer_ids.copy()

    routes = [instance.depot_id]
    capacity = instance.cargo_capacity

    # Nearest neighbor
    while len(customer_ids) > 0:
        best_customer = -1
        best_distance = float("inf")
        for customer in customer_ids:
            travel_distance = instance.distance_matrix[routes[-1]][customer]

            if best_distance > travel_distance:
                best_distance = travel_distance
                best_customer = customer

        routes.append(best_customer)
        customer_ids.remove(best_customer)
    routes.append(instance.depot_id)

    # Fix capacity violations
    capacity = instance.cargo_capacity
    i = 1
    while i < len(routes):
        capacity -= instance.nodes[routes[i]]["demand"]
        if capacity < 0:
            routes.insert(i, instance.depot_id)
            capacity = instance.cargo_capacity
        i += 1

    # Fix energy violations
    energy = instance.energy_capacity
    i = 1
    while i < len(routes):
        if energy < instance.energy_required(routes[i-1], routes[i]):
            charging_station_id = find_charging_station(
                instance, routes[i-1], routes[i], energy)
            while charging_station_id is None:
                i -= 1
                assert i > 0
                energy += instance.energy_required(routes[i-1], routes[i])
                charging_station_id = find_charging_station(
                    instance, routes[i-1], routes[i], energy)

            routes.insert(i, charging_station_id)
            energy = instance.energy_capacity
        else:
            if routes[i] == instance.depot_id:
                energy = instance.energy_capacity
            else:
                energy -= instance.energy_required(routes[i-1], routes[i])

        i += 1

    return evrp.Solution(routes, instance)
