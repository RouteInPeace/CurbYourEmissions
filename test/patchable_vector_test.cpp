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

TEST(PatchableVector, PostfixIncrement) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};

  auto it = vec.begin();
  EXPECT_EQ(*(it++), 0);
  EXPECT_EQ(*it, 1);
}

TEST(PatchableVector, PostfixDecrement) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};

  auto it = vec.begin();
  ++it;
  EXPECT_EQ(*(it--), 1);
  EXPECT_EQ(*it, 0);
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

TEST(PatchableVector, ConstIterator) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(1, 10);
  vec.add_patch(std::move(patch));

  const auto &ref = vec;

  auto result = std::vector<size_t>();
  for (auto it = ref.begin(); it != ref.end(); ++it) {
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
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto insertion_dist = std::uniform_int_distribution(0UZ, element_cnt);
    std::vector<std::pair<size_t, size_t>> changes;
    auto change_cnt = dist(gen);

    for (auto i = 0UZ; i < change_cnt; ++i) {
      changes.emplace_back(insertion_dist(gen), dist(gen));
    }

    std::ranges::sort(changes);
    for (auto [ind, value] : changes | std::views::reverse) {
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

    EXPECT_EQ(result, elements);
  }
}

TEST(PatchableVector, TwoPatches) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(0, 10);
  patch.add_change(0, 15);
  patch.add_change(1, 20);
  vec.add_patch(std::move(patch));

  auto patch2 = cye::Patch<size_t>();
  patch2.add_change(0, 30);
  patch2.add_change(1, 40);
  vec.add_patch(std::move(patch2));

  auto result = std::vector<size_t>();
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    result.push_back(*it);
  }

  auto expected = std::vector<size_t>{30, 10, 40, 15, 0, 20, 1, 2, 3, 4, 5};
  EXPECT_EQ(result, expected);
}

TEST(PatchableVector, MultiplePatchStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);
  auto patch_cnt_dist = std::uniform_int_distribution(2UZ, 5UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto patch_cnt = patch_cnt_dist(gen);
    for (auto p = 0UZ; p < patch_cnt; ++p) {
      auto insertion_dist = std::uniform_int_distribution(0UZ, elements.size());
      std::vector<std::pair<size_t, size_t>> changes;
      auto change_cnt = dist(gen);

      for (auto i = 0UZ; i < change_cnt; ++i) {
        changes.emplace_back(insertion_dist(gen), dist(gen));
      }

      std::ranges::sort(changes);
      for (auto [ind, value] : changes | std::views::reverse) {
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
      EXPECT_EQ(result, elements);
    }
  }
}

TEST(PatchableVector, Squash) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);
  auto patch_cnt_dist = std::uniform_int_distribution(2UZ, 5UZ);

  for (auto iter = 0UZ; iter < 1000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto patch_cnt = patch_cnt_dist(gen);
    for (auto p = 0UZ; p < patch_cnt; ++p) {
      auto insertion_dist = std::uniform_int_distribution(0UZ, elements.size() - 1UZ);
      std::vector<std::pair<size_t, size_t>> changes;
      auto change_cnt = dist(gen);

      for (auto i = 0UZ; i < change_cnt; ++i) {
        changes.emplace_back(insertion_dist(gen), dist(gen));
      }

      std::ranges::sort(changes);
      for (auto [ind, value] : changes | std::views::reverse) {
        elements.insert(elements.begin() + ind, value);
      }

      auto patch = cye::Patch<size_t>();
      for (auto [ind, value] : changes) {
        patch.add_change(ind, value);
      }
      patchable_vec.add_patch(std::move(patch));
    }

    patchable_vec.squash();
    auto result = std::vector<size_t>();
    for (auto it = patchable_vec.begin(); it != patchable_vec.end(); ++it) {
      result.push_back(*it);
    }
    EXPECT_EQ(result, elements);
  }
}

TEST(PatchableVector, DecrementWithoutPatches) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};

  auto prev_it = vec.begin();
  for (auto it = ++vec.begin(); it != vec.end(); ++it) {
    auto copy = it;
    --copy;
    EXPECT_EQ(prev_it, copy);
    EXPECT_EQ(*prev_it, *copy);
    prev_it = it;
  }
}

TEST(PatchableVector, DecrementWithOnePatch) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(0, 10);
  patch.add_change(0, 20);
  patch.add_change(1, 30);
  patch.add_change(1, 40);
  patch.add_change(3, 50);
  vec.add_patch(std::move(patch));

  auto prev_it = vec.begin();
  for (auto it = ++vec.begin(); it != vec.end(); ++it) {
    auto copy = it;
    --copy;
    EXPECT_EQ(prev_it, copy);
    EXPECT_EQ(*prev_it, *copy);
    prev_it = it;
  }
}

TEST(PatchableVector, DecrementWithTwoPatch) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(0, 10);
  patch.add_change(1, 20);
  vec.add_patch(std::move(patch));

  auto patch2 = cye::Patch<size_t>();
  patch2.add_change(0, 30);
  patch2.add_change(1, 40);
  patch2.add_change(2, 50);

  vec.add_patch(std::move(patch2));

  auto begin = vec.begin();

  auto prev_it = vec.begin();
  for (auto it = ++vec.begin(); it != vec.end(); ++it) {
    auto copy = it;
    --copy;
    EXPECT_EQ(prev_it, copy);
    EXPECT_EQ(*prev_it, *copy);
    prev_it = it;
  }
}

TEST(PatchableVector, DecrementWithTwoPatch2) {
  auto vec = cye::PatchableVector<size_t>{0, 1, 2, 3, 4, 5};
  auto patch = cye::Patch<size_t>();
  patch.add_change(1, 10);
  patch.add_change(2, 20);
  vec.add_patch(std::move(patch));

  auto patch2 = cye::Patch<size_t>();
  patch2.add_change(0, 30);
  vec.add_patch(std::move(patch2));

  auto begin = vec.begin();

  auto prev_it = vec.begin();
  for (auto it = ++vec.begin(); it != vec.end(); ++it) {
    auto copy = it;
    --copy;
    EXPECT_EQ(prev_it, copy);
    EXPECT_EQ(*prev_it, *copy);
    prev_it = it;
  }
}

TEST(PatchableVector, DecrementStress) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto dist = std::uniform_int_distribution(1UZ, 100UZ);
  auto patch_cnt_dist = std::uniform_int_distribution(0UZ, 5UZ);

  for (auto iter = 0UZ; iter < 10000UZ; ++iter) {
    auto elements = std::vector<size_t>();
    auto element_cnt = dist(gen);
    for (auto i = 0UZ; i < element_cnt; ++i) elements.push_back(dist(gen));

    auto copy = elements;
    auto patchable_vec = cye::PatchableVector<size_t>(std::move(copy));

    auto patch_cnt = patch_cnt_dist(gen);
    for (auto p = 0UZ; p < patch_cnt; ++p) {
      auto insertion_dist = std::uniform_int_distribution(0UZ, elements.size());
      std::vector<std::pair<size_t, size_t>> changes;
      auto change_cnt = dist(gen);

      for (auto i = 0UZ; i < change_cnt; ++i) {
        changes.emplace_back(insertion_dist(gen), dist(gen));
      }

      std::ranges::sort(changes);
      for (auto [ind, value] : changes | std::views::reverse) {
        elements.insert(elements.begin() + ind, value);
      }

      auto patch = cye::Patch<size_t>();
      for (auto [ind, value] : changes) {
        patch.add_change(ind, value);
      }
      patchable_vec.add_patch(std::move(patch));
    }

    auto prev_it = patchable_vec.begin();
    for (auto it = ++patchable_vec.begin(); it != patchable_vec.end(); ++it) {
      auto copy = it;
      --copy;
      EXPECT_EQ(prev_it, copy);
      EXPECT_EQ(*prev_it, *copy);
      prev_it = it;
    }
  }
}