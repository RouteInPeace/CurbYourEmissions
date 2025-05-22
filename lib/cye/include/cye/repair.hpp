#pragma once

#include <cstddef>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>
#include "alns/random_engine.hpp"
#include "cye/instance.hpp"
#include "cye/solution.hpp"

namespace cye {

auto patch_cargo_optimally(Solution &solution, unsigned bin_cnt) -> void;
auto patch_cargo_trivially(Solution &solution) -> void;
auto patch_energy_trivially(Solution &solution) -> void;
auto patch_endpoint_depots(Solution &solution) -> void;

Solution greedy_repair(Solution &&solution, alns::RandomEngine &gen);
Solution greedy_repair_best_first(Solution &&solution, alns::RandomEngine &gen);
Solution regret_repair(Solution &&solution, alns::RandomEngine &gen, size_t k);

class OptimalEnergyRepair {
 public:
  OptimalEnergyRepair(std::shared_ptr<Instance> instance);
  auto patch(Solution &solution, unsigned bin_cnt) -> void;

 private:
  auto compute_cs_dist_mat_() -> void;
  auto find_between_(size_t start_node_id, size_t goal_node_id) -> std::optional<std::pair<std::vector<size_t>, float>>;
  auto reset_() -> void;

  std::shared_ptr<Instance> instance_;
  std::vector<std::vector<float>> cs_dist_mat_;

  struct VisitedNode_ {
    float g;
    size_t parent;
  };

  struct UnvisitedNode_ {
    size_t node_id;
    size_t parent;
    float g;
    float h;
  };

  constexpr static auto cmp_ = [](UnvisitedNode_ const &a, UnvisitedNode_ const &b) {
    return (a.g + a.h) > (b.g + b.h);
  };

  std::unordered_map<size_t, VisitedNode_> visited_;
  std::priority_queue<UnvisitedNode_, std::vector<UnvisitedNode_>, decltype(cmp_)> unvisited_queue_;
};

}  // namespace cye