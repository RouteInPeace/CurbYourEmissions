import evrp

def test_evrp_solution_is_valid():
    instance = evrp.Instance("dataset/json/E-n22-k4.json")
    instance.energy_capacity = 1e9

    routes = [0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0]
    assert evrp.Solution(routes, instance).is_valid()

    # Not starting at the depot
    routes = [1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0]
    assert not evrp.Solution(routes, instance).is_valid()

    routes = [0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21]
    assert not evrp.Solution(routes, instance).is_valid()

    # Didn't visit everybody
    routes = [0, 1, 0, 2, 0]
    assert not evrp.Solution(routes, instance).is_valid()

    # Visited somebody multiple times
    routes = [0, 1, 2, 3, 4, 5, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0]
    assert not evrp.Solution(routes, instance).is_valid()

    # Not enough capacity
    routes = [0, 10, 28, 12, 27, 14, 16, 23, 13, 11, 24, 8, 6, 10, 0]
    assert not evrp.Solution(routes, instance).is_valid()

    # Not enough energy
    instance.energy_capacity = 10
    routes = [0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0]
    assert not evrp.Solution(routes, instance).is_valid()

    # Recharging
    instance.energy_capacity = 120
    routes = [0, 1, 29, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11,
              0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21, 0]
    assert evrp.Solution(routes, instance).is_valid()
