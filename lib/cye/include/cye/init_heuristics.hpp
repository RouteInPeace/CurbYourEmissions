#pragma once

#include <memory>
#include "instance.hpp"
#include "solution.hpp"

namespace cye {

[[nodiscard]] auto nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution;

}