#include "cye/repair.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <utility>
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

      EXPECT_LE(solution_opt.get_cost(), solution_tr.get_cost());
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

      // cye::patch_energy_trivially(solution_tr);
      optimal_energy_repair.patch(solution_opt, 101u);

      EXPECT_TRUE(solution_opt.is_valid());
      // EXPECT_TRUE(solution_tr.is_valid());
      // EXPECT_LE(solution_opt.get_cost() / solution_tr.get_cost(), 1.02f);
    }
  }
}

