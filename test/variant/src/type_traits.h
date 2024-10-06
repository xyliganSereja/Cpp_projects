#pragma once

#include <utility>

namespace impl::type_traits {
template <typename L, typename R>
inline constexpr bool no_cvref_equal_v = std::is_same_v<std::remove_cvref_t<L>, std::remove_cvref_t<R>>;

template <typename... Ts>
concept trivially_destructible_v = (std::is_trivially_destructible_v<Ts> && ...);

template <typename... Ts>
concept copy_constructible_v = (std::is_copy_constructible_v<Ts> && ...);

template <typename... Ts>
concept trivially_copy_constructible_v =
    copy_constructible_v<Ts...> && (std::is_trivially_copy_constructible_v<Ts> && ...);

template <typename... Ts>
concept copy_assignable_v = ((std::is_copy_constructible_v<Ts> && std::is_copy_assignable_v<Ts>)&&...);

template <typename... Ts>
concept trivially_copy_assignable_v =
    copy_assignable_v<Ts...> &&
    ((std::is_trivially_copy_constructible_v<Ts> && std::is_trivially_copy_assignable_v<Ts> &&
      std::is_trivially_destructible_v<Ts>)&&...);

template <typename... Ts>
concept move_constructible_v = (std::is_move_constructible_v<Ts> && ...);

template <typename... Ts>
concept trivially_move_constructible_v =
    move_constructible_v<Ts...> && (std::is_trivially_move_constructible_v<Ts> && ...);

template <typename... Ts>
concept move_assignable_v = ((std::is_move_constructible_v<Ts> && std::is_move_assignable_v<Ts>)&&...);

template <typename... Ts>
concept trivially_move_assignable_v =
    move_assignable_v<Ts...> &&
    ((std::is_trivially_move_constructible_v<Ts> && std::is_trivially_move_assignable_v<Ts> &&
      std::is_trivially_destructible_v<Ts>)&&...);

template <typename>
inline constexpr bool true_v = true;

template <typename F, typename T>
struct overloaded_impl {
private:
  struct test {
    T _[1];
  };

public:
  static constexpr T call(T t)
    requires true_v<decltype(test{{std::declval<F>()}})>
  {
    return t;
  }
};

template <typename F, typename... Ts>
struct overloaded : overloaded_impl<F, Ts>... {
  using overloaded_impl<F, Ts>::call...;
};

template <typename F, typename... Ts>
using overloaded_t = decltype(overloaded<F, Ts...>::call(std::declval<F>()));
} // namespace impl::type_traits
