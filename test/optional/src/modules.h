#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace impl {
template <bool cond, typename type>
struct skip_inheritance_if;

template <typename type_>
struct skip_inheritance_if<true, type_> {
  using type = type_::base;
};

template <typename type_>
struct skip_inheritance_if<false, type_> {
  using type = type_;
};

template <bool cond, typename type_>
using skip_inheritance_if_t = skip_inheritance_if<cond, type_>::type;

template <class T, bool trivially_destructible = std::is_trivially_destructible_v<T>>
union storage;

template <class T>
union storage<T, false> {
  constexpr storage() noexcept {}

  constexpr ~storage() noexcept {}

  constexpr storage(storage&&) noexcept = default;
  constexpr storage& operator=(storage&&) noexcept = default;
  constexpr storage(const storage&) noexcept = default;
  constexpr storage& operator=(const storage&) noexcept = default;

  T data;
};

template <class T>
union storage<T, true> {
  constexpr storage() {}

  constexpr ~storage() noexcept = default;
  constexpr storage(storage&&) noexcept = default;
  constexpr storage& operator=(storage&&) noexcept = default;
  constexpr storage(const storage&) noexcept = default;
  constexpr storage& operator=(const storage&) noexcept = default;

  T data;
};

template <typename T>
struct optional_base {
private:
  storage<T> data;
  bool is_present = false;

public:
  constexpr optional_base() noexcept = default;

  constexpr void reset() noexcept {
    if (is_present) {
      data.data.~T();
      is_present = false;
    }
  }

  constexpr T& value() & {
    if (!is_present) {
      throw std::runtime_error("Empty optional");
    }
    return **this;
  }

  constexpr const T& value() const& {
    if (!is_present) {
      throw std::runtime_error("Empty optional");
    }
    return **this;
  }

  constexpr T&& value() && {
    if (!is_present) {
      throw std::runtime_error("Empty optional");
    }
    return std::move(**this);
  }

  constexpr const T&& value() const&& {
    if (!is_present) {
      throw std::runtime_error("Empty optional");
    }
    return std::move(**this);
  }

  constexpr T& operator*() & {
    return data.data;
  }

  constexpr const T& operator*() const& {
    return data.data;
  }

  constexpr T&& operator*() && {
    return std::move(data.data);
  }

  constexpr const T&& operator*() const&& {
    return std::move(data.data);
  }

  constexpr T* operator->() noexcept {
    return &operator*();
  }

  constexpr const T* operator->() const noexcept {
    return &operator*();
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) {
    reset();
    std::construct_at(&data.data, std::forward<Args>(args)...);
    is_present = true;
  }

  constexpr explicit operator bool() const noexcept {
    return is_present;
  }

  constexpr optional_base(optional_base&&) noexcept = default;
  constexpr optional_base& operator=(optional_base&&) noexcept = default;
  constexpr optional_base(const optional_base&) noexcept = default;
  constexpr optional_base& operator=(const optional_base&) noexcept = default;
};

template <typename T>
struct non_triv_destruct_base : optional_base<T> {
  using base = optional_base<T>;
  using base::base;

  constexpr ~non_triv_destruct_base() noexcept {
    this->reset();
  }
};

template <typename T>
using destruct_base_t = skip_inheritance_if_t<std::is_trivially_destructible_v<T>, non_triv_destruct_base<T>>;

template <typename T>
struct non_triv_move_constr_base : destruct_base_t<T> {
  using base = destruct_base_t<T>;
  using base::base;

  constexpr non_triv_move_constr_base(non_triv_move_constr_base&& other) : base() {
    if (other) {
      this->emplace(std::move(*other));
    }
  }

  constexpr non_triv_move_constr_base& operator=(non_triv_move_constr_base&&) noexcept = default;
  constexpr non_triv_move_constr_base(const non_triv_move_constr_base&) noexcept = default;
  constexpr non_triv_move_constr_base& operator=(const non_triv_move_constr_base&) noexcept = default;
};

template <typename T>
using move_constr_base_t =
    skip_inheritance_if_t<std::is_trivially_move_constructible_v<T>, non_triv_move_constr_base<T>>;

template <typename T>
struct non_triv_move_assign_base : move_constr_base_t<T> {
  using base = move_constr_base_t<T>;
  using base::base;

  constexpr non_triv_move_assign_base(non_triv_move_assign_base&& other) noexcept = default;

  constexpr non_triv_move_assign_base& operator=(non_triv_move_assign_base&& other) {
    if (*this && other) {
      **this = std::move(*other);
    } else if (other) {
      this->emplace(std::move(*other));
    } else {
      this->reset();
    }
    return *this;
  }

  constexpr non_triv_move_assign_base(const non_triv_move_assign_base&) noexcept = default;
  constexpr non_triv_move_assign_base& operator=(const non_triv_move_assign_base&) noexcept = default;
};

template <typename T>
using move_assign_base_t =
    skip_inheritance_if_t<std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T>,
                          non_triv_move_assign_base<T>>;

template <typename T>
struct non_triv_copy_constr_base : move_assign_base_t<T> {
  using base = move_assign_base_t<T>;
  using base::base;

  constexpr non_triv_copy_constr_base(non_triv_copy_constr_base&& other) noexcept = default;
  constexpr non_triv_copy_constr_base& operator=(non_triv_copy_constr_base&& other) noexcept = default;

  constexpr non_triv_copy_constr_base(const non_triv_copy_constr_base& other) : base() {
    if (other) {
      this->emplace(*other);
    }
  }

  constexpr non_triv_copy_constr_base& operator=(const non_triv_copy_constr_base& other) noexcept = default;
};

template <typename T>
using copy_constr_base_t =
    skip_inheritance_if_t<std::is_trivially_copy_constructible_v<T>, non_triv_copy_constr_base<T>>;

template <typename T>
struct non_triv_copy_assign_base : copy_constr_base_t<T> {
  using base = copy_constr_base_t<T>;
  using base::base;

  constexpr non_triv_copy_assign_base& operator=(non_triv_copy_assign_base&& other) noexcept = default;
  constexpr non_triv_copy_assign_base(non_triv_copy_assign_base&& other) noexcept = default;
  constexpr non_triv_copy_assign_base(const non_triv_copy_assign_base& other) noexcept = default;

  constexpr non_triv_copy_assign_base& operator=(const non_triv_copy_assign_base& other) {
    if (*this && other) {
      **this = *other;
    } else if (other) {
      this->emplace(*other);
    } else {
      this->reset();
    }
    return *this;
  }
};

template <typename T>
using copy_assign_base_t =
    skip_inheritance_if_t<std::is_trivially_copy_assignable_v<T> && std::is_trivially_copy_constructible_v<T>,
                          non_triv_copy_assign_base<T>>;

template <typename T>
using modules = copy_assign_base_t<T>;
} // namespace impl
