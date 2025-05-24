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
  struct Change {
    size_t ind;
    T value;
  };

  inline auto add_change(size_t ind, T &&value) { changes_.emplace_back(ind, std::move(value)); }

  inline auto add_change(size_t ind, T const &value) { changes_.emplace_back(ind, value); }

  inline auto reverse() { std::ranges::reverse(changes_); }

  inline auto sort() { std::ranges::stable_sort(changes_); }

  [[nodiscard]] inline auto size() const { return changes_.size(); }

  [[nodiscard]] inline auto back() const { return changes_.back(); }
  [[nodiscard]] inline auto empty() const { return changes_.empty(); }

  friend PatchableVector<T>;

 private:
  std::vector<Change> changes_;
};

template <typename T>
class PatchableVector {
 public:
  PatchableVector() = default;
  PatchableVector(std::initializer_list<T> init) : base_(init) {}
  PatchableVector(std::vector<T> &&base) : base_(std::move(base)) {}

  inline auto add_patch(Patch<T> &&patch) {
#ifndef NDEBUG
    auto s = size();
    for (auto i = 1UZ; i < patch.changes_.size(); ++i) {
      assert(patch.changes_[i - 1].ind <= patch.changes_[i].ind);
      assert(patch.changes_[i].ind <= s);
    }
#endif

    patches_.push_back(std::move(patch));
  }

  inline auto clear_patches() { patches_.clear(); }
  inline auto pop_patch() { patches_.pop_back(); }
  inline auto base() { return std::span(base_); }
  inline auto base() const { return std::span(base_); }
  [[nodiscard]] auto size() const {
    auto size = base_.size();
    for (const auto &patch : patches_) {
      size += patch.size();
    }

    return size;
  }

  class Sentinel {};

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

    struct PatchInfo {
      size_t index;         // The curent index on each level.
      size_t change_index;  // Index of the next change to apply in a patch
      bool started;  // Did we start iteration on that level? This is needed when the patch inserts at position 0.
    };

    Iterator();

    Iterator(size_t base_ind, bool started_base, std::vector<PatchInfo> &&patch_info, BaseVecPtr base,
             PatchVecPtr patches);

    auto operator*() const -> reference { return *current_value_; }
    auto operator->() const -> pointer { return current_value_; }

    auto operator++() -> Iterator<V> &;
    auto operator++(int) -> Iterator<V>;

    auto operator--() -> Iterator<V> &;
    auto operator--(int) -> Iterator<V>;

    friend auto operator==(const Iterator &a, const Iterator &b) -> bool {
      return a.current_value_ == b.current_value_;
    };
    friend auto operator!=(const Iterator &a, const Iterator &b) -> bool {
      return a.current_value_ != b.current_value_;
    };
    friend auto operator==(const Iterator &a, Sentinel) { return a.current_value_ == nullptr; }
    friend auto operator==(Sentinel, const Iterator &a) { return a.current_value_ == nullptr; }

   private:
    auto patch_size_(size_t i) const { return (*patches_)[i].changes_.size(); }
    auto reached_end_of_patch_(size_t i) const { return patch_info_[i].change_index >= patch_size_(i); }
    auto &change_(size_t i) const { return (*patches_)[i].changes_[patch_info_[i].change_index]; }
    auto &prev_change_(size_t i) const { return (*patches_)[i].changes_[patch_info_[i].change_index - 1UZ]; }
    auto predecessor_ind_(size_t i) { return i == 0 ? base_ind_ : patch_info_[i - 1UZ].index; }
    auto predecessor_started_(size_t i) -> bool { return i == 0 ? started_base_ : patch_info_[i - 1UZ].started; }
    auto find_prev_value_() -> pointer;

    V *current_value_;

    size_t base_ind_;
    bool started_base_;

    std::vector<PatchInfo> patch_info_;

    BaseVecPtr base_;
    PatchVecPtr patches_;
  };

  static_assert(std::bidirectional_iterator<Iterator<T>>);
  static_assert(std::bidirectional_iterator<Iterator<const T>>);
  static_assert(std::sentinel_for<Sentinel, Iterator<T>>);
  static_assert(std::sentinel_for<Sentinel, Iterator<const T>>);

  [[nodiscard]] auto begin() -> Iterator<T> {
    return Iterator<T>(
        0UZ, false,
        std::vector<typename Iterator<T>::PatchInfo>(patches_.size(), typename Iterator<T>::PatchInfo{0, 0, false}),
        &base_, &patches_);
  }

  [[nodiscard]] auto begin() const -> Iterator<const T> {
    return Iterator<const T>(0UZ, false,
                             std::vector<typename Iterator<const T>::PatchInfo>(
                                 patches_.size(), typename Iterator<const T>::PatchInfo{0, 0, false}),
                             &base_, &patches_);
  }

  [[nodiscard]] auto end() -> Sentinel { return {}; }
  [[nodiscard]] auto end() const -> Sentinel { return {}; }

  [[nodiscard]] auto rbegin() -> Iterator<T> {
    auto patch_info = std::vector<typename Iterator<const T>::PatchInfo>(patches_.size());

    for (auto i = 0UZ; i < patches_.size(); ++i) {
      patch_info[i].index = i == 0 ? base_.size() : patch_info[i - 1].index;
      patch_info[i].index+= patches_[i].changes_.size();
      patch_info[i].change_index = patches_[i].changes_.size();
      patch_info[i].started = true;
    }

    return --Iterator<const T>(base_.size(), true, std::move(patch_info), &base_, &patches_);
  }
  [[nodiscard]] auto rbegin() const -> Iterator<const T> {
    auto patch_info = std::vector<typename Iterator<const T>::PatchInfo>(patches_.size());

    for (auto i = 0UZ; i < patches_.size(); ++i) {
      patch_info[i].index = i == 0 ? base_.size() : patch_info[i - 1].index;
      patch_info[i].index+= patches_[i].changes_.size();
      patch_info[i].change_index = patches_[i].changes_.size();
      patch_info[i].started = true;
    }

    return --Iterator<const T>(base_.size(), true, std::move(patch_info), &base_, &patches_);
  }

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

// ------------------------------------------------------------------------------------------------------
// ----------------------------------------------- Detail -----------------------------------------------
// ------------------------------------------------------------------------------------------------------

template <typename T>
template <typename V>
PatchableVector<T>::Iterator<V>::Iterator()
    : current_value_(nullptr), base_ind_(0UZ), started_base_(false), base_(nullptr), patches_(nullptr) {}

template <typename T>
template <typename V>
PatchableVector<T>::Iterator<V>::Iterator(size_t base_ind, bool started_base, std::vector<PatchInfo> &&patch_info,
                                          BaseVecPtr base, PatchVecPtr patches)
    : current_value_(nullptr),
      base_ind_(base_ind),
      started_base_(started_base),
      patch_info_(patch_info),
      base_(base),
      patches_(patches) {
  if (patches->size() > 0) {
    for (auto i = patches_->size(); i-- > 0;) {
      patch_info_[i].started = true;
      if (patch_info_[i].change_index > 0 && prev_change_(i).ind == patch_info_[i].index) {
        current_value_ = &prev_change_(i).value;
        return;
      }
      if (!reached_end_of_patch_(i) && change_(i).ind == patch_info_[i].index) {
        current_value_ = &change_(i).value;
        patch_info_[i].change_index++;
        return;
      }
    }
  }

  started_base_ = true;
  if (base_ind_ < base_->size()) {
    current_value_ = &(*base)[base_ind_];
  } else {
    current_value_ = nullptr;
  }
}

template <typename T>
template <typename V>
auto PatchableVector<T>::Iterator<V>::operator++() -> Iterator<V> & {
  if (patches_->size() > 0) {
    for (auto i = patches_->size(); i-- > 0;) {
      if (!patch_info_[i].started) {
        patch_info_[i].started = true;
      } else {
        patch_info_[i].index++;
      }
      if (!reached_end_of_patch_(i) &&
          (change_(i).ind == 0 || (predecessor_started_(i) && change_(i).ind - 1UZ == predecessor_ind_(i)))) {
        current_value_ = &change_(i).value;
        patch_info_[i].change_index++;
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
  } else {
    current_value_ = nullptr;
  }

  return *this;
}

template <typename T>
template <typename V>
auto PatchableVector<T>::Iterator<V>::operator--() -> Iterator<V> & {
  if (patches_->size() > 0) {
    for (auto i = patches_->size(); i-- > 0;) {
      if (patch_info_[i].index == 0) {
        started_base_ = false;
        for (auto j = i + 1UZ; j-- > 0;) {
          patch_info_[j].started = false;
          if (patch_info_[j].change_index > 0) {
            patch_info_[j].change_index--;
            current_value_ = find_prev_value_();
            return *this;
          }
        }
        break;
      }

      patch_info_[i].index--;

      if (patch_info_[i].change_index > 0 && &prev_change_(i).value == current_value_) {
        if (prev_change_(i).ind == 0) {
          started_base_ = false;
          for (auto j = 0UZ; j < i; ++j) patch_info_[j].started = false;
        }
        patch_info_[i].change_index--;
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

template <typename T>
template <typename V>
auto PatchableVector<T>::Iterator<V>::operator++(int) -> Iterator<V> {
  auto tmp = *this;
  ++(*this);
  return tmp;
}

template <typename T>
template <typename V>
auto PatchableVector<T>::Iterator<V>::operator--(int) -> Iterator<V> {
  auto tmp = *this;
  --(*this);
  return tmp;
}

template <typename T>
template <typename V>
auto PatchableVector<T>::Iterator<V>::find_prev_value_() -> pointer {
  for (auto i = patches_->size(); i-- > 0;) {
    if (!predecessor_started_(i) ||
        (patch_info_[i].change_index > 0 && predecessor_ind_(i) + 1UZ == prev_change_(i).ind)) {
      return &prev_change_(i).value;
    }
  }

  return &(*base_)[base_ind_];
}

}  // namespace cye