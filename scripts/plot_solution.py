import matplotlib.pyplot as plt
import json
from itertools import groupby


def plot_solution():
    with open("solution.json", "r") as f:
        data = json.loads(f.read())

    instance = data["instance"]
    solution = data["solution"]

    nodes = instance["nodes"]

    fig, ax = plt.subplots()

    # Plot all nodes
    for id, node in enumerate(nodes):
        if node["type"] == "customer":
            ax.scatter(node["x"], node["y"], color="blue", marker="o")
        elif node["type"] == "chargingStation":
            ax.scatter(node["x"], node["y"], color="green", marker="s")
        elif node["type"] == "depot":
            ax.scatter(node["x"], node["y"], color="red", marker="D")

        ax.text(node["x"] + 0.5, node["y"] - 1.0, str(id), fontsize=9)

    # Plot routes
    colors = plt.cm.get_cmap("tab20", len(solution["routes"]))

    for idx, (key, route) in enumerate(groupby(solution["routes"], key=lambda x: x != 0)):
        if not key:
            continue
        route_x = [nodes[0]["x"]]
        route_y = [nodes[0]["y"]]
        for i in route:
            route_x.append(nodes[i]["x"])
            route_y.append(nodes[i]["y"])
        route_x.append(nodes[0]["x"])
        route_y.append(nodes[0]["y"])

        plt.plot(route_x, route_y, color=colors(
            idx), label=f"Vehicle {idx + 1}")

    plt.title(instance["name"])
    plt.show()


if __name__ == '__main__':
    plot_solution()
