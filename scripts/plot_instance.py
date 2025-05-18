import json
import matplotlib.pyplot as plt

with open("dataset/json/E-n22-k4.json", "r") as f:
    instance = json.loads(f.read())

print(instance)
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

plt.title(instance["name"])
plt.show()