#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

namespace cye {

template <typename T>
class PatchableVector;

template <typename T>
class Patch {
 public:
  inline auto add_change(size_t ind, T &&value) { changes_.emplace_back(ind, std::move(value)); }

  inline auto add_change(size_t ind, T const &value) { changes_.emplace_back(ind, value); }

  inline auto reverse() { std::ranges::reverse(changes_); }

  inline auto sort() { std::ranges::stable_sort(changes_); }

  [[nodiscard]] inline auto size() const { return changes_.size(); }

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

  inline auto add_patch(Patch<T> &&patch) {
#ifndef NDEBUG
    for (auto i = 1UZ; i < patch.changes_.size(); ++i) {
      assert(patch.changes_[i - 1].ind <= patch.changes_[i].ind);
    }
#endif

    patches_.push_back(std::move(patch));
  }

  inline auto clear_patches() { patches_.clear(); }
  inline auto pop_patch() { patches_.pop_back(); }
  inline auto base() { return std::span(base_); }
  [[nodiscard]] inline auto size() const {
    auto size = base_.size();
    for (const auto &patch : patches_) {
      size += patch.size();
    }

    return size;
  }

  template <typename V>
  class Iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = V;
    using pointer = value_type *;
    using reference = value_type &;

    using BaseVecPtr = typename std::conditional<std::is_const_v<V>, const std::vector<std::remove_const_t<V>> *,
                                                 std::vector<std::remove_const_t<V>> *>::type;

    using PatchVecPtr =
        typename std::conditional<std::is_const_v<V>, const std::vector<Patch<std::remove_const_t<V>>> *,
                                  std::vector<Patch<std::remove_const_t<V>>> *>::type;

    Iterator() : base_ind_(0UZ), started_base_(false), base_(nullptr), patches_(nullptr) {}

    Iterator(size_t base_ind, bool started_base, std::vector<size_t> &&indices, std::vector<bool> &&started,
             std::vector<size_t> &&change_indices, BaseVecPtr base, PatchVecPtr patches)
        : current_value_(nullptr),
          base_ind_(base_ind),
          started_base_(started_base),
          indices_(std::move(indices)),
          started_(std::move(started)),
          change_indices_(std::move(change_indices)),
          base_(base),
          patches_(patches) {
      if (patches->size() > 0) {
        for (auto i = patches_->size(); i-- > 0;) {
          started_[i] = true;
          if (change_indices_[i] > 0 && prev_change_(i).ind == indices_[i]) {
            current_value_ = &prev_change_(i).value;
            return;
          }
          if (!reached_end_of_patch_(i) && change_(i).ind == indices_[i]) {
            current_value_ = &change_(i).value;
            change_indices_[i]++;
            return;
          }
        }
      }

      started_base_ = true;
      if (base_ind_ < base_->size()) {
        current_value_ = &(*base)[base_ind_];
      }
    }

    auto operator*() const -> reference { return *current_value_; }

    auto operator->() const -> pointer { return current_value_; }

    auto operator++() -> Iterator & {
      if (patches_->size() > 0) {
        for (auto i = patches_->size(); i-- > 0;) {
          if (!started_[i]) {
            started_[i] = true;
          } else {
            indices_[i]++;
          }
          if (!reached_end_of_patch_(i) &&
              (change_(i).ind == 0 || (predecessor_started_(i) && change_(i).ind - 1UZ == predecessor_ind_(i)))) {
            current_value_ = &change_(i).value;
            change_indices_[i]++;
            return *this;
          }
        }
      }

      if (!started_base_) {
        started_base_ = true;
      } else {
        base_ind_++;
      }
      if (base_ind_ < base_->size()) {
        current_value_ = &(*base_)[base_ind_];
      }

      return *this;
    }

    auto operator++(int) -> Iterator {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    auto operator--() -> Iterator & {
      if (patches_->size() > 0) {
        for (auto i = patches_->size(); i-- > 0;) {
          if (indices_[i] == 0) {
            started_base_ = false;
            for (auto j = i + 1UZ; j-- > 0;) {
              started_[j] = false;
              if (change_indices_[j] > 0) {
                change_indices_[j]--;
                current_value_ = find_prev_value_();
                return *this;
              }
            }
            break;
          }

          indices_[i]--;

          if (change_indices_[i] > 0 && &prev_change_(i).value == current_value_) {
            if (prev_change_(i).ind == 0) {
              started_base_ = false;
              for (auto j = 0UZ; j < i; ++j) started_[j] = false;
            }
            change_indices_[i]--;
            current_value_ = find_prev_value_();
            return *this;
          }
        }
      }

      if (base_ind_ == 0) {
        started_base_ = false;
      } else {
        base_ind_--;
      }

      current_value_ = find_prev_value_();

      return *this;
    }

    auto operator--(int) -> Iterator {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    friend auto operator==(const Iterator &a, const Iterator &b) -> bool {
      return a.base_ind_ == b.base_ind_ && a.started_base_ == b.started_base_ && a.indices_ == b.indices_ &&
             a.started_ == b.started_ && a.change_indices_ == b.change_indices_ && a.base_ == b.base_ &&
             a.patches_ == b.patches_;
    };
    friend auto operator!=(const Iterator &a, const Iterator &b) -> bool { return !(a == b); };

   private:
    auto patch_size_(size_t i) const { return (*patches_)[i].changes_.size(); }
    auto reached_end_of_patch_(size_t i) const { return change_indices_[i] >= patch_size_(i); }
    auto &change_(size_t i) const { return (*patches_)[i].changes_[change_indices_[i]]; }
    auto &prev_change_(size_t i) const { return (*patches_)[i].changes_[change_indices_[i] - 1UZ]; }
    auto predecessor_ind_(size_t i) {
      if (i == 0) return base_ind_;
      return indices_[i - 1UZ];
    }
    auto predecessor_started_(size_t i) -> bool {
      if (i == 0) return started_base_;
      return started_[i - 1UZ];
    }
    auto find_prev_value_() -> pointer {
      for (auto i = patches_->size(); i-- > 0;) {
        if (!predecessor_started_(i) || (change_indices_[i] > 0 && predecessor_ind_(i) + 1UZ == prev_change_(i).ind)) {
          return &prev_change_(i).value;
        }
      }

      return &(*base_)[base_ind_];
    }

    V *current_value_;

    size_t base_ind_;
    bool started_base_;

    std::vector<size_t> indices_;  // The curent index on each level.
    std::vector<bool>
        started_;  // Did we start iteration on that level? This is needed when the patch inserts at position 0.
    std::vector<size_t> change_indices_;  // Index of the next change to apply in a patch

    BaseVecPtr base_;
    PatchVecPtr patches_;
  };

  static_assert(std::bidirectional_iterator<Iterator<T>>);

  [[nodiscard]] auto begin() -> Iterator<T> {
    return Iterator<T>(0UZ, false, std::vector<size_t>(patches_.size(), 0UZ), std::vector<bool>(patches_.size(), false),
                       std::vector<size_t>(patches_.size(), 0UZ), &base_, &patches_);
  }

  [[nodiscard]] auto end() -> Iterator<T> {
    auto indices = std::vector<size_t>(patches_.size(), 0UZ);
    auto change_indices_ = std::vector<size_t>(patches_.size(), 0UZ);

    for (auto i = 0UZ; i < patches_.size(); ++i) {
      indices[i] = i == 0 ? base_.size() : indices[i - 1];
      indices[i] += patches_[i].changes_.size();
      change_indices_[i] = patches_[i].changes_.size();
    }

    return Iterator<T>(base_.size(), true, std::move(indices), std::vector<bool>(patches_.size(), true),
                       std::move(change_indices_), &base_, &patches_);
  }

  [[nodiscard]] auto begin() const -> Iterator<const T> {
    return Iterator<const T>(0UZ, false, std::vector<size_t>(patches_.size(), 0UZ),
                             std::vector<bool>(patches_.size(), false), std::vector<size_t>(patches_.size(), 0UZ),
                             &base_, &patches_);
  }

  [[nodiscard]] auto end() const -> Iterator<const T> {
    auto indices = std::vector<size_t>(patches_.size(), 0UZ);
    auto change_indices_ = std::vector<size_t>(patches_.size(), 0UZ);

    for (auto i = 0UZ; i < patches_.size(); ++i) {
      indices[i] = i == 0 ? base_.size() : indices[i - 1];
      indices[i] += patches_[i].changes_.size();
      change_indices_[i] = patches_[i].changes_.size();
    }

    return Iterator<const T>(base_.size(), true, std::move(indices), std::vector<bool>(patches_.size(), true),
                             std::move(change_indices_), &base_, &patches_);
  }

  [[nodiscard]] auto rbegin() -> Iterator<T> { return --end(); }
  [[nodiscard]] auto rbegin() const -> Iterator<const T> { return --end(); }

  auto squash() {
    auto new_base = std::vector<T>();
    new_base.reserve(base_.size());

    auto end_it = end();
    for (auto it = begin(); it != end_it; ++it) {
      new_base.push_back(std::move(*it));
    }

    base_ = std::move(new_base);
    patches_.clear();
  }

 private:
  std::vector<T> base_;
  std::vector<Patch<T>> patches_;
};

}  // namespace cye