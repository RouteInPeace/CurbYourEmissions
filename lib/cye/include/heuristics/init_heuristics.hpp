#pragma once

#include <memory>
#include "core/instance.hpp"
#include "core/solution.hpp"

namespace cye {

[[nodiscard]] auto nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution;

}