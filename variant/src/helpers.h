#pragma once

inline constexpr std::size_t variant_npos = -1;

namespace impl {

/// nth

template <std::size_t N, typename T, typename... Ts>
struct nth {
  using type = nth<N - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct nth<0, T, Ts...> {
  using type = T;
};

template <std::size_t N, typename... Ts>
using nth_t = nth<N, Ts...>::type;

/// find

template <typename F, typename... Ts>
struct find : std::integral_constant<std::size_t, variant_npos> {};

template <typename F, typename T, typename... Ts>
  requires(find<F, Ts...>::value != variant_npos)
struct find<F, T, Ts...> : std::integral_constant<std::size_t, 1 + find<F, Ts...>::value> {};

template <typename T, typename... Ts>
struct find<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename F, typename... Ts>
inline constexpr std::size_t find_v = find<F, Ts...>::value;

/// count

template <typename F, typename... Ts>
struct count : std::integral_constant<std::size_t, 0> {};

template <typename F, typename T, typename... Ts>
struct count<F, T, Ts...> : std::integral_constant<std::size_t, count<F, Ts...>::value> {};

template <typename T, typename... Ts>
struct count<T, T, Ts...> : std::integral_constant<std::size_t, 1 + count<T, Ts...>::value> {};

template <typename F, typename... Ts>
inline constexpr std::size_t count_v = count<F, Ts...>::value;
} // namespace impl
