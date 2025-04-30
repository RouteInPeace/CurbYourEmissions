#include <gtest/gtest.h>
#include "core/instance.hpp"
#include "heuristics/init_heuristics.hpp"

TEST(Heuristics, NearestNeighbor) {
  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto instance = std::make_shared<cye::Instance>(path);
    auto solution = cye::nearest_neighbor(instance);

    EXPECT_TRUE(solution.is_valid());
  }
}