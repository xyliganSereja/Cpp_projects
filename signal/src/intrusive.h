#pragma once

namespace signals::impl {

struct intrusive {
  intrusive *prev, *next;

  intrusive() noexcept;

  explicit intrusive(intrusive* list) noexcept;

  ~intrusive();

  void remove() noexcept;

  void insert(intrusive* list) noexcept;
};
} // namespace signals::impl
