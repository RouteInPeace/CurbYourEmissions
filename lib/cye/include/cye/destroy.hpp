#pragma once

#include "alns/random_engine.hpp"
#include "cye/solution.hpp"

namespace cye {
Solution random_destroy(Solution &&solution, alns::RandomEngine &gen, double max_destroy_rate);
}