#include "cye/operators.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
#include <utility>
#include "cye/individual.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/sa/simulated_annealing.hpp"

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

  auto &cargo_patch = p1.solution().get_patch(0);
  auto dist = std::uniform_int_distribution(1UZ, cargo_patch.size() - 1);
  auto ind = dist(gen);
  auto i = cargo_patch.changes()[ind - 1].ind;
  auto j = cargo_patch.changes()[ind].ind - 1;

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

auto cye::TwoOptSearch::search(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  auto &base = solution.base();
  solution.clear_patches();
  cye::patch_cargo_trivially(solution);

  const auto &cargo_patch = solution.get_patch(0);

  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind;

    auto stop = false;
    while (!stop) {
      stop = true;
      for (auto l = route_begin; l < route_end - 1; ++l) {
        for (auto k = l + 1; k < route_end; k++) {
          auto current_dist = 0.0;
          auto swapped_distance = 0.0;

          if (l > 0) {
            current_dist += instance_->distance(base[l - 1], base[l]);
            swapped_distance += instance_->distance(base[l - 1], base[k]);
          }

          if (k < base.size() - 1) {
            current_dist += instance_->distance(base[k], base[k + 1]);
            swapped_distance += instance_->distance(base[l], base[k + 1]);
          }

          if (current_dist > swapped_distance) {
            stop = false;
            for (auto x = 0UZ; x <= (k - l) / 2; ++x) {
              std::swap(base[l + x], base[k - x]);
            }
          }
        }
      }
    }
  }
  cye::patch_energy_trivially(solution);
  individual.set_valid();

  return individual;
}

auto cye::SATwoOptSearch::search(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  auto &base = solution.base();
  solution.clear_patches();
  cye::patch_cargo_trivially(solution);
  auto dist = std::uniform_real_distribution<double>(0.0, 1.0);

  const auto &cargo_patch = solution.get_patch(0);
  auto min_temp = 1e-8;

  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind;

    auto stop = false;
    auto temp = 1e-3;
    
    while (!stop) {
      stop = true;
      temp *= 0.1;

      for (auto l = route_begin; l < route_end - 1; ++l) {
        for (auto k = l + 1; k < route_end; k++) {
          auto current_dist = 0.0;
          auto swapped_distance = 0.0;

          if (l > 0) {
            current_dist += instance_->distance(base[l - 1], base[l]);
            swapped_distance += instance_->distance(base[l - 1], base[k]);
          }

          if (k < base.size() - 1) {
            current_dist += instance_->distance(base[k], base[k + 1]);
            swapped_distance += instance_->distance(base[l], base[k + 1]);
          }

          double cost_diff = swapped_distance - current_dist;

          if (cost_diff < 0 || (temp > min_temp && dist(gen) < exp(-cost_diff / temp))) {
            stop = false;
            for (auto x = 0UZ; x <= (k - l) / 2; ++x) {
              std::swap(base[l + x], base[k - x]);
            }
          }
        }
      }
    }
  }
  cye::patch_energy_trivially(solution);
  individual.set_valid();

  return individual;
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
  cye::patch_energy_trivially(solution);
  individual.set_valid();

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

namespace {
static auto find_route(cye::Patch<size_t> const &patch, size_t index) -> std::pair<size_t, size_t> {
  auto it = std::ranges::upper_bound(patch.changes(), index, std::less{}, [&](auto const &el) { return el.ind; });

  auto route_end = it->ind;
  auto route_begin = (--it)->ind;

  return {route_begin, route_end};
}
}  // namespace

auto cye::HSM::mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();

  solution.clear_patches();
  cye::patch_cargo_trivially(solution);

  auto &customers = individual.genotype();
  auto dist = std::uniform_int_distribution<size_t>(0, customers.size() - 1);
  auto index = dist(gen);
  auto customer = customers[index];

  auto [route_begin, route_end] = find_route(solution.get_patch(0), index);

  size_t best_id = 0;
  double best_dist = std::numeric_limits<double>::infinity();
  for (size_t i = 0; i < customers.size(); ++i) {
    if (i >= route_begin && i < route_end) continue;

    auto dist = instance_->distance(customer, customers[i]);
    if (dist < best_dist) {
      best_dist = dist;
      best_id = i;
    }
  }
  std::swap(customers[index], customers[best_id]);
  return individual;
}

auto cye::HMM::mutate(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();

  solution.clear_patches();
  cye::patch_cargo_trivially(solution);

  auto &customers = individual.genotype();
  auto dist = std::uniform_int_distribution<size_t>(0, customers.size() - 1);
  auto index = dist(gen);
  auto customer = customers[index];

  auto [route_begin, route_end] = find_route(solution.get_patch(0), index);

  size_t best_id = 0;
  double best_dist = std::numeric_limits<double>::infinity();
  for (size_t i = 0; i < customers.size(); ++i) {
    if (i >= route_begin && i < route_end) continue;

    auto dist = instance_->distance(customer, customers[i]);
    if (dist < best_dist) {
      best_dist = dist;
      best_id = i;
    }
  }
  customers.erase(customers.begin() + index);

  if (best_id > index) {
    --best_id;
  }
  customers.insert(customers.begin() + best_id, customer);
  return individual;
}

cye::EVRPIndividual cye::DistributedCrossover::crossover(meta::RandomEngine &gen, cye::EVRPIndividual const &p1,
                                                         cye::EVRPIndividual const &p2) {
  if (other_) {
    auto ret = std::move(*other_);
    other_ = std::nullopt;
    return ret;
  }

  customers1_set_.clear();
  customers2_set_.clear();

  auto const &customers1 = p1.genotype();
  auto const &customers2 = p2.genotype();

  auto dist = std::uniform_int_distribution(0UZ, customers1.size() - 1);
  auto index = dist(gen);
  auto random_customer = customers1[index];

  const auto &route1_cargo_patch = p1.solution().get_patch(0);
  auto [route1_begin, route1_end] = find_route(route1_cargo_patch, index);

  auto customer_location = std::ranges::find(customers2, random_customer) - customers2.begin();
  const auto &route_2_cargo_patch = p2.solution().get_patch(0);
  auto [route2_begin, route2_end] = find_route(route_2_cargo_patch, customer_location);

  for (size_t i = route1_begin; i < route1_end; ++i) {
    customers1_set_.insert(customers1[i]);
  }
  for (size_t i = route2_begin; i < route2_end; ++i) {
    customers2_set_.insert(customers2[i]);
  }

  auto child1 = p1;
  auto child2 = p2;

  auto k = route1_begin;
  auto l = route2_begin;
  std::vector<size_t> insertions;
  for (auto &id : child1.genotype()) {
    auto contained = customers1_set_.contains(id) || customers2_set_.contains(id);
    if (!contained) continue;

    while (l < route2_end && customers1_set_.contains(customers2[l])) {
      l++;
    }

    if (l < route2_end) {
      id = customers2[l];
      insertions.push_back(customers2[l]);
      l++;
    } else {
      id = customers1[k];
      insertions.push_back(customers1[k]);
      k++;
    }
  }

  auto it = insertions.rbegin();
  for (auto &id : child2.genotype()) {
    auto contained = customers1_set_.contains(id) || customers2_set_.contains(id);
    if (!contained) continue;
    id = *it;
    ++it;
  }
  other_ = std::move(child2);

  return child1;
}

cye::CrossRouteScramble::CrossRouteScramble(double mutation_rate) : mutation_rate_(mutation_rate) {
  if (mutation_rate < 0.0 || mutation_rate > 1.0) {
    throw std::runtime_error("Mutation rate has to be between 0 and 1.");
  }
}

auto cye::CrossRouteScramble::mutate(meta::RandomEngine &gen, EVRPIndividual &&individual) -> EVRPIndividual {
  auto &solution = individual.solution();

  solution.clear_patches();
  cye::patch_cargo_trivially(solution);
  auto &cargo_patch = solution.get_patch(0);

  auto route_dist = std::uniform_int_distribution(0UZ, cargo_patch.changes().size() - 2);

  auto r1_ind = route_dist(gen);
  auto r2_ind = route_dist(gen);

  while (r1_ind == r2_ind) {
    r2_ind = route_dist(gen);
  }

  auto route1_begin = cargo_patch.changes()[r1_ind].ind;
  auto route1_end = cargo_patch.changes()[r1_ind + 1].ind;

  auto route2_begin = cargo_patch.changes()[r2_ind].ind;
  auto route2_end = cargo_patch.changes()[r2_ind + 1].ind;

  auto route1_dist = std::uniform_int_distribution(route1_begin, route1_end - 1);
  auto route2_dist = std::uniform_int_distribution(route2_begin, route2_end - 1);

  auto n = static_cast<size_t>(mutation_rate_ * static_cast<double>(solution.base().size()));
  for (auto i = 0UZ; i < n; i++) {
    std::swap(solution.base()[route1_dist(gen)], solution.base()[route2_dist(gen)]);
  }

  return individual;
}