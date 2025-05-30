#pragma once

#include <cstddef>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>
#include "cye/instance.hpp"
#include "cye/solution.hpp"

namespace cye {

auto patch_cargo_optimally(Solution &solution, unsigned bin_cnt) -> void;
inline auto patch_cargo_optimally(Solution &solution) -> void {
  patch_cargo_optimally(solution, static_cast<unsigned>(solution.instance().cargo_capacity()) + 1u);
}

auto patch_cargo_trivially(Solution &solution) -> void;
auto patch_energy_trivially(Solution &solution) -> void;

struct DPCell {
  DPCell()
      : dist(std::numeric_limits<float>::infinity()),
        prev(0),
        entry_ind(std::numeric_limits<uint16_t>::max()),
        exit_ind(std::numeric_limits<uint16_t>::max()) {}

  float dist;
  unsigned prev;
  uint16_t entry_ind;
  uint16_t exit_ind;
};

class OptimalEnergyRepair {
 public:
  OptimalEnergyRepair(std::shared_ptr<Instance> instance);
  auto patch(Solution &solution, unsigned bin_cnt) -> void;
  auto fill_dp(Solution &solution, unsigned bin_cnt) -> std::vector<std::vector<DPCell>>;

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