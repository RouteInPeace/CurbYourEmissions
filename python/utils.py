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
