from heuristics import *
import evrp
import os


def test_nearest_neighbor():
    for filename in os.listdir("dataset/json"):
        print(filename)
        for _ in range(10):
            instance = evrp.Instance(os.path.join("dataset/json", filename))
            assert nearest_neighbor(instance).is_valid()
