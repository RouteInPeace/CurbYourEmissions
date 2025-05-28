#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/sa/simulated_annealing.hpp"

auto main() -> int {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto archive = serial::JSONArchive("dataset/json/X-n143-k7.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  auto config = meta::sa::Config<cye::Solution>(std::move(solution));
  config.get_neighbour = [](meta::RandomEngine &gen, cye::Solution const &solution) {
    auto copy = solution;

    auto base = copy.base();
    auto dist = std::uniform_int_distribution(0UZ, base.size() - 1UZ);
    auto i = dist(gen);
    auto j = dist(gen);
    if (i > j) std::swap(i, j);
    assert(i <= j);

    for (auto k = 0UZ; k <= (j - i) / 2; ++k) {
      std::swap(base[i + k], base[j - k]);
    }

    copy.clear_patches();
    cye::patch_endpoint_depots(copy);
    cye::patch_cargo_trivially(copy);
    cye::patch_energy_trivially(copy);
    assert(copy.is_valid());

    return copy;
  };

  config.get_temperature = meta::sa::create_geometric_schedule(1e2, 1e-12, 0.99999);
  config.verbose = true;

  auto best_solution = meta::sa::optimize(gen, config);
  std::cout << best_solution.cost() << ' ' << best_solution.is_valid() << '\n';
}