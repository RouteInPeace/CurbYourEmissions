from itertools import groupby
import evrp
import matplotlib.pyplot as plt


def plot_instance(instance: evrp.Instance):
    nodes = instance.nodes

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

    plt.title("EVRP Instance")
    plt.show()


def plot_solution(solution: evrp.Solution):
    nodes = solution.instance.nodes

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
    colors = plt.cm.get_cmap("tab20", len(solution.routes))

    for idx, (key, route) in enumerate(groupby(solution.routes, key=lambda x: x != solution.instance.depot_id)):
        if not key:
            continue
        route_x = [nodes[solution.instance.depot_id]["x"]]
        route_y = [nodes[solution.instance.depot_id]["y"]]
        for i in route:
            route_x.append(nodes[i]["x"])
            route_y.append(nodes[i]["y"])
        route_x.append(nodes[solution.instance.depot_id]["x"])
        route_y.append(nodes[solution.instance.depot_id]["y"])

        plt.plot(route_x, route_y, color=colors(
            idx), label=f"Vehicle {idx + 1}")

    plt.title("EVRP Instance")
    plt.show()
