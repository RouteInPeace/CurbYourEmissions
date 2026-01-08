#include "cye/repair_opt.hpp"
#include <algorithm>
#include <map>
#include <print>
#include <vector>

auto cye::SolutionCandidate::dominates(SolutionCandidate const &other) const -> bool {
  if (battery_used == other.battery_used && cargo_used == other.cargo_used && distance == other.distance) return false;

  return battery_used <= other.battery_used && cargo_used <= other.cargo_used && distance <= other.distance;
}

auto cye::ParetoFront::colapse() -> void {
  std::ranges::sort(front_, [](auto &a, auto &b) {
    if (a.distance == b.distance) {
      if (a.battery_used == b.battery_used) {
        return a.cargo_used < a.battery_used;
      }
      return a.battery_used < b.battery_used;
    }
    return a.distance < b.distance;
  });

  auto skyline = std::map<double, double>();
  auto new_front = std::vector<SolutionCandidate>();

  for (const auto &f : front_) {
    //std::println("{} {} {}", f.distance, f.battery_used, f.cargo_used);

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
}