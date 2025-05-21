#include "cye/patchable_vector.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstddef>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

TEST(PatchableVector, Empty) {
  auto vec = cye::PatchableVector<size_t>();

  EXPECT_EQ(vec.begin(), vec.end());
}

TEST(PatchableVector, NoPatchesStress) {
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

TEST(PatchableVector, NoPatches) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{0, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, SimplePatch) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(1, 10);
  vec.add_patch(std::move(patch));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{0, 10, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, FrontInsert) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(0, 10);
  vec.add_patch(std::move(patch));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{10, 0, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, BackInsert) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(6, 10);
  vec.add_patch(std::move(patch));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{0, 1, 2, 3, 4, 5, 10};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, SameIndex) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(1, 10);
  patch.add_change(1, 20);
  vec.add_patch(std::move(patch));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{0, 10, 20, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, OneAfterTheOther) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(0, 10);
  patch.add_change(0, 15);
  patch.add_change(1, 20);
  vec.add_patch(std::move(patch));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{10, 15, 0, 20, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, OnePatchStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(3);

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));
    // for (auto x : elements) std::cout << x << ' ';
    // std::cout << '\n';

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto insertion_dist = std::uniform_int_distribution(0UZ, element_cnt - 1UZ);
    std::vector<std::pair<size_t, size_t>> changes;
    auto change_cnt = dist(gen);

    for (auto i = 0UZ; i < change_cnt; ++i) {
      changes.emplace_back(insertion_dist(gen), dist(gen));
    }

    std::ranges::sort(changes);
    for (auto [ind, value] : changes | std::views::reverse) {
      // std::cout << ind << ' ' << value << '\n';
      elements.insert(elements.begin() + ind, value);
    }

    auto patch = cye::Patch<size_t>();
    for (auto [ind, value] : changes) {
      patch.add_change(ind, value);
    }
    patchable_vec.add_patch(std::move(patch));

    auto result = std::vector<size_t>();
    for (auto it = patchable_vec.begin(); it != patchable_vec.end(); ++it) {
      result.push_back(*it);
    }

    // std::cout << "\n\n";

    EXPECT_EQ(result, elements);
  }
}