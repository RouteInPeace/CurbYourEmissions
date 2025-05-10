#pragma once

#include "cye/solution.hpp"
#include "alns/random_engine.hpp"

namespace cye {
    Solution random_destroy(Solution &&solution, alns::RandomEngine &gen);
}