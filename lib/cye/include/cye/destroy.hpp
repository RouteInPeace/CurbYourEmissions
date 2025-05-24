#pragma once

#include "meta/common.hpp"
#include "cye/solution.hpp"

namespace cye {
Solution random_destroy(Solution &&solution, meta::RandomEngine &gen, double max_destroy_rate);
}