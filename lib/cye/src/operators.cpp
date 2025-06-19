#include "cye/operators.hpp"
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <utility>
#include "cye/individual.hpp"
#include "cye/instance.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"
#include "meta/common.hpp"
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

namespace {

std::vector<size_t> convert_to_vector(cye::Solution const &sol) {
    std::vector<size_t> v;
    v.reserve(sol.visited_node_cnt() + 2);
    for (auto it : sol.routes()) {
      v.emplace_back(it);
    }
    return v;
};

void DoTwoOpt(cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  auto &base = solution.base();
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
            current_dist += instance->distance(base[l - 1], base[l]);
            swapped_distance += instance->distance(base[l - 1], base[k]);
          }

          if (k < base.size() - 1) {
            current_dist += instance->distance(base[k], base[k + 1]);
            swapped_distance += instance->distance(base[l], base[k + 1]);
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
}

void DoTwoOptFull(cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  auto &base = solution.base();
  auto stop = false;
  while (!stop) {
    stop = true;
    for (auto l = 0UZ; l < base.size() - 1; ++l) {
      for (auto k = l + 1; k < base.size(); k++) {
        auto current_dist = 0.0;
        auto swapped_distance = 0.0;

        if (l > 0) {
          current_dist += instance->distance(base[l - 1], base[l]);
          swapped_distance += instance->distance(base[l - 1], base[k]);
        }

        if (k < base.size() - 1) {
          current_dist += instance->distance(base[k], base[k + 1]);
          swapped_distance += instance->distance(base[l], base[k + 1]);
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
  solution.clear_patches();
  cye::patch_cargo_optimally(solution);
}

inline auto neighbor_dist(std::vector<size_t> const &base, size_t i, const cye::Instance *instance) -> float {
  auto d = 0.f;
  if (i > 0) {
    d += instance->distance(base[i - 1], base[i]);
  }
  if (i < base.size() - 1) {
    d += instance->distance(base[i], base[i + 1]);
  }

  return d;
}

void DoSwapSearch(cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  auto &base = solution.base();
  const auto &cargo_patch = solution.get_patch(0);
  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind - 1;

    auto stop = false;
    while (!stop) {
      stop = true;
      for (auto l = route_begin; l < route_end; l++) {
        for (auto k = l + 1; k <= route_end; k++) {
          auto prev_dist = neighbor_dist(base, l, instance) + neighbor_dist(base, k, instance);
          std::swap(base[l], base[k]);
          auto new_dist = neighbor_dist(base, l, instance) + neighbor_dist(base, k, instance);

          if (new_dist < prev_dist) {
            stop = false;
          } else {
            std::swap(base[l], base[k]);
          }
        }
      }
    }
  }
}

bool check_load(std::vector<size_t> const &base, cye::Instance const *instance, size_t index) {
  auto load = 0U;
  for (auto i = index; base[i] != instance->depot_id(); ++i) {
    load += instance->demand(base[i]);
  }
  for (auto i = index - 1; base[i] != instance->depot_id(); --i) {
    load += instance->demand(base[i]);
  }
  return load <= instance->cargo_capacity();
}

std::vector<size_t> generate_shuffled_indices(size_t size, meta::RandomEngine &gen) {
  std::vector<size_t> v(size);
  for (size_t i = 1; i < size - 1; ++i) {
    v[i] = i;
  }
  std::shuffle(v.begin(), v.end(), gen);
  return v;
}

bool DoFullSwapSearch(meta::RandomEngine &gen, cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  std::vector<size_t> route;
  for (auto it : solution.routes()) {
    route.emplace_back(it);
  }

  bool stop = false;
  bool found_improvement = false;
  auto cost = solution.cost();

  auto convert_to_solution = [&](std::vector<size_t> const &v) {
    std::vector<size_t> new_base;
    new_base.reserve(v.size() + 1);
    for (auto it : v) {
      if (it != instance->depot_id()) {
        new_base.emplace_back(it);
      }
    }
    return cye::Solution(solution.instance_ptr(), std::move(new_base));
  };
  //auto indices = generate_shuffled_indices(route.size(), gen);

  while (!stop) {
    stop = true;
    for (auto l = 0UZ; l < route.size() - 1; l++) {
      if (route[l] == instance->depot_id()) continue;
      for (auto k = l + 1; k < route.size() - 1; k++) {
        if (route[k] == instance->depot_id()) continue;
        auto prev_dist = neighbor_dist(route, l, instance) + neighbor_dist(route, k, instance);
        std::swap(route[l], route[k]);
        auto new_dist = neighbor_dist(route, l, instance) + neighbor_dist(route, k, instance);

        if (new_dist < prev_dist) {
          if (!check_load(route, instance, l) || !check_load(route, instance, k)) {
            auto new_sol = convert_to_solution(route);
            cye::patch_cargo_trivially(new_sol);
            auto new_cost = new_sol.cost();
            if (new_cost < cost) {
              solution = std::move(new_sol);
              cost = new_cost;
              route = convert_to_vector(solution);
              continue;
            }
            std::swap(route[l], route[k]);
            continue;
          }

          cost += new_dist - prev_dist;
          found_improvement = true;
          stop = false;
        } else {
          std::swap(route[l], route[k]);
        }
      }
    }
  }
  std::vector<size_t> new_base;
  new_base.reserve(route.size() + 1);
  for (auto it : route) {
    if (it != instance->depot_id()) {
      new_base.emplace_back(it);
    }
  }
  solution = cye::Solution(solution.instance_ptr(), std::move(new_base));
  solution.clear_patches();
  cye::patch_cargo_optimally(solution);
  return found_improvement;
}

bool DoFullTwoOptSearch(meta::RandomEngine &gen, cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  std::vector<size_t> route;
  for (auto it : solution.routes()) {
    route.emplace_back(it);
  }

  bool stop = false;
  auto cost = solution.cost();

  auto convert_to_solution = [&](std::vector<size_t> const &v) {
    std::vector<size_t> new_base;
    new_base.reserve(v.size() + 1);
    for (auto it : v) {
      if (it != instance->depot_id()) {
        new_base.emplace_back(it);
      }
    }
    return cye::Solution(solution.instance_ptr(), std::move(new_base));
  };

  auto indices = generate_shuffled_indices(route.size(), gen);
  bool found_improvement = false;

  while (!stop) {
    stop = true;
    for (auto i = 0UZ; i < route.size() - 1; i++) {
      if (route[i] == instance->depot_id()) continue;
      for (auto j = i + 1; j < route.size() - 1; j++) {
        if (route[j] == instance->depot_id()) continue;

        // Calculate the current distance involving edges (i, i+1) and (j, j+1)
        double current_dist = instance->distance(route[i], route[i + 1]) + instance->distance(route[j], route[j + 1]);

        // Calculate the potential new distance if we reverse the segment between i+1 and j
        double new_dist = instance->distance(route[i], route[j]) + instance->distance(route[i + 1], route[j + 1]);

        if (new_dist < current_dist) {
          // Perform the 2-opt swap by reversing the segment between i+1 and j
          std::reverse(route.begin() + i + 1, route.begin() + j + 1);

          // Check if the new route is feasible
          if (!check_load(route, instance, i) || !check_load(route, instance, j)) {
            auto new_sol = convert_to_solution(route);
            cye::patch_cargo_trivially(new_sol);
            auto new_cost = new_sol.cost();
            if (new_cost < cost) {
              solution = std::move(new_sol);
              cost = new_cost;
              route = convert_to_vector(solution);
              stop = false;
              found_improvement = true;
              continue;
            }
            // Revert if not better
            std::reverse(route.begin() + i + 1, route.begin() + j + 1);
            continue;
          } else {
            cost += new_dist - current_dist;
            stop = false;
            found_improvement = true;
          }
        }
      }
    }
    std::shuffle(indices.begin(), indices.end(), gen);
  }

  // Convert the final route back to a solution
  std::vector<size_t> new_base;
  new_base.reserve(route.size() + 1);
  for (auto it : route) {
    if (it != instance->depot_id()) {
      new_base.emplace_back(it);
    }
  }
  solution = cye::Solution(solution.instance_ptr(), std::move(new_base));
  solution.clear_patches();
  cye::patch_cargo_optimally(solution);
  return found_improvement;
}

bool DoFullMoveSearch(meta::RandomEngine &gen, cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  std::vector<size_t> route;
  for (auto it : solution.routes()) {
    route.emplace_back(it);
  }

  bool stop = false;
  bool found_improvement = false;
  auto cost = solution.cost();

  auto convert_to_solution = [&](std::vector<size_t> const &v) {
    std::vector<size_t> new_base;
    new_base.reserve(v.size() + 1);
    for (auto it : v) {
      if (it != instance->depot_id()) {
        new_base.emplace_back(it);
      }
    }
    return cye::Solution(solution.instance_ptr(), std::move(new_base));
  };

  while (!stop) {
    stop = true;
    for (size_t from = 1; from < route.size() - 1; ++from) {
      if (route[from] == instance->depot_id()) continue;

      for (size_t to = 1; to < route.size() - 1; ++to) {
        if (route[to] == instance->depot_id() || route[from] == instance->depot_id() || std::abs((int)from - (int)to) < 2) continue;

        auto original = route[from];
        auto prev_dist = neighbor_dist(route, from, instance) + neighbor_dist(route, to, instance);
        auto new_dist = instance->distance(route[from], route[to - 1]) + instance->distance(route[from], route[to]) +
                        instance->distance(route[to], route[to + 1]) + instance->distance(route[from - 1], route[from + 1]);

        if (new_dist + 1e-5 < prev_dist) {
          std::vector<size_t> temp_route = route;
          temp_route.erase(temp_route.begin() + from);
          temp_route.insert(temp_route.begin() + (to > from ? to - 1 : to), original);
          if (!check_load(temp_route, instance, to)) {
            auto new_sol = convert_to_solution(temp_route);
            cye::patch_cargo_trivially(new_sol);
            auto new_cost = new_sol.cost();
            if (new_cost < cost) {
              solution = std::move(new_sol);
              cost = new_cost;
              route = convert_to_vector(solution);
              stop = false;
              found_improvement = true;
              break;
            }
            continue;
          }

          route = std::move(temp_route);
          cost += new_dist - prev_dist;
          stop = false;
          found_improvement = true;
        }
      }
    }
  }

  std::vector<size_t> new_base;
  new_base.reserve(route.size() + 1);
  for (auto it : route) {
    if (it != instance->depot_id()) {
      new_base.emplace_back(it);
    }
  }
  solution = cye::Solution(solution.instance_ptr(), std::move(new_base));
  solution.clear_patches();
  cye::patch_cargo_optimally(solution);

  return found_improvement;
}

void DoMoveSearch(cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  auto &base = solution.base();
  const auto &cargo_patch = solution.get_patch(0);
  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind - 1;

    auto stop = false;
    while (!stop) {
      stop = true;
      for (auto l = route_begin; l < route_end; l++) {
        for (auto k = l + 2; k <= route_end; k++) {
          auto prev_dist = instance->distance(base[l], base[l + 1]) + instance->distance(base[k - 1], base[k]) +
                           instance->distance(base[k], (k == route_end) ? 0 : base[k + 1]);
          auto new_dist = instance->distance(base[l], base[k]) +
                          instance->distance(base[k - 1], (k == route_end) ? 0 : base[k + 1]) +
                          instance->distance(base[k], base[l + 1]);

          if (new_dist < prev_dist) {
            stop = false;
            auto erased_id = base[k];
            base.erase(base.begin() + k);
            base.insert(base.begin() + l + 1, erased_id);
          }
        }
      }
    }
  }
}

void DoMoveSearchFull(cye::EVRPIndividual &individual, cye::Instance const *instance) {
  auto &solution = individual.solution();
  auto &base = solution.base();
  const auto &cargo_patch = solution.get_patch(0);
  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto stop = false;
    while (!stop) {
      stop = true;
      for (auto l = 0UZ; l < base.size() - 1; l++) {
        for (auto k = l + 2; k < base.size(); k++) {
          auto prev_dist = instance->distance(base[l], base[l + 1]) + instance->distance(base[k - 1], base[k]) +
                           instance->distance(base[k], (k == base.size() - 1) ? 0 : base[k + 1]);
          auto new_dist = instance->distance(base[l], base[k]) +
                          instance->distance(base[k - 1], (k == base.size() - 1) ? 0 : base[k + 1]) +
                          instance->distance(base[k], base[l + 1]);

          if (new_dist < prev_dist) {
            stop = false;
            auto erased_id = base[k];
            base.erase(base.begin() + k);
            base.insert(base.begin() + l + 1, erased_id);
          }
        }
      }
    }
  }
  solution.clear_patches();
  cye::patch_cargo_optimally(solution);
}

}  // namespace

auto cye::TwoOptSearch::search(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  solution.clear_patches();

  cye::patch_cargo_optimally(solution);
  DoTwoOpt(individual, instance_.get());
  // cye::patch_energy_removal_heuristic(solution);
  energy_repair_->patch(solution, 151U);
  // cye::patch_energy_trivially(solution);
  individual.set_valid();

  return individual;
}

auto cye::VNSSearch::search(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  solution.clear_patches();

  cye::patch_cargo_optimally(solution);

  DoTwoOptFull(individual, instance_.get());
  DoFullSwapSearch(gen, individual, instance_.get());
  DoMoveSearchFull(individual, instance_.get());

  energy_repair_->patch(solution, 151U);
  // cye::patch_energy_optimal_heuristic(solution);
  individual.set_valid();

  return individual;
}

auto cye::SOTASearch::search(meta::RandomEngine &gen, cye::EVRPIndividual &&individual) -> cye::EVRPIndividual {
  auto &solution = individual.solution();
  solution.clear_patches();

  cye::patch_cargo_optimally(solution);

  //DoTwoOpt(individual, instance_.get());
  //DoMoveSearch(individual, instance_.get());
  while (DoFullTwoOptSearch(gen, individual, instance_.get()) ||
         DoFullSwapSearch(gen, individual, instance_.get()) ||
         DoFullMoveSearch(gen, individual, instance_.get())) {
    // Continue until no more improvements can be made
  }

  solution.clear_patches();
  cye::patch_cargo_optimally(solution);
  energy_repair_->patch(solution, 1001U);
  // cye::patch_energy_optimal_heuristic(solution);
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
  auto min_temp = 1e-2;

  for (auto i = 1UZ; i < cargo_patch.size(); i++) {
    auto route_begin = cargo_patch.changes()[i - 1].ind;
    auto route_end = cargo_patch.changes()[i].ind;

    auto stop = false;
    auto temp = 100.0;

    while (!stop) {
      stop = true;
      temp *= 0.25;

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
  solution.clear_patches();

  cye::patch_cargo_optimally(solution, static_cast<unsigned>(instance_->cargo_capacity()) + 1U);
  DoSwapSearch(individual, instance_.get());

  // cye::patch_energy_trivially(solution);
  energy_repair_->patch(solution, 151U);
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
  cye::patch_cargo_optimally(solution);

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
  cye::patch_cargo_optimally(solution);

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