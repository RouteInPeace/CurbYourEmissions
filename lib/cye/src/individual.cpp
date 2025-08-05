#include "cye/individual.hpp"
#include "cye/repair.hpp"

cye::EVRPIndividual::EVRPIndividual(std::shared_ptr<cye::OptimalEnergyRepair> energy_repair, cye::Solution &&solution)
    : energy_repair_(energy_repair), solution_(std::move(solution)), valid_(false) {
  update_cost();
}

auto cye::EVRPIndividual::update_cost() -> void {
  if (!valid_) {
    solution_.clear_patches();
    if (trivial_) {
      assert(false);
      cye::patch_cargo_trivially(solution_);
      //cye::patch_energy_trivially(solution_);
      cye::patch_energy_removal_heuristic(solution_);
    } else {
      cye::patch_cargo_optimally(solution_, static_cast<unsigned>(solution_.instance().cargo_capacity()) + 1u);
      //cye::patch_energy_optimal_heuristic(solution_);
      energy_repair_->patch(solution_, 151U);
    }
    valid_ = true;
  }

  cost_ = 0.f;
  auto previous_node_id = *solution_.routes().begin();
  hash_ = 14695981039346656037u;
  for (auto it = ++solution_.routes().begin(); it != solution_.routes().end(); ++it) {
    auto current_node_id = *it;
    cost_ += solution_.instance().distance(previous_node_id, current_node_id);
    previous_node_id = current_node_id;

    hash_ *= fnv_prime_;
    hash_ ^= std::hash<size_t>{}(current_node_id);
  }
}