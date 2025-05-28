#include "meta/alns/acceptance_criterion.hpp"

auto meta::alns::HillClimbingCriterion::accept(double current, double previous, double /* best */,
                                               RandomEngine & /* gen */) -> bool {
  return current < previous;
}