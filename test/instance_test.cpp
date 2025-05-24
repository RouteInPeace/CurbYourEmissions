#include "cye/instance.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(Instance, Distance) {
  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = cye::Instance(archive.root());
    auto &nodes = instance.nodes();

    for (auto node1_id = 0UZ; node1_id < instance.node_cnt(); ++node1_id) {
      for (auto node2_id = 0UZ; node2_id < instance.node_cnt(); ++node2_id) {
        auto delta_x = nodes[node1_id].x - nodes[node2_id].x;
        auto delta_y = nodes[node1_id].y - nodes[node2_id].y;
        auto dist = std::sqrt(delta_x * delta_x + delta_y * delta_y);

        EXPECT_EQ(dist, instance.distance(node1_id, node2_id));
      }
    }
  }
}