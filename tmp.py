import json
import math

with open("dataset/json/small.json", "r") as f:
    instance = json.loads(f.read())


for i in range(4):
    for j in range(4):
        dist = math.sqrt((instance["nodes"][i]["x"] - instance["nodes"][j]["x"])
                         ** 2 + (instance["nodes"][i]["y"] - instance["nodes"][j]["y"]) ** 2)
        print(round(dist, 2), end=' ') # math.ceil((dist * 1.2) / 18.7999992)
    print()
