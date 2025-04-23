import random
import math
import json
import matplotlib.pyplot as plt
from copy import deepcopy

from alns import ALNS
from alns.accept import HillClimbing
from alns.select import RandomSelect
from alns.stop import MaxIterations


def euclidean_distance(n1, n2):
    return math.hypot(n1["x"] - n2["x"], n1["y"] - n2["y"])

def calculate_distance_matrix(nodes):
    n = len(nodes)
    dist = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            dist[i][j] = euclidean_distance(nodes[i], nodes[j])
    return dist

def route_distance(route, distance_matrix):
    dist = 0
    for i in range(len(route) - 1):
        dist += distance_matrix[route[i]][route[i + 1]]
    return dist


class EVRPSolution:
    def __init__(self, routes, total_distance):
        self.routes = routes
        self.total_distance = total_distance


    def copy(self):
        return EVRPSolution(deepcopy(self.routes), self.total_distance)

    def objective(self):
        return self.total_distance


class EVRPData:
    def __init__(self, json_data):
        self.nodes = json_data["nodes"]
        self.vehicles = json_data["vehicles"]
        self.capacity = json_data["capacity"]
        self.energy_capacity = json_data["energyCapacity"]
        self.energy_consumption = json_data["energyConsumption"]
        self.distance_matrix = calculate_distance_matrix(self.nodes)
        self.customers = [
            i for i, node in enumerate(self.nodes) if node["type"] == "customer"
        ]
        self.depot = 0
        self.charging_stations = [
            i for i, node in enumerate(self.nodes) if node["type"] == "chargingStation"
        ]



def destroy_random(solution, data, **kwargs):
    new_solution = solution.copy()
    customers = [
        c
        for route in new_solution.routes
        for c in route
        if c != data.depot and data.nodes[c]["type"] == "customer"
    ]
    
    if not customers:
        return new_solution
        
    remove_count = kwargs.get('remove_count', 3)
    to_remove = random.sample(customers, min(remove_count, len(customers)))

    for route in new_solution.routes:
        route[:] = [n for n in route if n not in to_remove]

    # Remove empty routes
    new_solution.routes = [route for route in new_solution.routes if len(route) > 2]  # More than just depot-depot
    
    # Recalculate total distance
    new_solution.total_distance = sum(
        route_distance(route, data.distance_matrix) for route in new_solution.routes
    )
    
    return new_solution


def repair_greedy(solution, data, **kwargs):
    new_solution = solution.copy()
    
    # Get customers to insert (either from kwargs or from unassigned customers)
    to_insert = kwargs.get('to_insert', None)
    if to_insert is None:
        # Find all customers not in any route
        assigned = {c for route in new_solution.routes for c in route 
                   if data.nodes[c]["type"] == "customer"}
        to_insert = [c for c in data.customers if c not in assigned]
    
    if not to_insert:
        return new_solution, {'inserted': []}

    for customer in to_insert:
        best_route = None
        best_position = None
        best_increase = float("inf")
        customer_demand = data.nodes[customer]["demand"]

        for idx, route in enumerate(new_solution.routes):
            # Check capacity constraint
            current_load = sum(data.nodes[n]["demand"] for n in route if data.nodes[n]["type"] == "customer")
            if current_load + customer_demand > data.capacity:
                continue

            for pos in range(1, len(route)):
                # Calculate distance increase
                prev_node = route[pos-1]
                next_node = route[pos]
                increase = (data.distance_matrix[prev_node][customer] + 
                          data.distance_matrix[customer][next_node] - 
                          data.distance_matrix[prev_node][next_node])
                
                if increase < best_increase:
                    best_route = idx
                    best_position = pos
                    best_increase = increase

        if best_route is not None:
            new_solution.routes[best_route].insert(best_position, customer)
        else:
            # Create new route if couldn't insert (if we have available vehicles)
            if len(new_solution.routes) < data.vehicles:
                new_route = [data.depot, customer, data.depot]
                new_solution.routes.append(new_route)

    # Recalculate distances and repair energy constraints
    repaired_routes = []
    for route in new_solution.routes:
        repaired_route = add_charging_stations(route, data)
        repaired_routes.append(repaired_route)

    new_solution.routes = repaired_routes
    new_solution.total_distance = sum(
        route_distance(r, data.distance_matrix) for r in repaired_routes
    )
    
    return new_solution

def check_route_feasibility(route, data):
    battery = data.energy_capacity
    energy_consumption = data.energy_consumption
    total_distance = 0

    for i in range(len(route) - 1):
        travel_distance = data.distance_matrix[route[i]][route[i + 1]]
        total_distance += travel_distance
        battery -= travel_distance * energy_consumption

        if battery < 0:
            return False, float("inf")

        # Recharge if at charging station
        if data.nodes[route[i + 1]]["type"] == "chargingStation":
            battery = data.energy_capacity

    return True, total_distance


def add_charging_stations(route, data):
    if not route:
        return route

    battery = data.energy_capacity
    energy_consumption = data.energy_consumption
    new_route = [route[0]]  # start with depot

    for i in range(1, len(route)):
        from_node = new_route[-1]
        to_node = route[i]
        travel_distance = data.distance_matrix[from_node][to_node]
        energy_needed = travel_distance * energy_consumption

        if battery - energy_needed < 0:
            # Need to recharge -> find best station
            best_station = min(
                data.charging_stations,
                key=lambda s: data.distance_matrix[from_node][s] + data.distance_matrix[s][to_node],
            )
            new_route.append(best_station)
            battery = data.energy_capacity
            new_route.append(to_node)
            battery -= data.distance_matrix[best_station][to_node] * energy_consumption
        else:
            new_route.append(to_node)
            battery -= energy_needed

    return new_route


def find_nearest_charging_station(node, data, battery_left):
    best_station = None
    min_distance = float("inf")

    if battery_left - data.distance_matrix[node][data.depot] * data.energy_consumption >= 0:
        min_distance = data.distance_matrix[node][data.depot]
        best_station = data.depot

    for station in data.charging_stations:
        distance = data.distance_matrix[node][station]
        if distance < min_distance and battery_left - distance * data.energy_consumption >= 0:
            min_distance = distance
            best_station = station

    return best_station, min_distance


def initial_solution(data: EVRPData):
    customers = data.customers.copy()
    random.shuffle(customers)

    routes = [data.depot]
    capacity = data.capacity
    battery_left = data.energy_capacity

    while len(customers) > 0:
        best_customer = -1
        best_distance = float("inf")
        for customer in customers:
            demand = data.nodes[customer]["demand"]
            travel_distance = data.distance_matrix[routes[-1]][customer]
            energy_needed = travel_distance * data.energy_consumption

            if battery_left - energy_needed >= 0 and demand <= capacity and best_distance > travel_distance:
                best_distance = travel_distance
                best_customer = customer

        if best_customer != -1:
            routes.append(best_customer)
            customers.remove(best_customer)
            capacity -= data.nodes[best_customer]["demand"]
            battery_left -= best_distance * data.energy_consumption
            continue

        charging_station = find_nearest_charging_station(routes[-1], data, battery_left)
        while charging_station is None:
            previous = routes.pop()

            # please don't hit this this assertion
            assert data.nodes[previous]["type"] == "chargingStation", "No charging station found"
            assert data.nodes[previous]["type"] == "depot", "No depot found"

            capacity += data.nodes[previous]["demand"]
            battery_left += data.distance_matrix[routes[-1]][previous] * data.energy_consumption

            charging_station, _ = find_nearest_charging_station(routes[-1], data, battery_left)

        assert charging_station is not None, "No charging station found"
        routes.append(charging_station)
        battery_left = data.energy_capacity

        if charging_station == data.depot:
            capacity = data.capacity

    return EVRPSolution(routes, route_distance(routes, data.distance_matrix))


def run_alns(data):
    initial = initial_solution(data)

    print("initial cost:", initial.total_distance)
    print("Initial Routes:")
    for i, route in enumerate(initial.routes):
        print(f"Vehicle {i+1}: {route}")

    return
    alns = ALNS()
    alns.add_destroy_operator(lambda sol, rnd: destroy_random(sol, data))
    alns.add_repair_operator(lambda sol, rnd: repair_greedy(sol, data))

    num_destroy = 1
    num_repair = 1
    
    select = RandomSelect(num_destroy, num_repair)  # Pass the counts here
    accept = HillClimbing()
    stop = MaxIterations(100)

    result = alns.iterate(
        initial,
        select,
        accept,
        stop,
    )

    print("Best total distance:", result.best_state.objective())
    print("Routes:")
    for i, route in enumerate(result.best_state.routes):
        print(f"Vehicle {i+1}: {route}")

    return result


def plot_routes(data, routes):
    nodes = data.nodes
    depot = data.depot
    charging_stations = data.charging_stations

    colors = plt.cm.get_cmap("tab20", len(routes))

    plt.figure(figsize=(10, 8))

    # Plot all nodes
    for idx, node in enumerate(nodes):
        if node["type"] == "customer":
            plt.scatter(node["x"], node["y"], color="blue", marker="o", label="Customer" if idx == 0 else "")
        elif node["type"] == "chargingStation":
            plt.scatter(node["x"], node["y"], color="green", marker="s", label="Charging Station" if idx == 0 else "")
        elif node["type"] == "depot":
            plt.scatter(node["x"], node["y"], color="red", marker="D", label="Depot")

    # Plot routes
    for idx, route in enumerate(routes):
        route_x = [nodes[i]["x"] for i in route]
        route_y = [nodes[i]["y"] for i in route]
        plt.plot(route_x, route_y, color=colors(idx), label=f"Vehicle {idx + 1}")

    # Remove duplicate labels
    handles, labels = plt.gca().get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    plt.legend(by_label.values(), by_label.keys())

    plt.title("EVRP Solution with Charging Stations")
    plt.xlabel("X Coordinate")
    plt.ylabel("Y Coordinate")
    plt.grid(True)
    plt.show()


if __name__ == "__main__":
    dataset_json_file = "dataset/json/E-n22-k4.json"
    with open(dataset_json_file, "r") as f:
        json_data = json.load(f)
        formatted_json = json.dumps(json_data, indent=4)
        print(formatted_json)
        data = EVRPData(json_data)
        result = run_alns(data)
        plot_routes(data, result.best_state.routes)
