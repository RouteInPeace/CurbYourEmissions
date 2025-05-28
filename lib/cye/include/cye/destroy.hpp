#pragma once

#include "cye/solution.hpp"
#include "meta/common.hpp"

namespace cye {
Solution random_destroy(Solution &&solution, meta::RandomEngine &gen, double max_destroy_rate);
}