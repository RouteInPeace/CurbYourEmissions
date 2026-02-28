#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "cye/instance.hpp"
#include "cye/node.hpp"
#include "cye/solution.hpp"

namespace cye {

enum class JumpCase : uint8_t { None, Direct, Depot, Charging };

struct SolutionCandidate {
  // Solution quality
  double distance;
  double battery_used;
  double cargo_used;

  // Backtracking info
  size_t prev;
  uint8_t cs_ind_in;
  uint8_t cs_ind_out;
  JumpCase jump_case;

  SolutionCandidate();
  SolutionCandidate(double distance, double battery_used, double cargo_used);

  auto dominates(SolutionCandidate const &other) const -> bool;
  inline auto is_dominated_by(SolutionCandidate const &other) const { return other.dominates(*this); }
};

class ParetoFront {
 public:
  template <typename... Args>
  auto emplace(Args &&...args) -> void {
    // valid_ = false;
    front_.emplace_back(std::forward<Args>(args)...);
  }

  inline auto clear() -> void {
    valid_ = true;
    front_.clear();
  }
  [[nodiscard]] inline auto size() const { return front_.size(); }

  auto colapse() -> void;

  inline auto begin() const {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_.cbegin();
  }
  inline auto end() const {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_.cend();
  }

  inline auto operator[](size_t i) const -> SolutionCandidate const & {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_[i];
  }

 private:
  bool valid_;
  std::vector<SolutionCandidate> front_;
};

class ParetoFront2D {
 public:
  template <typename... Args>
  auto emplace(Args &&...args) -> void {
    // valid_ = false;
    front_.emplace_back(std::forward<Args>(args)...);
  }

  inline auto clear() -> void {
    valid_ = true;
    front_.clear();
  }
  [[nodiscard]] inline auto size() const { return front_.size(); }

  auto colapse() -> void;

  inline auto begin() const {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_.cbegin();
  }
  inline auto end() const {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_.cend();
  }

  inline auto operator[](size_t i) const -> SolutionCandidate const & {
    if (!valid_) throw std::runtime_error("Call colapse before accesing elements after changing the data.");
    return front_[i];
  }

 private:
  bool valid_;
  std::vector<SolutionCandidate> front_;
};

class CsMatrix {
 public:
  CsMatrix(std::shared_ptr<Instance> instance);

  [[nodiscard]] inline auto distance(size_t ind1, size_t ind2) const { return dist_mat_[ind1][ind2]; };
  [[nodiscard]] inline auto size() const { return cs_cnt_; }
  [[nodiscard]] inline auto ind_to_id(size_t ind) const {
    return ind == 0 ? instance_->depot_id() : instance_->charging_station_ids()[ind - 1];
  }

  auto trace(Patch<size_t> &patch, size_t pos, size_t ind1, size_t ind2) -> void;

 private:
  auto floyd_warshall_() -> void;

  size_t cs_cnt_;
  std::shared_ptr<Instance> instance_;
  std::vector<std::vector<double>> dist_mat_;
  std::vector<std::vector<size_t>> prev_;
};

class OptimalRepair {
 public:
  OptimalRepair(std::shared_ptr<Instance> instance);

  auto patch(Solution &solution, double upper_bound) -> void;

 private:
  auto forward_pass_(Solution const &solution, double upper_bound) -> void;
  auto backward_pass_(Solution &solution) -> void;

  std::shared_ptr<Instance> instance_;
  CsMatrix cs_mat_;
  std::vector<ParetoFront> fronts_;
};

class BatteryDecode {
 public:
  BatteryDecode(std::shared_ptr<Instance> instance);

  auto decode(Solution &solution, double upper_bound) -> void;

 private:
  auto forward_pass_(Solution const &solution, double upper_bound) -> void;
  auto backward_pass_(Solution &solution) -> void;

  std::shared_ptr<Instance> instance_;
  CsMatrix cs_mat_;
  std::vector<ParetoFront2D> fronts_;
};

}  // namespace cye