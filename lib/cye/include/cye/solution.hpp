#pragma once

#include <memory>
#include <vector>
#include "instance.hpp"

namespace cye {

class Solution {
 public:
  struct Station {
    size_t station_id;
    size_t position;
  };

  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> const &routes);
  Solution(std::shared_ptr<Instance> instance, std::vector<size_t> &&routes,
           std::vector<size_t> &&unassigned_customers);

  [[nodiscard]] auto &instance() const { return *instance_; }
  [[nodiscard]] auto &customers() const { return customers_; }
  [[nodiscard]] auto &unassigned_customers() const { return unassigned_customers_; }
  [[nodiscard]] auto visited_node_cnt() const { return customers_.size() + stations_.size() + depots_.size(); }

  [[nodiscard]] auto is_energy_and_cargo_valid() const -> bool;
  [[nodiscard]] auto is_cargo_valid() const -> bool;
  [[nodiscard]] auto is_valid() const -> bool;
  [[nodiscard]] auto get_cost() const -> double;

  auto clear_unassigned_customers() -> void { unassigned_customers_.clear(); }
  auto insert_customer(size_t i, size_t customer_id) -> void;

  auto swap_customer(size_t customer_id1, size_t customer_id2) -> void;

  // TODO: write check that insertions are sorted
  auto insert_charging_station(size_t position, size_t station_id) -> void;
  auto insert_depot(size_t position) -> void;

  auto clear_charging_stations() -> void;
  auto clear_depots() -> void;
  [[nodiscard]] auto pop_depot() -> size_t;

  class CustomerIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const size_t *;
    using reference = const size_t &;

    CustomerIterator(const std::vector<size_t> &customers, size_t customer_pos = 0 ) : customers_(customers), customer_pos_(customer_pos) {}

    reference operator*() const { return customers_[customer_pos_]; }
    pointer operator->() const { return &(customers_[customer_pos_]); }

    CustomerIterator &operator++() {
      ++customer_pos_;
      return *this;
    }

    CustomerIterator operator++(int) {
      CustomerIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const CustomerIterator &a, const CustomerIterator &b) { return a.customer_pos_ == b.customer_pos_; }
    friend bool operator!=(const CustomerIterator &a, const CustomerIterator &b) { return a.customer_pos_ != b.customer_pos_; }

   private:
    const std::vector<size_t> &customers_;
    size_t customer_pos_;
  };

  auto customer_begin() const -> CustomerIterator { return CustomerIterator{customers_, 0}; }
  auto customer_end() const -> CustomerIterator { return CustomerIterator{customers_, customers_.size()}; }

  auto customer_cbegin() const -> CustomerIterator { return CustomerIterator{customers_, 0}; }
  auto customer_cend() const -> CustomerIterator { return CustomerIterator{customers_, customers_.size()}; }

  class CustomerDepotIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const size_t *;
    using reference = const size_t &;

    CustomerDepotIterator(const std::vector<size_t> &customers, const std::vector<size_t> &depots,
                          size_t customer_pos = 0, size_t depot_pos = 0)
        : customers_(customers), depots_(depots), customer_pos_(customer_pos), depot_pos_(depot_pos) {}

    reference operator*() const {
      // depot is always at the end of the route so this will never be out of bounds
      if (depots_[depot_pos_] != customer_pos_) {
        return customers_[customer_pos_];
      }
      return depots_[0];
    }

    pointer operator->() const {
      // depot is always at the end of the route so this will never be out of bounds
      if (depots_[depot_pos_] != customer_pos_) {
        return &customers_[customer_pos_];
      }
      return &depots_[0];
    }

    CustomerDepotIterator &operator++() {
      // depot is always at the end of the route so this will never be out of bounds
      if (depots_[depot_pos_] != customer_pos_) {
        ++customer_pos_;
        return *this;
      }
      ++depot_pos_;
      return *this;
    }

    CustomerDepotIterator operator++(int) {
      CustomerDepotIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    CustomerDepotIterator &operator--() {
      if (depot_pos_ > 0 && depots_[depot_pos_ - 1] == customer_pos_) {
        --depot_pos_;
      } else {
        --customer_pos_;
      }
      return *this;
    }

    CustomerDepotIterator operator--(int) {
      CustomerDepotIterator tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const CustomerDepotIterator &a, const CustomerDepotIterator &b) {
      return a.customer_pos_ == b.customer_pos_ && a.depot_pos_ == b.depot_pos_;
    }
    friend bool operator!=(const CustomerDepotIterator &a, const CustomerDepotIterator &b) { return !(a == b); }

   private:
    const std::vector<size_t> &customers_;
    const std::vector<size_t> &depots_;
    size_t customer_pos_;
    size_t depot_pos_;
  };

  auto customer_depot_begin() const -> CustomerDepotIterator { return CustomerDepotIterator{customers_, depots_}; }
  auto customer_depot_end() const -> CustomerDepotIterator {
    return CustomerDepotIterator{customers_, depots_, customers_.size(), depots_.size()};
  }
  auto customer_depot_cbegin() const -> CustomerDepotIterator { return CustomerDepotIterator{customers_, depots_}; }
  auto customer_depot_cend() const -> CustomerDepotIterator {
    return CustomerDepotIterator{customers_, depots_, customers_.size(), depots_.size()};
  }

  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const size_t *;
    using reference = const size_t &;

    Iterator(const std::vector<size_t> &customers, const std::vector<size_t> &depots,
             const std::vector<Station> &stations, size_t customer_pos = 0, size_t depot_pos = 0,
             size_t station_pos = 0)
        : customers_(customers),
          depots_(depots),
          stations_(stations),
          customer_pos_(customer_pos),
          depot_pos_(depot_pos),
          station_pos_(station_pos) {}

    reference operator*() const {
      if (station_pos_ < stations_.size() && customer_pos_ + depot_pos_ == stations_[station_pos_].position) {
        return stations_[station_pos_].station_id;
      }
      if (depots_[depot_pos_] != customer_pos_) {
        return customers_[customer_pos_];
      }
      return depots_[0];
    }

    pointer operator->() const {
      if (station_pos_ < stations_.size() && customer_pos_ + depot_pos_ == stations_[station_pos_].position) {
        return &stations_[station_pos_].station_id;
      }
      if (depots_[depot_pos_] != customer_pos_) {
        return &customers_[customer_pos_];
      }
      return &depots_[0];
    }

    Iterator &operator++() {
      if (station_pos_ < stations_.size() && customer_pos_ + depot_pos_ == stations_[station_pos_].position) {
        ++station_pos_;
      } else if (depots_[depot_pos_] != customer_pos_) {
        ++customer_pos_;
      } else {
        ++depot_pos_;
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) {
      return a.customer_pos_ == b.customer_pos_ && a.depot_pos_ == b.depot_pos_ && a.station_pos_ == b.station_pos_;
    }
    friend bool operator!=(const Iterator &a, const Iterator &b) { return !(a == b); }

   private:
    const std::vector<size_t> &customers_;
    const std::vector<size_t> &depots_;
    const std::vector<Station> &stations_;
    size_t customer_pos_;
    size_t depot_pos_;
    size_t station_pos_;
  };

  auto begin() const -> Iterator { return Iterator{customers_, depots_, stations_}; }
  auto end() const -> Iterator {
    return Iterator{customers_, depots_, stations_, customers_.size(), depots_.size(), stations_.size()};
  }
  auto cbegin() const -> Iterator { return Iterator{customers_, depots_, stations_}; }
  auto cend() const -> Iterator {
    return Iterator{customers_, depots_, stations_, customers_.size(), depots_.size(), stations_.size()};
  }

 private:
  std::shared_ptr<Instance> instance_;

  // position priority: customer -> depot -> charging station
  std::vector<size_t> customers_;
  std::vector<size_t> depots_;
  std::vector<Station> stations_;
  std::vector<size_t> unassigned_customers_;

  bool stations_valid_{false};
  bool depots_valid_{false};
};

}  // namespace cye