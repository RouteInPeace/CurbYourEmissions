#include "meta/alns/acceptance_criterion.hpp"

auto meta::alns::HillClimbingCriterion::accept(double current, double previous, double /* best */, RandomEngine & /* gen */)
    -> bool {
  return current < previous;
}

auto meta::alns::ThresholdAcceptance::accept(double current, double previous, double /*best*/, RandomEngine & /*gen*/)
    -> bool {
  auto return_value = current <= (1.0 + threshold_) * previous;
  threshold_ -= decay_;
  return return_value;
}

auto meta::alns::RecordToRecordTravel::accept(double current, double /*previous*/, double best, RandomEngine & /*gen*/)
    -> bool {
  auto return_value = current <= (1.0 + threshold_) * best;
  threshold_ -= decay_;
  return return_value;
}

auto meta::alns::SimulatedAnnealing::accept(double current, double previous, double /*best*/, RandomEngine &gen)
    -> bool {
  if (current <= previous) {
    return true;
  }

  double delta = current - previous;
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  auto return_value = dist(gen) < std::exp(-delta / temperature_);
  // TODO: cooling
  return return_value;
}