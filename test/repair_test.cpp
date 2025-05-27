#include "cye/repair.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <print>
#include <random>
#include <utility>
#include <vector>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"
#include "cye/solution.hpp"

TEST(Repair, PatchCargoTrivially) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());

    auto routes = std::vector<size_t>();
    routes.push_back(instance->depot_id());
    for (auto c : instance->customer_ids()) {
      routes.push_back(c);
    }
    routes.push_back(instance->depot_id());

    for (auto i = 0UZ; i < 1000UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;

      auto solution = cye::Solution(instance, std::move(copy));
      cye::patch_cargo_trivially(solution);
      EXPECT_TRUE(solution.is_cargo_valid());
    }
  }
}

TEST(Repair, PatchCargoOptimally) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());

    auto routes = std::vector<size_t>();
    routes.push_back(instance->depot_id());
    for (auto c : instance->customer_ids()) {
      routes.push_back(c);
    }
    routes.push_back(instance->depot_id());

    for (auto i = 0UZ; i < 100UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;
      auto copy2 = routes;

      auto solution_opt = cye::Solution(instance, std::move(copy));
      auto solution_tr = cye::Solution(instance, std::move(copy2));
      cye::patch_cargo_optimally(solution_opt, static_cast<unsigned>(instance->cargo_capacity()) + 1u);
      cye::patch_cargo_trivially(solution_tr);

      EXPECT_TRUE(solution_opt.is_cargo_valid());
      EXPECT_TRUE(solution_tr.is_cargo_valid());

      EXPECT_LE(solution_opt.cost(), solution_tr.cost());
    }
  }
}

TEST(Repair, PatchEnergyTrivially) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());

    auto routes = std::vector<size_t>();
    routes.push_back(instance->depot_id());
    for (auto c : instance->customer_ids()) {
      routes.push_back(c);
    }
    routes.push_back(instance->depot_id());

    for (auto i = 0UZ; i < 100UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;

      auto solution = cye::Solution(instance, std::move(copy));
      cye::patch_cargo_trivially(solution);
      cye::patch_energy_trivially(solution);

      EXPECT_TRUE(solution.is_valid());
    }
  }
}

TEST(Repair, PatchEnergyOptimally) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());
    auto optimal_energy_repair = cye::OptimalEnergyRepair(instance);

    auto routes = std::vector<size_t>();
    routes.push_back(instance->depot_id());
    for (auto c : instance->customer_ids()) {
      routes.push_back(c);
    }
    routes.push_back(instance->depot_id());

    for (auto i = 0UZ; i < 10UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;

      auto solution_tr = cye::Solution(instance, std::move(copy));
      cye::patch_cargo_trivially(solution_tr);
      auto solution_opt = solution_tr;

      cye::patch_energy_trivially(solution_tr);
      optimal_energy_repair.patch(solution_opt, 101u);

      EXPECT_TRUE(solution_opt.is_valid());
      EXPECT_TRUE(solution_tr.is_valid());
      EXPECT_LE(solution_opt.cost() / solution_tr.cost(), 1.02f);
    }
  }
}

TEST(Repair, GreedyRepair) {
  std::mt19937 gen(16);

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  std::vector<size_t> initial = {0, 14, 16, 20, 18, 1, 12, 6, 7, 5, 9, 21, 15, 3, 4, 11, 13, 19, 2, 0};
  auto solution = cye::Solution(instance, std::move(initial), {8, 10, 17});

  auto repaired_solution = cye::greedy_repair(std::move(solution), gen);
  EXPECT_TRUE(repaired_solution.is_valid());

  auto expected = std::vector<size_t>{0, 14, 16, 20, 18, 1, 12, 6, 8, 10, 7, 5, 9, 21, 15, 3, 4, 11, 13, 19, 2, 17, 0};
  auto repaired = std::ranges::to<std::vector<size_t>>(repaired_solution.routes().base());
  ASSERT_EQ(repaired, expected);
}

TEST(Repair, GreedyRepairBestFirst) {
  std::mt19937 gen(8);

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  std::vector<size_t> initial = {0, 14, 16, 20, 18, 1, 12, 6, 7, 5, 9, 21, 15, 3, 4, 11, 13, 19, 2, 0};
  auto solution = cye::Solution(instance, std::move(initial), {8, 10, 17});

  auto repaired_solution = cye::greedy_repair_best_first(std::move(solution), gen);
  EXPECT_TRUE(repaired_solution.is_valid());

  auto expected = std::vector<size_t>{0, 14, 16, 20, 18, 1, 12, 6, 8, 10, 7, 5, 9, 21, 15, 3, 4, 11, 13, 19, 2, 17, 0};
  auto repaired = std::ranges::to<std::vector<size_t>>(repaired_solution.routes().base());
  ASSERT_EQ(repaired, expected);
}

TEST(Repair, RegretRepair) {
  std::mt19937 gen(8);

  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  std::vector<size_t> initial = {0, 14, 16, 20, 18, 1, 12, 6, 7, 5, 9, 21, 15, 3, 4, 11, 13, 19, 2, 0};
  auto solution = cye::Solution(instance, std::move(initial), {8, 10, 17});

  auto repaired_solution = cye::regret_repair(std::move(solution), gen, 2);
  EXPECT_TRUE(repaired_solution.is_valid());

  auto expected = std::vector<size_t>{0, 14, 16, 20, 18, 1, 17, 12, 8, 6, 7, 5, 9, 21, 15, 3, 4, 10, 11, 13, 19, 2, 0};
  auto repaired = std::ranges::to<std::vector<size_t>>(repaired_solution.routes().base());
  ASSERT_EQ(repaired, expected);
}

TEST(Repair, DPSparsity) {
  auto archive = serial::JSONArchive("dataset/json/X-n916-k207.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto solution = cye::nearest_neighbor(instance);
  solution.pop_patch();

  // for (auto x : solution.routes()) {
  //   std::cout << x << ' ';
  // }
  // std::cout << '\n';

  auto energy_repair = cye::OptimalEnergyRepair(instance);

  auto bin_cnt = 10001u;
  auto dp = energy_repair.fill_dp(solution, bin_cnt);

  auto heuristic_bound = std::vector<std::pair<unsigned, float>>();
  heuristic_bound.emplace_back(bin_cnt - 1u, 0.f);
  {
    auto copy = solution;
    cye::patch_energy_trivially(copy);
    auto dist = 0.f;
    auto energy = instance->battery_capacity();
    auto energy_per_bin = instance->battery_capacity() / static_cast<float>(bin_cnt - 1);
    auto previous_node_id = *copy.routes().begin();
    auto original_it = ++solution.routes().begin();

    for (auto it = ++copy.routes().begin(); it != copy.routes().end(); ++it) {
      auto current_node_id = *it;
      dist += instance->distance(previous_node_id, current_node_id);
      energy -= instance->energy_required(previous_node_id, current_node_id);

      if (instance->is_charging_station(current_node_id)) {
        energy = instance->battery_capacity();
      }

      if (current_node_id == *original_it) {
        auto energy_quant = static_cast<unsigned>(std::ceil(energy / energy_per_bin));
        heuristic_bound.emplace_back(energy_quant, dist);
        ++original_it;
      }

      previous_node_id = current_node_id;
    }
  }

  // for (auto [bin, dist] : heuristic_bound) {
  //   std::print("({:2},{:5.1f}) ", bin, dist);
  // }
  // std::cout << heuristic_bound.size() << ' ' << solution.visited_node_cnt() << '\n';
  // std::cout << "\n\n\n";

  // for (auto i = 0UZ; i < bin_cnt; ++i) {
  //   for (auto j = 0UZ; j < solution.visited_node_cnt(); ++j) {
  //     std::print("{:6.1f} ", dp[i][j].dist);
  //   }
  //   std::cout << '\n';
  // }

  // std::cout << "\n\n\n\n";
  auto inf = std::numeric_limits<float>::infinity();
  for (auto j = 0UZ; j < solution.visited_node_cnt(); ++j) {
    for (auto i = 0UZ; i < bin_cnt; ++i) {
      if (dp[i][j].dist != inf) {
        auto dominated = false;
        for (auto k = 0UZ; k < bin_cnt; ++k) {
          if (dp[k][j].dist != inf && i != k && k >= i && dp[k][j].dist <= dp[i][j].dist) {
            dominated = true;
            break;
          }
        }
        if (dominated) {
          dp[i][j].dist = inf;
        }
      }
    }
  }

  // for (auto i = 0UZ; i < bin_cnt; ++i) {
  //   for (auto j = 0UZ; j < solution.visited_node_cnt(); ++j) {
  //     std::print("{:6.1f} ", dp[i][j].dist);
  //   }
  //   std::cout << '\n';
  // }

  auto total_cnt = 0UZ;
  auto full_cnt = 0UZ;
  for (auto i = 0UZ; i < bin_cnt; ++i) {
    for (auto j = 0UZ; j < solution.visited_node_cnt(); ++j) {
      if (dp[i][j].dist != inf) {
        full_cnt++;
      }
      total_cnt++;
    }
  }

  std::cout << full_cnt << ' ' << total_cnt << ' '
            << static_cast<double>(full_cnt) / static_cast<double>(total_cnt) * 100.0 << "%\n";
}