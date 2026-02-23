#include "cye/repair_opt.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <print>
#include <vector>
#include "cye/instance.hpp"
#include "cye/repair.hpp"

cye::SolutionCandidate::SolutionCandidate()
    : distance(0.0),
      battery_used(0.0),
      cargo_used(0.0),
      prev(std::numeric_limits<size_t>::max()),
      cs_ind_in(0),
      cs_ind_out(0),
      jump_case(JumpCase::None) {}
cye::SolutionCandidate::SolutionCandidate(double distance_, double battery_used_, double cargo_used_)
    : distance(distance_),
      battery_used(battery_used_),
      cargo_used(cargo_used_),
      prev(std::numeric_limits<size_t>::max()),
      cs_ind_in(0),
      cs_ind_out(0),
      jump_case(JumpCase::None) {}

auto cye::SolutionCandidate::dominates(SolutionCandidate const &other) const -> bool {
  if (battery_used == other.battery_used && cargo_used == other.cargo_used && distance == other.distance) return false;

  return battery_used <= other.battery_used && cargo_used <= other.cargo_used && distance <= other.distance;
}

auto cye::ParetoFront::colapse() -> void {
  std::ranges::sort(front_, [](const auto &a, const auto &b) {
    if (a.distance != b.distance) return a.distance < b.distance;
    if (a.battery_used != b.battery_used) return a.battery_used < b.battery_used;

    return a.cargo_used < b.cargo_used;
  });

  auto skyline = std::map<double, double>();
  auto new_front = std::vector<SolutionCandidate>();

  for (const auto &f : front_) {
    // std::println("{} {} {}", f.distance, f.battery_used, f.cargo_used);

    auto it = skyline.upper_bound(f.battery_used);
    if (!skyline.empty() && it != skyline.begin()) {
      auto prev = std::prev(it);
      if (prev->second <= f.cargo_used) {
        continue;
      }
    }

    new_front.push_back(f);

    while (it != skyline.end() && it->second >= f.cargo_used) {
      it = skyline.erase(it);
    }

    skyline[f.battery_used] = f.cargo_used;
  }

  front_ = std::move(new_front);
  valid_ = true;
}

cye::CsMatrix::CsMatrix(std::shared_ptr<Instance> instance)
    : cs_cnt_(instance->charging_station_cnt() + 1),
      instance_(instance),
      dist_mat_(cs_cnt_, std::vector(cs_cnt_, std::numeric_limits<double>::infinity())),
      prev_(cs_cnt_, std::vector(cs_cnt_, std::numeric_limits<size_t>::max())) {
  floyd_warshall_();
}

auto cye::CsMatrix::floyd_warshall_() -> void {
  for (auto i = 0UZ; i < cs_cnt_; ++i) {
    dist_mat_[i][i] = 0.0;
    prev_[i][i] = i;

    for (auto j = i + 1; j < cs_cnt_; ++j) {
      auto node1 = ind_to_id(i);
      auto node2 = ind_to_id(j);

      auto distance = instance_->distance(node1, node2);
      if (instance_->energy_required(node1, node2) <= instance_->battery_capacity()) {
        dist_mat_[i][j] = distance;
        dist_mat_[j][i] = distance;
        prev_[i][j] = i;
        prev_[j][i] = j;
      }
    }
  }

  for (auto k = 0UZ; k < cs_cnt_; ++k) {
    for (auto i = 0UZ; i < cs_cnt_; ++i) {
      for (auto j = 0UZ; j < cs_cnt_; ++j) {
        if (dist_mat_[i][j] > dist_mat_[i][k] + dist_mat_[k][j]) {
          dist_mat_[i][j] = dist_mat_[i][k] + dist_mat_[k][j];
          prev_[i][j] = prev_[k][j];
        }
      }
    }
  }
}

auto cye::CsMatrix::trace(Patch<size_t> &patch, size_t pos, size_t ind1, size_t ind2) -> void {
  if (prev_[ind1][ind2] == std::numeric_limits<size_t>::max()) {
    return;
  }

  patch.add_change(pos, ind_to_id(ind2));
  while (ind1 != ind2) {
    ind2 = prev_[ind1][ind2];
    patch.add_change(pos, ind_to_id(ind2));
  }
}

cye::OptimalRepair::OptimalRepair(std::shared_ptr<Instance> instance)
    : instance_(instance), cs_mat_(instance), fronts_(instance_->customer_cnt() + 2) {}

auto cye::OptimalRepair::patch(Solution &solution, double upper_bound) -> void {
  for (auto &front : fronts_) front.clear();
  forward_pass_(solution, upper_bound);
  backward_pass_(solution);
}

auto cye::OptimalRepair::forward_pass_(Solution const &solution, double upper_bound) -> void {
  const auto &perm = solution.base();

  fronts_[0].emplace(0.0, 0.0, 0.0);
  fronts_[0].colapse();

  for (auto i = 0UZ; i <= instance_->customer_cnt(); ++i) {
    auto prev_node = i == 0 ? instance_->depot_id() : perm[i - 1];
    auto curr_node = i == instance_->customer_cnt() ? instance_->depot_id() : perm[i];

    auto &prev_front = fronts_[i];
    auto &curr_front = fronts_[i + 1];

    for (auto sol_id = 0UZ; sol_id < prev_front.size(); ++sol_id) {
      const auto &sol = prev_front[sol_id];

      // Option 1 - Direct rute
      {
        auto prop = sol;
        prop.distance += instance_->distance(prev_node, curr_node);
        prop.cargo_used += instance_->demand(curr_node);
        prop.battery_used += instance_->energy_required(prev_node, curr_node);
        prop.prev = sol_id;
        prop.jump_case = JumpCase::Direct;

        if (prop.cargo_used <= instance_->cargo_capacity() && prop.battery_used <= instance_->battery_capacity() &&
            prop.distance <= upper_bound) {
          curr_front.emplace(prop);
        }
      }

      for (auto cs_ind_in = 0UZ; cs_ind_in < cs_mat_.size(); ++cs_ind_in) {
        // Skip this entry cs if we can't reach it with the remaining battery
        auto cs_id_in = cs_mat_.ind_to_id(cs_ind_in);
        if (instance_->energy_required(prev_node, cs_id_in) + sol.battery_used > instance_->battery_capacity()) {
          continue;
        }

        for (auto cs_ind_out = 0UZ; cs_ind_out < cs_mat_.size(); ++cs_ind_out) {
          // Skip this exit cs if we cannot reach the next node. Remember that we leave the cs with a full battery
          auto cs_id_out = cs_mat_.ind_to_id(cs_ind_out);
          if (instance_->energy_required(cs_id_out, curr_node) > instance_->battery_capacity()) {
            continue;
          }

          // Option 2 - Charging detour
          {
            auto prop = sol;
            prop.distance += instance_->distance(prev_node, cs_id_in);
            prop.distance += cs_mat_.distance(cs_ind_in, cs_ind_out);
            prop.distance += instance_->distance(cs_id_out, curr_node);
            prop.battery_used = instance_->energy_required(cs_id_out, curr_node);
            prop.cargo_used += instance_->demand(curr_node);
            prop.prev = sol_id;
            prop.jump_case = JumpCase::Charging;
            prop.cs_ind_in = cs_ind_in;
            prop.cs_ind_out = cs_ind_out;

            if (prop.cargo_used <= instance_->cargo_capacity() && prop.battery_used <= instance_->battery_capacity() &&
                prop.distance <= upper_bound) {
              curr_front.emplace(prop);
            }
          }

          // Option 3 - Depot detour
          {
            auto prop = sol;
            prop.distance += instance_->distance(prev_node, cs_id_in);
            prop.distance += cs_mat_.distance(cs_ind_in, 0);
            prop.distance += cs_mat_.distance(0, cs_ind_out);
            prop.distance += instance_->distance(cs_id_out, curr_node);
            prop.battery_used = instance_->energy_required(cs_id_out, curr_node);
            prop.cargo_used = instance_->demand(curr_node);
            prop.prev = sol_id;
            prop.jump_case = JumpCase::Depot;
            prop.cs_ind_in = cs_ind_in;
            prop.cs_ind_out = cs_ind_out;

            if (prop.cargo_used <= instance_->cargo_capacity() && prop.battery_used <= instance_->battery_capacity() &&
                prop.distance <= upper_bound) {
              curr_front.emplace(prop);
            }
          }
        }
      }
    }
    // Remove dominated solutions
    // auto s = curr_front.size();
    curr_front.colapse();
    // std::println("{} {}", s, curr_front.size());
  }
}

auto cye::OptimalRepair::backward_pass_(Solution &solution) -> void {
  const auto &perm = solution.base();

  auto sol_ind = 0;
  auto &last_front = fronts_.back();
  for (auto i = 0UZ; i < last_front.size(); ++i) {
    if (last_front[i].distance < last_front[sol_ind].distance) {
      sol_ind = i;
    }
  }

  auto patch = Patch<size_t>();
  patch.add_change(perm.size(), instance_->depot_id());

  for (size_t i = fronts_.size() - 1; i >= 1; --i) {
    const auto &sol = fronts_[i][sol_ind];

    if (sol.jump_case == JumpCase::Charging) {
      cs_mat_.trace(patch, i - 1, sol.cs_ind_in, sol.cs_ind_out);
    } else if (sol.jump_case == JumpCase::Depot) {
      cs_mat_.trace(patch, i - 1, instance_->depot_id(), sol.cs_ind_out);
      cs_mat_.trace(patch, i - 1, sol.cs_ind_in, instance_->depot_id());
      // patch.pop_back();
    }
    sol_ind = sol.prev;
  }

  patch.add_change(0, instance_->depot_id());
  patch.reverse();

  solution.add_patch(std::move(patch));
}

auto cye::ParetoFront2D::colapse() -> void {
  std::ranges::sort(front_, [](const auto &a, const auto &b) {
    if (a.distance != b.distance) return a.distance < b.distance;
    if (a.battery_used != b.battery_used) return a.battery_used < b.battery_used;

    return a.cargo_used < b.cargo_used;
  });

  auto new_front = std::vector<SolutionCandidate>();
  auto min_battery = std::numeric_limits<double>::infinity();
  for (const auto &f : front_) {
    if (f.battery_used < min_battery) {
      new_front.push_back(f);
      min_battery = f.battery_used;
    }
  }

  front_ = std::move(new_front);
  valid_ = true;
}

cye::BatteryDecode::BatteryDecode(std::shared_ptr<Instance> instance) : instance_(instance), cs_mat_(instance) {}

auto cye::BatteryDecode::decode(Solution &solution, double upper_bound) -> void {
  fronts_.resize(solution.routes().size());
  for (auto &front : fronts_) front.clear();
  forward_pass_(solution, upper_bound);
  backward_pass_(solution);
}

auto cye::BatteryDecode::forward_pass_(Solution const &solution, double upper_bound) -> void {
  fronts_[0].emplace(0.0, 0.0, 0.0);
  fronts_[0].colapse();

  auto prev_node = *solution.routes().begin();
  for (auto curr_node : solution.routes() | std::views::drop(1)) {
    auto &prev_front = fronts_[i - 1];
    auto &curr_front = fronts_[i];

    for (auto sol_id = 0UZ; sol_id < prev_front.size(); ++sol_id) {
      const auto &sol = prev_front[sol_id];

      // Option 1 - Direct rute
      {
        auto prop = sol;
        prop.distance += instance_->distance(prev_node, curr_node);
        prop.cargo_used += instance_->demand(curr_node);
        prop.battery_used += instance_->energy_required(prev_node, curr_node);
        prop.prev = sol_id;
        prop.jump_case = JumpCase::Direct;

        if (curr_node == instance_->depot_id()) {
          prop.battery_used = 0;
          prop.cargo_used = 0;
        }

        if (prop.cargo_used <= instance_->cargo_capacity() && prop.battery_used <= instance_->battery_capacity() &&
            prop.distance <= upper_bound) {
          curr_front.emplace(prop);
        }
      }

      for (auto cs_ind_in = 0UZ; cs_ind_in < cs_mat_.size(); ++cs_ind_in) {
        // Skip this entry cs if we can't reach it with the remaining battery
        auto cs_id_in = cs_mat_.ind_to_id(cs_ind_in);
        if (instance_->energy_required(prev_node, cs_id_in) + sol.battery_used > instance_->battery_capacity()) {
          continue;
        }

        for (auto cs_ind_out = 0UZ; cs_ind_out < cs_mat_.size(); ++cs_ind_out) {
          // Skip this exit cs if we cannot reach the next node. Remember that we leave the cs with a full battery
          auto cs_id_out = cs_mat_.ind_to_id(cs_ind_out);
          if (instance_->energy_required(cs_id_out, curr_node) > instance_->battery_capacity()) {
            continue;
          }

          // Option 2 - Charging detour
          {
            auto prop = sol;
            prop.distance += instance_->distance(prev_node, cs_id_in);
            prop.distance += cs_mat_.distance(cs_ind_in, cs_ind_out);
            prop.distance += instance_->distance(cs_id_out, curr_node);
            prop.battery_used = instance_->energy_required(cs_id_out, curr_node);
            prop.cargo_used += instance_->demand(curr_node);
            prop.prev = sol_id;
            prop.jump_case = JumpCase::Charging;
            prop.cs_ind_in = cs_ind_in;
            prop.cs_ind_out = cs_ind_out;

            if (curr_node == instance_->depot_id()) {
              prop.battery_used = 0;
              prop.cargo_used = 0;
            }

            if (prop.cargo_used <= instance_->cargo_capacity() && prop.battery_used <= instance_->battery_capacity() &&
                prop.distance <= upper_bound) {
              curr_front.emplace(prop);
            }
          }
        }
      }
    }
    // Remove dominated solutions
    curr_front.colapse();
    prev_node = curr_node;
  }
}

auto cye::BatteryDecode::backward_pass_(Solution &solution) -> void {
  const auto &perm = solution.base();

  auto sol_ind = 0;
  auto &last_front = fronts_.back();
  for (auto i = 0UZ; i < last_front.size(); ++i) {
    if (last_front[i].distance < last_front[sol_ind].distance) {
      sol_ind = i;
    }
  }

  auto patch = Patch<size_t>();
  patch.add_change(perm.size(), instance_->depot_id());

  for (size_t i = fronts_.size() - 1; i >= 1; --i) {
    const auto &sol = fronts_[i][sol_ind];

    if (sol.jump_case == JumpCase::Charging) {
      cs_mat_.trace(patch, i - 1, sol.cs_ind_in, sol.cs_ind_out);
    } else if (sol.jump_case == JumpCase::Depot) {
      cs_mat_.trace(patch, i - 1, instance_->depot_id(), sol.cs_ind_out);
      cs_mat_.trace(patch, i - 1, sol.cs_ind_in, instance_->depot_id());
      // patch.pop_back();
    }
    sol_ind = sol.prev;
  }

  patch.add_change(0, instance_->depot_id());
  patch.reverse();

  solution.add_patch(std::move(patch));
}