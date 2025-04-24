import evrp
import random


def nearest_neighbor(instance: evrp.Instance):
    customer_ids = instance.customer_ids.copy()
    random.shuffle(customer_ids)

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
            charging_station_id, _ = instance.find_charging_station(
                routes[i-1], routes[i])
            while instance.energy_required(routes[i-1], charging_station_id) > energy:
                i -= 1
                assert i > 0
                energy += instance.energy_required(routes[i-1], routes[i])
                charging_station_id, _ = instance.find_charging_station(
                    routes[i-1], routes[i])

            routes.insert(i, charging_station_id)
            energy = instance.energy_capacity
        else:
            if routes[i] == instance.depot_id:
                energy = instance.energy_capacity
            else :
                energy -= instance.energy_required(routes[i-1], routes[i])

        i += 1

    return evrp.Solution(routes, instance)
