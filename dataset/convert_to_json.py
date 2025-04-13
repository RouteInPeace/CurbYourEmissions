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
                try:
                    value = int(value)
                except:
                    try:
                        value = float(value)
                    except:
                        pass

                result[camel_case(key)] = value
        i += 1

    result["nodeCoordinates"] = []
    i += 1
    while i < len(lines) and lines[i].strip() != "DEMAND_SECTION":
        line = lines[i].strip()
        if line:
            parts = line.split()
            if len(parts) >= 3:
                node = {"id": int(parts[0]), "x": float(
                    parts[1]), "y": float(parts[2])}
                result["nodeCoordinates"].append(node)
        i += 1

    result["demand"] = []
    i += 1
    while i < len(lines) and lines[i].strip() != "STATIONS_COORD_SECTION":
        line = lines[i].strip()
        if line:
            parts = line.split()
            if len(parts) >= 2:
                demand = {"node_id": int(parts[0]), "demand": float(parts[1])}
                result["demand"].append(demand)
        i += 1

    result["chargingStationCoordinates"] = []
    i += 1
    while i < len(lines) and lines[i].strip() != "DEPOT_SECTION":
        line = lines[i].strip()
        if line:
            parts = line.split()
            if parts:
                station_id = int(parts[0])
                for node in result["nodeCoordinates"]:
                    if node["id"] == station_id:
                        result["chargingStationCoordinates"].append(node)
                        break
        i += 1

    result["depot"] = []
    i += 1
    while i < len(lines) and lines[i].strip() != "EOF":
        line = lines[i].strip()
        if line and line != "-1":
            depot_id = int(line)
            for node in result["nodeCoordinates"]:
                if node["id"] == depot_id:
                    result["depot"].append(node)
                    break
        i += 1

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
