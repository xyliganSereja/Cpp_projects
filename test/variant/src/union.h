#pragma once

#include "helpers.h"

#include <stdexcept>

namespace impl::union_ {
struct do_nothing {
  constexpr do_nothing() = delete;

  constexpr explicit do_nothing(int) {}
};

inline constexpr do_nothing do_nothing_v(0);

template <typename... Ts>
union union_impl;

template <typename... Ts>
struct skip_non_unique {
  using type = union_impl<>;
};

template <typename T, typename... Ts>
  requires(find_v<T, Ts...> == variant_npos)
struct skip_non_unique<T, Ts...> {
  using type = union_impl<T, Ts...>;
};

template <typename T, typename... Ts>
struct skip_non_unique<T, Ts...> {
  using type = skip_non_unique<Ts...>::type;
};

template <typename... Ts>
using union_t = skip_non_unique<Ts...>::type;

template <>
union union_impl<> {
public:
  constexpr explicit union_impl(do_nothing) noexcept {}

  template <typename T, typename... Args>
  T& construct(Args&&... /*args*/) {
    throw std::runtime_error("union_impl::construct unexpected call");
  }

  template <typename T>
  T& get() {
    throw std::runtime_error("union_impl::get unexpected call");
  }

  template <typename T>
  const T& get() const {
    throw std::runtime_error("union_impl::get unexpected call");
  }
};

template <typename T0, typename... Ts>
union union_impl<T0, Ts...> {
private:
  T0 t;
  union_t<Ts...> other;

public:
  constexpr ~union_impl()
    requires(std::is_trivially_destructible_v<T0> && std::is_trivially_destructible_v<decltype(other)>)
  = default;

  constexpr ~union_impl() {}

  constexpr union_impl() noexcept(std::is_nothrow_default_constructible_v<T0>)
    requires(std::is_default_constructible_v<T0>)
      : t() {}

  constexpr explicit union_impl(do_nothing) noexcept {}

  template <typename T, typename... Args>
  constexpr T& construct(Args&&... args) {
    if constexpr (std::is_same_v<T, T0>) {
      std::construct_at(std::addressof(t), std::forward<Args>(args)...);
      return t;
    } else {
      std::construct_at(std::addressof(other), do_nothing_v);
      return other.template construct<T>(std::forward<Args>(args)...);
    }
  }

  template <typename T>
  constexpr T& get() {
    if constexpr (std::is_same_v<T, T0>) {
      return t;
    } else {
      return other.template get<T>();
    }
  }

  template <typename T>
  constexpr const T& get() const {
    if constexpr (std::is_same_v<T, T0>) {
      return t;
    } else {
      return other.template get<T>();
    }
  }
};
} // namespace impl::union_
