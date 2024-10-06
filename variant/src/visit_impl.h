#pragma once

#include <utility>

namespace impl::visit_impl {
template <typename... Ts>
struct pack {};

template <std::size_t... Indexes>
struct multiindex {};

// make_seq<N>::type == multiindex<1, 2, ..., N>
template <std::size_t N, typename Res = multiindex<>>
struct make_seq;

template <std::size_t... Vals, std::size_t N>
struct make_seq<N, multiindex<Vals...>> {
  using type = make_seq<N, multiindex<Vals..., sizeof...(Vals)>>::type;
};

template <std::size_t... Vals, std::size_t N>
  requires(sizeof...(Vals) == N)
struct make_seq<N, multiindex<Vals...>> {
  using type = multiindex<Vals...>;
};

// unite<pack<A1, ..., AN>, ..., pack<Z1, ..., ZM>>::type == pack<A1, ..., AN, ..., Z1, ..., ZM>
template <typename... Packs>
struct unite;

template <typename Pack>
struct unite<Pack> {
  using type = Pack;
};

template <typename... Ts1, typename... Ts2, typename... Packs>
struct unite<pack<Ts1...>, pack<Ts2...>, Packs...> {
  using type = unite<pack<Ts1..., Ts2...>, Packs...>::type;
};

// add_index<X, pack<multiindex<a1, ..., aN>, ..., multiindex<z1, ..., zM>>::type
//     == pack<multiindex<a1, ..., aN, X>, ..., multiindex<z1, ..., zM, X>>
template <std::size_t Index, typename Pack>
struct add_index;

template <std::size_t Index>
struct add_index<Index, pack<>> {
  using type = pack<>;
};

template <std::size_t Index, std::size_t... Indexes, typename... Multiindexes>
struct add_index<Index, pack<multiindex<Indexes...>, Multiindexes...>> {
  using type = unite<pack<multiindex<Indexes..., Index>>, typename add_index<Index, pack<Multiindexes...>>::type>::type;
};

// add_indexes<multiindex<x1, ..., xk>, pack<multiindex<a1, ..., aN>, ..., multiindex<z1, ..., zM>>::type
//     == pack<multiindex<a1, ..., aN, x1>, ..., multiindex<z1, ..., zM, x1>, ...,
//             multiindex<a1, ..., aN, xk>, ..., multiindex<z1, ..., zM, xk>>
template <typename Multiindex, typename Pack>
struct add_indexes;

template <typename Pack>
struct add_indexes<multiindex<>, Pack> {
  using type = Pack;
};

template <std::size_t... Indexes, typename... Multiindexes>
struct add_indexes<multiindex<Indexes...>, pack<Multiindexes...>> {
  using type = unite<typename add_index<Indexes, pack<Multiindexes...>>::type...>::type;
};

// all_multiindexes<multiindex<x1, ..., xk>>::type
//     == pack<..., multiindex<a1, ..., ak>, ...> for a1 in [1, x1], ..., ak in [1, xk]
template <typename Bounds, typename Result = pack<multiindex<>>>
struct all_multiindexes;

template <typename Pack>
struct all_multiindexes<multiindex<>, Pack> {
  using type = Pack;
};

template <std::size_t Bound, typename... Results>
struct all_multiindexes<multiindex<Bound>, pack<Results...>> {
  using type = typename add_indexes<typename make_seq<Bound>::type, pack<Results...>>::type;
};

template <std::size_t Bound, std::size_t... Bounds, typename... Results>
  requires(sizeof...(Bounds) > 0)
struct all_multiindexes<multiindex<Bound, Bounds...>, pack<Results...>> {
  using type = all_multiindexes<multiindex<Bounds...>,
                                typename add_indexes<typename make_seq<Bound>::type, pack<Results...>>::type>::type;
};

constexpr std::size_t multiindex_to_index(multiindex<>) {
  return 0;
}

template <std::size_t Bound, std::size_t... Bounds>
constexpr std::size_t multiindex_to_index(multiindex<Bound, Bounds...>, std::size_t index, auto... indexes) {
  return Bound * multiindex_to_index(multiindex<Bounds...>{}, indexes...) + index;
}

template <typename R, typename Vis, typename Bounds, typename... Variants>
struct func_table;

template <typename R, typename Vis, typename... Variants, std::size_t... sizes>
struct func_table<R, Vis, multiindex<sizes...>, Variants...> {
private:
  static constexpr std::size_t table_size = (1 * ... * sizes);
  using func = R (*)(Vis&&, Variants&&...);
  using bounds = multiindex<sizes...>;

  func table[table_size];

  template <std::size_t... indexes>
  constexpr void add_func(visit_impl::multiindex<indexes...>) {
    table[multiindex_to_index(bounds{}, indexes...)] = [](Vis&& vis, Variants&&... vars) -> R {
      return std::forward<Vis>(vis)(get<indexes>(std::forward<Variants>(vars))...);
    };
  }

  template <typename... Multiindexes>
  constexpr void make_table_impl(visit_impl::pack<Multiindexes...>) {
    (add_func(Multiindexes{}), ...);
  }

public:
  constexpr func_table() {
    make_table_impl(typename visit_impl::all_multiindexes<bounds>::type{});
  }

  constexpr func get_overload(auto... indexes) const {
    return table[multiindex_to_index(bounds{}, indexes...)];
  }
};

template <typename Vis, typename Bounds, typename... Variants>
constexpr func_table<Vis, Bounds, Variants...> func_table_v{};
} // namespace impl::visit_impl
