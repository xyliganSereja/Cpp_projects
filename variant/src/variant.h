#pragma once

#include "helpers.h"
#include "type_traits.h"
#include "union.h"
#include "visit_impl.h"
#include "visitors.h"

#include <stdexcept>

template <typename... Ts>
struct variant;

/// variant_size

template <typename T>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename... Types>
struct variant_size<const variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename T>
inline constexpr std::size_t variant_size_v = variant_size<T>::value;

/// variant_alternative

template <std::size_t N, typename T>
struct variant_alternative;

template <std::size_t N, typename... Ts>
struct variant_alternative<N, variant<Ts...>> : impl::nth<N, Ts...> {};

template <std::size_t N, typename... Ts>
struct variant_alternative<N, const variant<Ts...>> : impl::nth<N, const Ts...> {};

template <std::size_t N, typename T>
using variant_alternative_t = typename variant_alternative<N, T>::type;

/// monostate

struct monostate {};

constexpr bool operator==(monostate, monostate) noexcept {
  return true;
}

constexpr bool operator!=(monostate, monostate) noexcept {
  return false;
}

constexpr bool operator<(monostate, monostate) noexcept {
  return false;
}

constexpr bool operator>(monostate, monostate) noexcept {
  return false;
}

constexpr bool operator<=(monostate, monostate) noexcept {
  return true;
}

constexpr bool operator>=(monostate, monostate) noexcept {
  return true;
}

/// in_place_type & in_place_index

template <typename T>
struct in_place_type_t {
  constexpr in_place_type_t() = delete;

  constexpr explicit in_place_type_t(int) {}
};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type(0);

template <std::size_t N>
struct in_place_index_t {
  constexpr in_place_index_t() = delete;

  constexpr explicit in_place_index_t(int) {}
};

template <std::size_t N>
inline constexpr in_place_index_t<N> in_place_index(0);

/// bad_variant_access

struct bad_variant_access : std::exception {
  const char* what() const noexcept override {
    return "bad_variant_access";
  }
};

/// visit

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  using Bounds = impl::visit_impl::multiindex<variant_size_v<std::remove_reference_t<Variants>>...>;
  return impl::visit_impl::func_table_v<R, Visitor, Bounds, Variants...>.get_overload(vars.index()...)(
      std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars) {
  using R = decltype(std::forward<Visitor>(vis)(get<0>(std::forward<Variants>(vars))...));
  return ::visit<R, Visitor, Variants...>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

/// variant

template <typename... Ts>
struct variant {
  static_assert(sizeof...(Ts), "variant must have minimum one alternative");
  static_assert((!std::is_reference_v<Ts> && ...), "variant must not have any reference alternative");
  static_assert((!std::is_same_v<Ts, void> && ...), "variant must not have any void alternatives");

private:
  std::size_t index_ = 0;
  impl::union_::union_t<Ts...> data;

public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<impl::nth_t<0, Ts...>>) = default;

  constexpr ~variant()
    requires(impl::type_traits::trivially_destructible_v<Ts...>)
  = default;

  constexpr ~variant() {
    destroy();
  }

  constexpr variant(const variant& other) noexcept
    requires(impl::type_traits::trivially_copy_constructible_v<Ts...>)
  = default;

  constexpr variant(const variant& other) noexcept((std::is_nothrow_copy_constructible_v<Ts> && ...))
    requires(impl::type_traits::copy_constructible_v<Ts...>)
      : index_(other.index_),
        data(impl::union_::do_nothing_v) {
    if (!other.valueless_by_exception()) {
      ::visit(
          [this, &other](auto& v) {
            using V = std::remove_cvref_t<decltype(v)>;
            if (holds_alternative<V>(other)) {
              data.template construct<V>(v);
            } else {
              data.template construct<const V>(v);
            }
          },
          other);
    }
  }

  constexpr variant& operator=(const variant& other) noexcept
    requires(impl::type_traits::trivially_copy_assignable_v<Ts...>)
  = default;

  constexpr variant& operator=(const variant& other) noexcept(
      ((std::is_nothrow_copy_assignable_v<Ts> && std::is_nothrow_copy_constructible_v<Ts>)&&...))
    requires(impl::type_traits::copy_assignable_v<Ts...>)
  {
    if (this == &other) {
      return *this;
    }
    if (index() == other.index()) {
      ::visit(impl::visitors::assign, *this, other);
    } else {
      bool flag = ::visit(
          [&other](auto& v) -> bool {
            using V = std::remove_cvref_t<decltype(v)>;
            return holds_alternative<V>(other)
                     ? std::is_nothrow_copy_constructible_v<V> || !std::is_nothrow_move_constructible_v<V>
                     : std::is_nothrow_copy_constructible_v<const V> || !std::is_nothrow_move_constructible_v<const V>;
          },
          other);
      if (flag) {
        ::visit(
            [&other, this](auto& v) {
              using V = std::remove_cvref_t<decltype(v)>;
              destroy();
              if (holds_alternative<V>(other)) {
                data.template construct<V>(v);
              } else {
                data.template construct<const V>(v);
              }
            },
            other);
      } else {
        *this = variant(other);
      }
      index_ = other.index_;
    }
    return *this;
  }

  constexpr variant(variant&& other) noexcept
    requires(impl::type_traits::trivially_move_constructible_v<Ts...>)
  = default;

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...))
    requires(impl::type_traits::move_constructible_v<Ts...>)
      : index_(other.index_),
        data(impl::union_::do_nothing_v) {
    if (!other.valueless_by_exception()) {
      ::visit([this](auto&& v) { data.template construct<std::remove_reference_t<decltype(v)>>(std::move(v)); },
              std::move(other));
    }
  }

  constexpr variant& operator=(variant&& other) noexcept
    requires(impl::type_traits::trivially_move_assignable_v<Ts...>)
  = default;

  constexpr variant& operator=(variant&& other) noexcept(
      ((std::is_nothrow_move_assignable_v<Ts> && std::is_nothrow_move_constructible_v<Ts>)&&...))
    requires(impl::type_traits::move_assignable_v<Ts...>)
  {
    if (this == &other) {
      return *this;
    }
    if (index() == other.index()) {
      ::visit(impl::visitors::assign, *this, std::move(other));
    } else {
      destroy();
      if (!other.valueless_by_exception()) {
        ::visit([this](auto&& v) { data.template construct<std::remove_cvref_t<decltype(v)>>(std::move(v)); },
                std::move(other));
        index_ = other.index();
      }
    }
    return *this;
  }

  template <typename T, typename... Args>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(impl::count_v<T, Ts...> == 1 && std::is_constructible_v<T, Args...>)
      : variant(in_place_index<impl::find_v<T, Ts...>>, std::forward<Args>(args)...) {}

  template <std::size_t N, typename... Args>
  constexpr explicit variant(in_place_index_t<N>,
                             Args&&... args) noexcept(std::is_nothrow_constructible_v<impl::nth_t<N, Ts...>, Args...>)
    requires(N < sizeof...(Ts) && std::is_constructible_v<impl::nth_t<N, Ts...>, Args...>)
      : index_(N),
        data(impl::union_::do_nothing_v) {
    data.template construct<impl::nth_t<N, Ts...>>(std::forward<Args>(args)...);
  }

  template <typename F, typename T = impl::type_traits::overloaded_t<F&&, Ts...>>
  constexpr variant(F&& value) noexcept(std::is_nothrow_constructible_v<T, F&&>)
      : variant(in_place_type<T>, std::forward<F>(value)) {}

  template <typename F, typename T = impl::type_traits::overloaded_t<F&&, Ts...>>
  constexpr variant& operator=(F&& value) noexcept(std::is_nothrow_constructible_v<T, F&&> &&
                                                   std::is_nothrow_assignable_v<T, F&&>) {
    constexpr std::size_t N = impl::find_v<T, Ts...>;
    if (index() == N) {
      ::visit(
          [&value](auto& v) {
            if constexpr (impl::type_traits::no_cvref_equal_v<decltype(v), T>) {
              v = std::forward<F>(value);
            }
          },
          *this);
    } else if constexpr (std::is_nothrow_constructible_v<T, F> || !std::is_nothrow_move_constructible_v<T>) {
      emplace<N>(std::forward<F>(value));
    } else {
      emplace<N>(T(std::forward<F>(value)));
    }
    return *this;
  }

  constexpr std::size_t index() const noexcept {
    return index_;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  template <typename T, typename... Args>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(std::is_constructible_v<T, Args...>)
  {
    return emplace<impl::find_v<T, Ts...>>(std::forward<Args>(args)...);
  }

  template <std::size_t N, typename... Args>
  constexpr auto&& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<impl::nth_t<N, Ts...>, Args...>)
    requires(std::is_constructible_v<impl::nth_t<N, Ts...>, Args...>)
  {
    destroy();
    auto&& v = data.template construct<impl::nth_t<N, Ts...>>(std::forward<Args>(args)...);
    index_ = N;
    return v;
  }

  constexpr void swap(variant& rhs) noexcept(
      ((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>)&&...))
    requires((std::is_move_constructible_v<Ts> && std::is_swappable_v<Ts>) && ...)
  {
    if (index() == rhs.index()) {
      if (!valueless_by_exception()) {
        ::visit(impl::visitors::swap, *this, rhs);
      }
    } else {
      auto tmp = std::move(*this);
      *this = std::move(rhs);
      rhs = std::move(tmp);
    }
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend T& get(variant& v) {
    return get<impl::find_v<T, Ts...>>(v);
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend T&& get(variant&& v) {
    return get<impl::find_v<T, Ts...>>(std::move(v));
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend const T& get(const variant& v) {
    return get<impl::find_v<T, Ts...>>(v);
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend const T&& get(const variant&& v) {
    return get<impl::find_v<T, Ts...>>(std::move(v));
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend auto& get(variant& v) {
    if (v.index() != N) {
      throw bad_variant_access();
    }
    return v.data.template get<impl::nth_t<N, Ts...>>();
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend auto&& get(variant&& v) {
    if (v.index() != N) {
      throw bad_variant_access();
    }
    return std::move(v.data.template get<impl::nth_t<N, Ts...>>());
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend const auto& get(const variant& v) {
    if (v.index() != N) {
      throw bad_variant_access();
    }
    return v.data.template get<impl::nth_t<N, Ts...>>();
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend const auto&& get(const variant&& v) {
    if (v.index() != N) {
      throw bad_variant_access();
    }
    return std::move(v.data.template get<impl::nth_t<N, Ts...>>());
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend T* get_if(variant* v) noexcept {
    return get_if<impl::find_v<T, Ts...>>(v);
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend T* get_if(const variant* v) noexcept {
    return get_if<impl::find_v<T, Ts...>>(v);
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend impl::nth_t<N, Ts...>* get_if(variant* v) noexcept {
    if (v != nullptr && v->index() == N) {
      return std::addressof(get<N>(*v));
    }
    return nullptr;
  }

  template <std::size_t N>
    requires(N < sizeof...(Ts))
  constexpr friend const impl::nth_t<N, Ts...>* get_if(const variant* v) noexcept {
    if (v != nullptr && v->index() == N) {
      return std::addressof(get<N>(*v));
    }
    return nullptr;
  }

  template <typename T>
    requires(impl::count_v<T, Ts...> == 1)
  constexpr friend bool holds_alternative(const variant& v) noexcept {
    return v.index() == impl::find_v<T, Ts...>;
  }

  constexpr friend bool operator==(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() == std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() == std::declval<Ts>())> && ...)
  {
    return lhs.index() == rhs.index() && (lhs.valueless_by_exception() || ::visit(impl::visitors::equal, lhs, rhs));
  }

  constexpr friend bool operator!=(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() != std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() != std::declval<Ts>())> && ...)
  {
    return lhs.index() != rhs.index() ||
           (!lhs.valueless_by_exception() && ::visit(impl::visitors::not_equal, lhs, rhs));
  }

  constexpr friend bool operator<(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() < std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() < std::declval<Ts>())> && ...)
  {
    if (rhs.valueless_by_exception()) {
      return false;
    }
    if (lhs.valueless_by_exception()) {
      return true;
    }
    return lhs.index() < rhs.index() || (lhs.index() == rhs.index() && ::visit(impl::visitors::less, lhs, rhs));
  }

  constexpr friend bool operator>(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() > std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() > std::declval<Ts>())> && ...)
  {
    if (lhs.valueless_by_exception()) {
      return false;
    }
    if (rhs.valueless_by_exception()) {
      return true;
    }
    return lhs.index() > rhs.index() || (lhs.index() == rhs.index() && ::visit(impl::visitors::greater, lhs, rhs));
  }

  constexpr friend bool operator<=(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() <= std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() <= std::declval<Ts>())> && ...)
  {
    if (lhs.valueless_by_exception()) {
      return true;
    }
    if (rhs.valueless_by_exception()) {
      return false;
    }
    return lhs.index() < rhs.index() || (lhs.index() == rhs.index() && ::visit(impl::visitors::less_equal, lhs, rhs));
  }

  constexpr friend bool operator>=(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() >= std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() >= std::declval<Ts>())> && ...)
  {
    if (rhs.valueless_by_exception()) {
      return true;
    }
    if (lhs.valueless_by_exception()) {
      return false;
    }
    return lhs.index() > rhs.index() ||
           (lhs.index() == rhs.index() && ::visit(impl::visitors::greater_equal, lhs, rhs));
  }

  constexpr friend auto operator<=>(const variant& lhs, const variant& rhs) noexcept(
      (noexcept(std::declval<Ts>() <=> std::declval<Ts>()) && ...))
    requires(impl::type_traits::true_v<decltype(std::declval<Ts>() <=> std::declval<Ts>())> && ...)
  {
    using R = std::common_type_t<decltype(std::declval<Ts>() <=> std::declval<Ts>())...>;
    if (lhs.valueless_by_exception() && rhs.valueless_by_exception()) {
      return R::equivalent;
    }
    if (lhs.valueless_by_exception()) {
      return R::less;
    }
    if (rhs.valueless_by_exception()) {
      return R::greater;
    }
    if (lhs.index() != rhs.index()) {
      return static_cast<R>(lhs.index() <=> rhs.index());
    }
    return ::visit(impl::visitors::starship<R>, lhs, rhs);
  }

private:
  constexpr void destroy() {
    if (!valueless_by_exception()) {
      ::visit([](auto&& v) { std::destroy_at(std::addressof(v)); }, *this);
      index_ = variant_npos;
    }
  }
};
