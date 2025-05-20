#include "cye/patchable_vector.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

TEST(PatchableVector, Empty) {
  auto vec = cye::PatchableVector<size_t>();

  EXPECT_EQ(vec.begin(), vec.end());
}

TEST(PatchableVector, NoPatches) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(0UZ, 1000UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto result = std::vector<size_t>();

    for (auto it = patchable_vec.begin(); it != patchable_vec.end(); ++it) {
      result.push_back(*it);
    }

    EXPECT_EQ(result, elements);
  }
}

TEST(PatchableVector, SimplePatch) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(1, 10);
  vec.add_patch(std::move(patch));

  for (auto it = vec.begin(); it != vec.end(); ++it) {
    std::cout << *it << ' ';
  }
  std::cout << '\n';
}

TEST(PatchableVector, OnePatch) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(0UZ, 1000UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto insertion_dist = std::uniform_int_distribution(0UZ, 20UZ);
    auto patch = cye::Patch<size_t>();

    auto ind = 0UZ;
    while (true) {
      ind += insertion_dist(gen);
      if (ind >= elements.size()) {
        break;
      }
      auto value = insertion_dist(gen);

      patch.add_change(ind, value);
      elements.insert(elements.begin() + ind, value);
    }
    patchable_vec.add_patch(std::move(patch));

    auto result = std::vector<size_t>();
    for (auto it = patchable_vec.begin(); it != patchable_vec.end(); ++it) {
      result.push_back(*it);
    }

    EXPECT_EQ(result, elements);
  }
}