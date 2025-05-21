#include <gtest/gtest.h>
#include "cye/init_heuristics.hpp"
#include "cye/instance.hpp"

TEST(Heuristics, NearestNeighbor) {
  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());
    auto solution = cye::nearest_neighbor(instance);

    // std::cout << path << ' ' << solution.get_cost() << '\n';

    EXPECT_TRUE(solution.is_valid());
  }
}