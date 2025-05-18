#include "cye/repair.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include "cye/instance.hpp"
#include "cye/solution.hpp"

TEST(Repair, FixCargoViolationsTrivially) {
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

    for (auto i = 0UZ; i < 10000UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;

      auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));
      EXPECT_TRUE(solution.is_cargo_valid());
    }
  }
}

TEST(Repair, FixCargoViolationsOptimally) {
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

      auto solution_opt = cye::repair_cargo_violations_optimally(
          cye::Solution(instance, std::move(copy)), static_cast<unsigned>(instance->cargo_capacity()) + 1u);
      auto solution_tr = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy2)));
      EXPECT_TRUE(solution_opt.is_cargo_valid());
      EXPECT_TRUE(solution_tr.is_cargo_valid());

      EXPECT_LE(solution_opt.get_cost(), solution_tr.get_cost());
    }
  }
}

TEST(Repair, FixEnergyViolationsTrivially) {
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

      auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));
      auto solution2 = cye::repair_energy_violations_trivially(std::move(solution));
      EXPECT_TRUE(solution2.is_valid());
    }
  }
}

TEST(Repair, FixEnergyViolationsOptimally) {
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

    std::cout << path << '\n';

    for (auto i = 0UZ; i < 100UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;

      auto solution = cye::repair_cargo_violations_trivially(cye::Solution(instance, std::move(copy)));
      solution = optimal_energy_repair.repair(std::move(solution), 11u);
      EXPECT_TRUE(solution.is_valid());
    }
  }
}