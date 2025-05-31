#pragma once

#include <memory>
#include "instance.hpp"
#include "meta/common.hpp"
#include "solution.hpp"

namespace cye {

[[nodiscard]] auto random_customer_permutation(meta::RandomEngine &gen, std::shared_ptr<Instance> instance) -> Solution;
[[nodiscard]] auto nearest_neighbor(std::shared_ptr<Instance> instance) -> Solution;
[[nodiscard]] auto stochastic_rank_nearest_neighbor(meta::RandomEngine &gen, std::shared_ptr<Instance> instance,
                                                    size_t k) -> Solution;
[[nodiscard]] auto stochastic_nearest_neighbor(meta::RandomEngine &gen, std::shared_ptr<Instance> instance) -> Solution;

}  // namespace cye