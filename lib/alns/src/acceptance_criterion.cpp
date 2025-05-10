#include "alns/acceptance_criterion.hpp"

auto alns::HillClimbingCriterion::accept(double current, double previous, double /* best */) -> bool {
  return current < previous;
}