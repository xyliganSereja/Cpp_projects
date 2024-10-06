#pragma once

#include "enablers.h"
#include "modules.h"

#include <type_traits>
#include <utility>

struct nullopt_t {
  constexpr explicit nullopt_t(int) {}
};

inline constexpr nullopt_t nullopt(0);

struct in_place_t {
  constexpr explicit in_place_t(int) {}
};

inline constexpr in_place_t in_place(0);

template <typename T>
class optional : impl::modules<T>,
                 impl::enable_copy<std::is_copy_constructible_v<T>, std::is_copy_assignable_v<T>>,
                 impl::enable_move<std::is_move_constructible_v<T>, std::is_move_assignable_v<T>> {
  using base = impl::modules<T>;

public:
  using base::base;
  using base::emplace;
  using base::operator bool;
  using base::operator*;
  using base::operator->;
  using base::reset;
  using base::value;

  constexpr optional() noexcept = default;

  constexpr optional(nullopt_t) noexcept : optional() {}

  constexpr optional(const optional&) noexcept = default;
  constexpr optional(optional&&) noexcept = default;

  constexpr optional& operator=(const optional&) noexcept = default;
  constexpr optional& operator=(optional&&) noexcept = default;

  constexpr optional(T value) : optional() {
    this->emplace(std::move(value));
  }

  template <typename... Args>
  explicit constexpr optional(in_place_t, Args&&... args) : optional() {
    this->emplace(std::forward<Args>(args)...);
  }

  constexpr optional& operator=(nullopt_t) noexcept {
    this->reset();
    return *this;
  }
};

template <typename T>
constexpr void swap(optional<T>& lhs, optional<T>& rhs) {
  if (lhs && rhs) {
    using std::swap;
    swap(*lhs, *rhs);
  } else if (rhs) {
    lhs = std::move(rhs);
    rhs.reset();
  } else if (lhs) {
    swap(rhs, lhs);
  }
}

template <typename T>
constexpr bool operator==(const optional<T>& left, const optional<T>& right) {
  return (!left && !right) || (left && right && *left == *right);
}

template <typename T>
constexpr bool operator!=(const optional<T>& left, const optional<T>& right) {
  return (left || right) && (!left || !right || *left != *right);
}

template <typename T>
constexpr bool operator<(const optional<T>& left, const optional<T>& right) {
  return right && (!left || (left && *left < *right));
}

template <typename T>
constexpr bool operator<=(const optional<T>& left, const optional<T>& right) {
  return !left || (right && *left <= *right);
}

template <typename T>
constexpr bool operator>(const optional<T>& left, const optional<T>& right) {
  return left && (!right || (right && *left > *right));
}

template <typename T>
constexpr bool operator>=(const optional<T>& left, const optional<T>& right) {
  return !right || (left && *left >= *right);
}
