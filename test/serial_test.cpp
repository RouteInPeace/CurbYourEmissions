#include <gtest/gtest.h>
#include <string_view>
#include <vector>
#include "serial/json_archive.hpp"

struct Vec2f {
  float x;
  float y;

  template <serial::Value V>
  auto write(V v) const {
    v.emplace("x", x);
    v.emplace("y", y);
  }
};

TEST(Serial, ReadBasicTypes) {
  auto archive = serial::JSONArchive("test/data/test.json");
  auto root = archive.root();

  EXPECT_EQ(root["int"].get<int>(), 1);
  EXPECT_EQ(root["size_t"].get<size_t>(), 1);
  EXPECT_EQ(root["float"].get<float>(), 1);
  EXPECT_EQ(root["string"].get<std::string_view>(), std::string_view("string"));
  EXPECT_EQ(root["simpleArray"].get<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
}

TEST(Serial, WriteBasicTypes) {
  auto archive = serial::JSONArchive();
  auto root = archive.root();

  root.emplace("int", 1);
  root.emplace("float", 1.f);
  root.emplace("string", "string");
  root.emplace("array", std::vector{1, 2, 3, 4, 5});
  root.emplace("Vec2f", Vec2f{1.f, 2.f});
  root.emplace("ArrayOfVec2fs", std::vector{Vec2f{1.f, 2.f}, Vec2f{1.f, 2.f}, Vec2f{1.f, 2.f}});

  std::cout << archive.to_string() << '\n';
}

TEST(Serial, NonExistingValue) {
  auto archive = serial::JSONArchive("test/data/test.json");
  auto root = archive.root();

  EXPECT_ANY_THROW(auto _ = root["*"].get<int>());
  EXPECT_ANY_THROW(auto _ = root["*"].get<size_t>());
  EXPECT_ANY_THROW(auto _ = root["*"].get<float>());
  EXPECT_ANY_THROW(auto _ = root["*"].get<std::string_view>());
  EXPECT_ANY_THROW(auto _ = root["*"].get<std::vector<int>>());
}

TEST(Serial, WrongType) {
  auto archive = serial::JSONArchive("test/data/test.json");
  auto root = archive.root();

  EXPECT_ANY_THROW(auto _ = root["string"].get<int>());
  EXPECT_ANY_THROW(auto _ = root["string"].get<size_t>());
  EXPECT_ANY_THROW(auto _ = root["string"].get<float>());
  EXPECT_ANY_THROW(auto _ = root["int"].get<std::string_view>());
  EXPECT_ANY_THROW(auto _ = root["string"].get<std::vector<int>>());
}

TEST(Serial, GetOr) {
  auto archive = serial::JSONArchive("test/data/test.json");
  auto root = archive.root();

  EXPECT_EQ(root["*"].get_or<int>(1), 1);
  EXPECT_EQ(root["string"].get_or<int>(1), 1);

  EXPECT_EQ(root["*"].get_or<size_t>(1), 1);
  EXPECT_EQ(root["string"].get_or<size_t>(1), 1);

  EXPECT_EQ(root["*"].get_or<float>(1.f), 1.f);
  EXPECT_EQ(root["string"].get_or<float>(1.f), 1.f);

  EXPECT_EQ(root["*"].get_or<std::string_view>("string"), std::string_view("string"));
  EXPECT_EQ(root["int"].get_or<std::string_view>("string"), std::string_view("string"));
}
