import json
import os
from pathlib import Path


def camel_case(s):
    parts = s.lower().split('_')
    return parts[0] + ''.join(part.capitalize() for part in parts[1:])


def parse_evrp_data(data):
    result = {}
    lines = data.split("\n")

    i = 0
    while i < len(lines) and lines[i].strip() != "NODE_COORD_SECTION":
        line = lines[i].strip()
        if line:
            if ": " in line:
                key, value = line.split(": ", 1)

                if key == "Name":
                    result["name"] = value
                elif key == "OPTIMAL_VALUE":
                    result["optimalValue"] = float(value)
                elif key == "VEHICLES":
                    result["minimumRouteCnt"] = int(value)
                elif key == "CAPACITY":
                    result["cargoCapacity"] = float(value)
                elif key == "ENERGY_CAPACITY":
                    result["batteryCapacity"] = float(value)
                elif key == "ENERGY_CONSUMPTION":
                    result["energyConsumption"] = float(value)
                elif key == "DIMENSION":
                    result["customerCnt"] = int(value)
                elif key == "STATIONS":
                    result["chargingStationCnt"] = int(value)
        i += 1

    result["customerCnt"] -= 1
    nodes = {}

    i += 1
    while i < len(lines) and lines[i].strip() != "DEMAND_SECTION":
        line = lines[i].strip()
        if line:
            parts = line.split()
            if len(parts) >= 3:
                id = parts[0]
                x = float(parts[1])
                y = float(parts[2])

                nodes[id] = {"x": x, "y": y}
        i += 1

    i += 1
    while i < len(lines) and lines[i].strip() != "STATIONS_COORD_SECTION":
        line = lines[i].strip()
        if line:
            parts = line.split()
            if len(parts) >= 2:
                id = parts[0]
                demand = float(parts[1])
                nodes[id]["demand"] = demand
                nodes[id]["type"] = "customer"
        i += 1

    i += 1
    while i < len(lines) and lines[i].strip() != "DEPOT_SECTION":
        id = lines[i].strip()
        if id:
            nodes[id]["type"] = "chargingStation"
        i += 1

    i += 1
    while i < len(lines) and lines[i].strip() != "EOF":
        id = lines[i].strip()
        if id and id != "-1":
            nodes[id]["type"] = "depot"
        i += 1

    result["nodes"] = []
    for key, value in nodes.items():
        value["id"] = key
        result["nodes"].append(value)

    return result


if __name__ == "__main__":
    dataset_path = Path("dataset/original")
    dataset_json_path = "dataset/json"

    if not os.path.exists(dataset_json_path):
        os.makedirs(dataset_json_path)

    for file_path in dataset_path.glob("*.evrp"):
        with open(file_path, "r") as file:
            data = file.read()

        parsed_data = parse_evrp_data(data)
        json_file_path = f"{dataset_json_path}/{file_path.stem}.json"
        with open(json_file_path, "w") as json_file:
            json.dump(parsed_data, json_file, indent=2)
