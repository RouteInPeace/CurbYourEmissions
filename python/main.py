import random
import json
import matplotlib.pyplot as plt
import evrp
import utils
import heuristics
import os

import numpy as np

from alns import ALNS
from alns.accept import RecordToRecordTravel
from alns.select import RouletteWheel
from alns.stop import MaxIterations


def random_removal(state: evrp.Solution, rng: np.random._generator.Generator) -> evrp.Solution:
    destroyed = state.copy()

    new_route = []
    destroyed.unassigned = []
    for node_id in destroyed.routes:
        if rng.random() < 0.95 or state.instance.nodes[node_id]["type"] != "customer":
            new_route.append(node_id)
        else:
            destroyed.unassigned.append(node_id)

    destroyed.routes = new_route

    return destroyed


def greedy_repair(state: evrp.Solution, rng: np.random._generator.Generator) -> evrp.Solution:
    """
    Inserts the unassigned customers in the best route. If there are no
    feasible insertions, then a new route is created.
    """
    rng.shuffle(state.unassigned)

    while len(state.unassigned) > 0:
        customer = state.unassigned.pop()

        best_objective = float("inf")
        best_insertion_place = -1

        for i in range(1, len(state.routes)):
            copy = state.copy()
            copy.routes.insert(i, customer)
            if copy.is_energy_and_cargo_valid():
                objective = copy.objective()
                if objective < best_objective:
                    best_objective = objective
                    best_insertion_place = i

        state.routes.insert(best_insertion_place, customer)

    return state


def run_alns(instance: evrp.Instance):
    # todo change seed
    alns = ALNS(np.random.default_rng(0))
    alns.add_destroy_operator(random_removal)
    alns.add_repair_operator(greedy_repair)

    num_iterations = 500
    initial_solution = heuristics.nearest_neighbor(instance)
    print(initial_solution.objective())
    select = RouletteWheel([25, 5, 1, 0], 0.8, 1, 1)
    accept = RecordToRecordTravel.autofit(
        initial_solution.objective(), 0.02, 0, num_iterations
    )
    stop = MaxIterations(num_iterations)
    result = alns.iterate(initial_solution, select, accept, stop)

    solution = result.best_state
    print(solution.routes, solution.objective())

    utils.plot_solution(initial_solution)
    utils.plot_solution(solution)
    # objective = solution.objective()
    # pct_diff = 100 * (objective - bks.cost) / bks.cost

    # print(f"Best heuristic objective is {objective}.")
    # print(
    #     f"This is {pct_diff:.1f}% worse than the optimal solution, which is {bks.cost}."
    # )


if __name__ == "__main__":
    instance = evrp.Instance("dataset/json/E-n101-k8.json")
    run_alns(instance)

    # for filename in os.listdir("dataset/json"):
    #     instance = evrp.Instance(os.path.join("dataset/json", filename))
    #     solution = heuristics.nearest_neighbor(instance)

    #     utils.plot_solution(solution, path=os.path.join(
    #         "results", filename[:-4] + 'png'))
    #     print(filename, solution.eval())
