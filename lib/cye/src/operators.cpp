#include "cye/operators.hpp"
#include "cye/repair.hpp"

auto cye::NeighborSwap::mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  candidates_.clear();
  auto genotype = individual.genotype();
  auto &instance = individual.solution().instance();

  auto dist = std::uniform_int_distribution(0UZ, genotype.size() - 1);
  auto ind1 = dist(gen);
  auto node1_id = genotype[ind1];

  for (auto i = 0UZ; i < genotype.size(); ++i) {
    if (i == ind1) {
      continue;
    }
    auto node2_id = genotype[i];
    auto distance = instance.distance(node1_id, node2_id);

    candidates_.emplace(distance, i);
    if (candidates_.size() > k_) {
      candidates_.erase(--candidates_.end());
    }
  }

  auto dist2 = std::uniform_int_distribution(0UZ, k_ - 1);
  auto candidate_ind = std::min(dist2(gen), candidates_.size() - 1);
  auto it = candidates_.begin();
  while (candidate_ind > 0) {
    ++it;
    --candidate_ind;
  }
  auto ind2 = it->second;

  std::swap(genotype[ind1], genotype[ind2]);
  return individual;
}

auto cye::RouteOX1::crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1, cye::EVRPIndividual const &p2)
    -> cye::EVRPIndividual {
  auto child = p1;

  auto &&p1_genotype = p1.genotype();
  auto &&p2_genotype = p2.genotype();
  auto &&child_genotype = child.genotype();

  auto &depot_patch = p1.solution().get_patch(1);
  auto dist = std::uniform_int_distribution(0UZ, depot_patch.size());
  auto ind = dist(gen);
  auto j = ind == depot_patch.size() ? p1_genotype.size() - 1 : depot_patch.changes()[ind].ind - 1;
  auto i = ind == 0 ? 0 : depot_patch.changes()[ind - 1].ind;

  assert(p1_genotype.size() == p2_genotype.size());

  assert(i <= j);

  used_.clear();
  for (auto k = i; k <= j; ++k) {
    used_.emplace(p1_genotype[k]);
  }

  auto l = 0UZ;
  for (auto k = 0UZ; k < p1_genotype.size(); ++k) {
    // Skip the copied section
    if (k == i) {
      k = j;
      continue;
    }

    while (used_.contains(p2_genotype[l])) {
      ++l;
    }

    child_genotype[k] = p2_genotype[l];
    ++l;
  }

  return child;
}

auto cye::SwapSearch::search(meta::RandomEngine & /*gen*/, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  auto &base = solution.base();
  solution.clear_patches();
  cye::patch_cargo_trivially(solution);

  const auto &cargo_patch = solution.get_patch(0);

  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind - 1;

    auto stop = false;
    while (!stop) {
      stop = true;
      for (auto l = route_begin; l < route_end; l++) {
        for (auto k = l + 1; k <= route_end; k++) {
          auto prev_dist = neighbor_dist_(base, l) + neighbor_dist_(base, k);
          std::swap(base[l], base[k]);
          auto new_dist = neighbor_dist_(base, l) + neighbor_dist_(base, k);

          if (new_dist < prev_dist) {
            stop = false;
          } else {
            std::swap(base[l], base[k]);
          }
        }
      }
    }
  }
  // energy_repair_->patch(solution, 101u);
  cye::patch_energy_trivially(solution);

  return individual;
}

auto cye::SwapSearch::neighbor_dist_(std::vector<size_t> const &base, size_t i) -> float {
  auto d = 0.f;
  if (i > 0) {
    d += instance_->distance(base[i - 1], base[i]);
  }
  if (i < base.size() - 1) {
    d += instance_->distance(base[i], base[i + 1]);
  }

  return d;
}