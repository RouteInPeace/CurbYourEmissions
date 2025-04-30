#include "core/solution.hpp"
#include <gtest/gtest.h>
#include "core/instance.hpp"

TEST(SolutionTest, ValidRoutes) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0,  9,  7,  6, 3,  0,  5, 8,  11, 28, 12, 19, 17, 18, 0, 16,
                                20, 29, 13, 1, 14, 24, 0, 15, 21, 24, 10, 0,  4,  2,  0};

  EXPECT_TRUE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, StartNotAtDepot) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {9,  7,  6,  3, 0,  5,  8, 11, 28, 12, 19, 17, 18, 0, 16,
                                20, 29, 13, 1, 14, 24, 0, 15, 21, 24, 10, 0,  4,  2, 0};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, FinishNotAtDepot) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0,  9,  7,  6,  3, 0,  5,  8, 11, 28, 12, 19, 17, 18, 0,
                                16, 20, 29, 13, 1, 14, 24, 0, 15, 21, 24, 10, 0,  4,  2};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, DidNotVisitEverybody) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0, 9, 7, 6, 3, 0};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, VisitedMultipleTimes) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0,  9,  7, 6,  3,  0, 5,  8,  11, 28, 12, 19, 17, 18, 0, 16, 20,
                                29, 13, 1, 14, 24, 0, 15, 21, 24, 10, 0,  4,  2,  0,  9, 0};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, NotEnoughCapacity) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0,  9,  7,  6,  3, 0,  5,  8, 11, 28, 12, 19, 17, 18, 0,
                                16, 20, 29, 13, 1, 14, 24, 0, 15, 21, 24, 10, 4,  2,  0};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}

TEST(SolutionTest, NotEnoughEnergy) {
  auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
  std::vector<size_t> routes = {0,  9,  7,  6,  3, 0,  5, 8,  11, 28, 12, 19, 17, 18, 0,
                                16, 20, 29, 13, 1, 14, 0, 15, 21, 24, 10, 0,  4,  2,  0};

  EXPECT_FALSE(cye::Solution(instance, std::move(routes)).is_valid());
}
