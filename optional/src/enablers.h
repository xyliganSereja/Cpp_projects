#pragma once

namespace impl {
template <bool enable, bool enable_assign>
struct enable_copy {
  constexpr enable_copy() noexcept = default;
  constexpr enable_copy(enable_copy&&) noexcept = default;
  constexpr enable_copy& operator=(enable_copy&&) noexcept = default;
  constexpr enable_copy(const enable_copy&) = delete;
  constexpr enable_copy& operator=(const enable_copy&) = delete;
};

template <>
struct enable_copy<true, true> {};

template <>
struct enable_copy<true, false> {
  constexpr enable_copy() noexcept = default;
  constexpr enable_copy(enable_copy&&) noexcept = default;
  constexpr enable_copy& operator=(enable_copy&&) noexcept = default;
  constexpr enable_copy(const enable_copy&) noexcept = default;
  constexpr enable_copy& operator=(const enable_copy&) = delete;
};

template <bool enable, bool enable_assign>
struct enable_move {
  constexpr enable_move() noexcept = default;
  constexpr enable_move(enable_move&&) = delete;
  constexpr enable_move& operator=(enable_move&&) = delete;
  constexpr enable_move(const enable_move&) noexcept = default;
  constexpr enable_move& operator=(const enable_move&) noexcept = default;
};

template <>
struct enable_move<true, true> {};

template <>
struct enable_move<true, false> {
  constexpr enable_move() noexcept = default;
  constexpr enable_move(enable_move&&) = default;
  constexpr enable_move& operator=(enable_move&&) = delete;
  constexpr enable_move(const enable_move&) noexcept = default;
  constexpr enable_move& operator=(const enable_move&) noexcept = default;
};
} // namespace impl
