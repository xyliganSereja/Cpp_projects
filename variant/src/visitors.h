#pragma once

#include "helpers.h"
#include "type_traits.h"

#include <stdexcept>
#include <utility>

namespace impl::visitors {
constexpr auto swap = [](auto&& lhs, auto&& rhs) {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    using std::swap;
    swap(lhs, rhs);
  } else {
    throw std::runtime_error("visitors::swap unexpected call");
  }
};

constexpr auto assign = [](auto&& lhs, auto&& rhs) {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    std::forward<decltype(lhs)>(lhs) = std::forward<decltype(rhs)>(rhs);
  } else {
    throw std::runtime_error("visitors::assign unexpected call");
  }
};

constexpr auto equal = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs == rhs;
  } else {
    throw std::runtime_error("visitors::equal unexpected call");
  }
};

constexpr auto not_equal = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs != rhs;
  } else {
    throw std::runtime_error("visitors::not_equal unexpected call");
  }
};

constexpr auto less = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs < rhs;
  } else {
    throw std::runtime_error("visitors::less unexpected call");
  }
};

constexpr auto greater = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs > rhs;
  } else {
    throw std::runtime_error("visitors::greater unexpected call");
  }
};

constexpr auto less_equal = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs <= rhs;
  } else {
    throw std::runtime_error("visitors::less_equal unexpected call");
  }
};

constexpr auto greater_equal = [](auto&& lhs, auto&& rhs) -> bool {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs >= rhs;
  } else {
    throw std::runtime_error("visitors::greater_equal unexpected call");
  }
};

template <typename R>
constexpr auto starship = [](auto&& lhs, auto&& rhs) -> R {
  if constexpr (type_traits::no_cvref_equal_v<decltype(lhs), decltype(rhs)>) {
    return lhs <=> rhs;
  } else {
    throw std::runtime_error("visitors::starship unexpected call");
  }
};
} // namespace impl::visitors
