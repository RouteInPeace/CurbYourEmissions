#include "cye/repair_opt.hpp"
#include <gtest/gtest.h>
#include <random>
#include <ranges>
#include <vector>
#include "cye/solution.hpp"

TEST(RepairOpt, Dominates) {
  auto s1 = cye::SolutionCandidate{1, 1, 1};
  auto s2 = cye::SolutionCandidate{3, 3, 3};
  auto s3 = cye::SolutionCandidate{3, 2, 2};

  EXPECT_TRUE(s1.dominates(s2));
  EXPECT_FALSE(s1.is_dominated_by(s2));

  EXPECT_FALSE(s1.dominates(s1));
  EXPECT_FALSE(s1.is_dominated_by(s1));

  EXPECT_FALSE(s2.dominates(s3));
  EXPECT_FALSE(s3.is_dominated_by(s2));
}

TEST(RepairOpt, ParetoFront) {
  std::random_device rd;
  std::mt19937 gen(rd());

  const auto point_cnt = 1000;
  const auto iter = 1000;
  auto dist = std::uniform_real_distribution(0.0, 10.0);

  for (auto i = 0; i < iter; ++i) {
    // Generate random points
    std::vector<cye::SolutionCandidate> points;
    for (auto j = 0; j < point_cnt; ++j) {
      points.emplace_back(dist(gen), dist(gen), dist(gen));
    }

    // Brute force sol
    std::vector<cye::SolutionCandidate> brute_sol;
    for (const auto &p1 : points) {
      bool non_dominated = true;

      for (const auto &p2 : points) {
        if (p1.is_dominated_by(p2)) {
          non_dominated = false;
          break;
        }
      }

      if (non_dominated) {
        brute_sol.push_back(p1);
      }
    }

    // Sort for comparison
    std::ranges::sort(brute_sol, [](auto &a, auto &b) {
      if (a.distance == b.distance) {
        if (a.battery_used == b.battery_used) {
          return a.cargo_used < a.battery_used;
        }
        return a.battery_used < b.battery_used;
      }
      return a.distance < b.distance;
    });

    // Solve
    auto front = cye::ParetoFront();
    for (const auto &p : points) front.emplace(p);
    front.colapse();

    EXPECT_EQ(front.size(), brute_sol.size());

    // Verify
    for (const auto &[p1, p2] : std::ranges::zip_view(brute_sol, front)) {
      EXPECT_EQ(p1.distance, p2.distance);
      EXPECT_EQ(p1.battery_used, p2.battery_used);
      EXPECT_EQ(p1.cargo_used, p2.cargo_used);
    }
  }
}