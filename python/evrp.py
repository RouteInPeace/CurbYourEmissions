import math
from copy import deepcopy
import json
from typing import Self


class Instance:
    def __init__(self, path: str):
        with open(path, "r") as f:
            json_data = json.load(f)

        self.nodes = json_data["nodes"]
        self.vehicles = json_data["vehicles"]
        self.cargo_capacity = json_data["capacity"]
        self.energy_capacity = json_data["energyCapacity"]
        self.energy_consumption = json_data["energyConsumption"]
        self.customer_ids = [
            i for i, node in enumerate(self.nodes) if node["type"] == "customer"
        ]
        self.depot_id = 0
        self.charging_station_ids = [
            i for i, node in enumerate(self.nodes) if node["type"] == "chargingStation"
        ]

        self.distance_matrix = None
        self.update_distance_matrix()

    def distance(self, node1_id: int, node2_id: int) -> float:
        return math.hypot(self.nodes[node1_id]["x"] - self.nodes[node2_id]["x"], self.nodes[node1_id]["y"] - self.nodes[node2_id]["y"])

    def energy_required(self, node1_id: int, node2_id: int) -> float:
        return self.distance_matrix[node1_id][node2_id] * self.energy_consumption

    def update_distance_matrix(self):
        n = len(self.nodes)
        self.distance_matrix = [[0] * n for _ in range(n)]
        for i in range(n):
            for j in range(n):
                self.distance_matrix[i][j] = self.distance(i, j)
        return self.distance_matrix

    def find_charging_station(self, node_id_1: int, node_id_2: int) -> tuple[int, float]:
        min_distance = float("inf")
        best_station_id = -1

        if node_id_1 != self.depot_id and node_id_2 == self.depot_id:
            min_distance = self.distance_matrix[node_id_1][self.depot_id] + self.distance_matrix[self.depot_id][node_id_2]
            best_station_id = self.depot_id

        for station_id in self.charging_station_ids:
            if station_id == node_id_1 or station_id == node_id_2:
                continue

            distance = self.distance_matrix[node_id_1][station_id] + self.distance_matrix[station_id][node_id_2]
            if distance < min_distance:
                min_distance = distance
                best_station_id = station_id

        return best_station_id, min_distance


class Solution:
    def __init__(self, routes: list[int], instance: Instance):
        self.routes = routes
        self.instance = instance
        self.total_distance = -1

    def copy(self) -> Self:
        return Solution(deepcopy(self.routes), self.total_distance)

    def eval(self):
        self.total_distance = 0
        for i in range(len(self.routes) - 1):
            self.total_distance += self.instance.distance_matrix[self.routes[i]
                                                                 ][self.routes[i + 1]]

    def is_valid(self) -> bool:
        if self.routes[0] != self.instance.depot_id or self.routes[-1] != self.instance.depot_id:
            return False

        customers = [
            id for id in self.routes if self.instance.nodes[id]["type"] == "customer"]
        if len(self.instance.customer_ids) != len(customers) or len(self.instance.customer_ids) != len(set(customers)):
            return False

        energy = self.instance.energy_capacity
        cargo = self.instance.cargo_capacity

        for i in range(1, len(self.routes)):
            energy -= self.instance.energy_required(
                self.routes[i-1], self.routes[i])
            if energy < 0:
                return False

            node = self.instance.nodes[self.routes[i]]
            if node["type"] == "customer":
                cargo -= node["demand"]
                if cargo < 0:
                    return False
            elif node["type"] == "depot":
                energy = self.instance.energy_capacity
                cargo = self.instance.cargo_capacity
            elif node["type"] == "chargingStation":
                energy = self.instance.energy_capacity
            else:
                raise "Unknown node type"

        return True
