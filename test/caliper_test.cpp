#include "caliper/caliper.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <atomic>

struct Config {
  size_t measurement_ind;
};

template <size_t N>
class TestApparatus {
 public:
  TestApparatus() {
    for (auto i = 0UZ; i < N; i++) {
      measurement_counters[i] = 0;
    }
  }

  auto measure(Config const &config) -> size_t { return measurement_counters[config.measurement_ind].fetch_add(1); }

  std::array<std::atomic_ullong, N> measurement_counters;
};

TEST(Caliper, mesurement) {
  constexpr auto measuremnts = 100000UZ;
  constexpr auto samples = 50UZ;

  auto test = TestApparatus<measuremnts>{};
  auto caliper = cali::Caliper<Config, size_t>([&](auto const &config) { return test.measure(config); });

  for (auto i = 0UZ; i < measuremnts; ++i) {
    caliper.add_measurement(samples, {i});
  }

  auto tasks = caliper.run(8);

  for (auto i = 0UZ; i < measuremnts; ++i) {
    EXPECT_EQ(test.measurement_counters[i], samples);

    std::ranges::sort(tasks[i].results);
    for (auto j = 0UZ; j < samples; ++j) {
      EXPECT_EQ(tasks[i].results[j], j);
    }
  }
}