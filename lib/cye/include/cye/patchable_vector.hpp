#pragma once

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace cye {

template <typename T>
class PatchableVector;

template <typename T>
class Patch {
 public:
 inline auto add_change(size_t ind, T &&value) {
  assert(changes_.size() == 0 || changes_.back().ind <= ind);
  changes_.emplace_back(ind, std::move(value));
}

  inline auto add_change(size_t ind, T const &value) {
    assert(changes_.size() == 0 || changes_.back().ind <= ind);
    changes_.emplace_back(ind, value);
  }

  friend PatchableVector<T>;

 private:
  struct Change_ {
    size_t ind;
    T value;
  };

  std::vector<Change_> changes_;
};

template <typename T>
class PatchableVector {
 public:
  PatchableVector() = default;
  PatchableVector(std::initializer_list<T> init) : base_(init) {}
  PatchableVector(std::vector<T> &&base) : base_(std::move(base)) {}

  inline auto add_patch(Patch<T> &&patch) { patches_.push_back(std::move(patch)); }
  inline auto clear_patches() { patches_.clear(); }
  inline auto pop_patch() { patches_.pop_back(); }

  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type *;
    using reference = value_type &;

    Iterator(size_t base_ind, std::vector<size_t> &&indices, std::vector<size_t> &&change_indices, std::vector<T> &base,
             std::vector<Patch<T>> &patches)
        : current_value_(nullptr),
          base_ind_(base_ind),
          indices_(std::move(indices)),
          change_indices_(std::move(change_indices)),
          base_(base),
          patches_(patches) {
      if (patches.size() > 0) {
        for (auto i = patches_.size(); i-- > 0;) {
          if (change_indices_[i] > 0 && last_applied_change_(i).ind == indices_[i]) {
            current_value_ = &last_applied_change_(i).value;
            return;
          }
        }
      }

      if (base_ind_ < base_.size()) {
        current_value_ = &base[base_ind_];
      }
    }

    auto operator*() const -> reference { return *current_value_; }

    auto operator->() const -> pointer { return current_value_; }

    auto operator++() -> Iterator & {
      if (patches_.size() > 0) {
        for (auto i = patches_.size(); i-- > 0;) {
          indices_[i]++;
          if (!reached_end_of_patch_(i) && change_(i).ind == indices_[i]) {
            current_value_ = &change_(i).value;
            change_indices_[i]++;
            return *this;
          }
        }
      }

      base_ind_++;
      if (base_ind_ < base_.size()) {
        current_value_ = &base_[base_ind_];
      }

      return *this;
    }

    friend auto operator==(const Iterator &a, const Iterator &b) -> bool {
      return a.base_ind_ == b.base_ind_ && a.indices_ == b.indices_ && &a.base_ == &b.base_ &&
             &a.patches_ == &b.patches_;
    };
    friend auto operator!=(const Iterator &a, const Iterator &b) -> bool { return !(a == b); };

   private:
    auto patch_size_(size_t i) const { return patches_[i].changes_.size(); }
    auto reached_end_of_patch_(size_t i) const { return change_indices_[i] >= patch_size_(i); }
    auto &change_(size_t i) const { return patches_[i].changes_[change_indices_[i]]; }
    auto &last_applied_change_(size_t i) const { return patches_[i].changes_[change_indices_[i] - 1UZ]; }

    T *current_value_;

    size_t base_ind_;
    std::vector<size_t> indices_;
    std::vector<size_t> change_indices_;

    std::vector<T> &base_;
    std::vector<Patch<T>> &patches_;
  };

  auto begin() -> Iterator {
    return Iterator(0UZ, std::vector<size_t>(patches_.size(), 0UZ), std::vector<size_t>(patches_.size(), 0UZ), base_,
                    patches_);
  }
  auto end() -> Iterator {
    auto indices = std::vector<size_t>(patches_.size(), 0UZ);
    auto change_indices_ = std::vector<size_t>(patches_.size(), 0UZ);

    for (auto i = 0UZ; i < patches_.size(); ++i) {
      indices[i] = i == 0 ? base_.size() : indices[i - 1];
      indices[i] += patches_[i].changes_.size();
      change_indices_[i] = patches_[i].changes_.size();
    }

    return Iterator(base_.size(), std::move(indices), std::move(change_indices_), base_, patches_);
  }

 private:
  std::vector<T> base_;
  std::vector<Patch<T>> patches_;
};

}  // namespace cye