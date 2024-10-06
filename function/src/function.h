#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

class bad_function_call : public std::exception {
public:
  const char* what() const noexcept override {
    return "bad function call";
  }
};

namespace impl {
inline constexpr size_t DATA_SIZE = std::max(sizeof(std::byte*), alignof(std::max_align_t) - sizeof(std::byte*));

template <typename R, typename... Args>
class descriptor_base {
public:
  virtual void deconstruct(std::byte*) const noexcept = 0;
  virtual R call(std::byte* func, Args... args) const = 0;
  virtual void move(std::byte* data, std::byte* to_data) const noexcept = 0;
  virtual void copy(const std::byte* data, std::byte* to_data) const = 0;
};

template <typename T, typename R, typename... Args>
class descriptor : public descriptor_base<R, Args...> {
  static void set_pointer(std::byte* data, T* new_val) noexcept {
    new (data) T*(std::move(new_val));
  }

public:
  void deconstruct(std::byte* data) const noexcept override {
    delete get_pointer(data);
  }

  R call(std::byte* data, Args... args) const override {
    return (*get_pointer(data))(std::forward<Args>(args)...);
  }

  void move(std::byte* from_data, std::byte* to_data) const noexcept override {
    set_pointer(to_data, get_pointer(from_data));
    set_pointer(from_data, nullptr);
  }

  void copy(const std::byte* from_data, std::byte* to_data) const override {
    set_pointer(to_data, new T(*get_pointer(from_data)));
  }

  static void relocate(T&& data, std::byte* to_data) {
    set_pointer(to_data, new T(std::move(data)));
  }

  static T* get_pointer(std::byte* data) noexcept {
    return std::launder(*std::launder(reinterpret_cast<T**>(data)));
  }

  static const T* get_pointer(const std::byte* data) noexcept {
    return std::launder(*std::launder(reinterpret_cast<const T* const*>(data)));
  }
};

template <typename T, typename R, typename... Args>
  requires(sizeof(T) <= DATA_SIZE) && std::is_nothrow_move_constructible_v<T>
class descriptor<T, R, Args...> : public descriptor_base<R, Args...> {
public:
  void deconstruct(std::byte* data) const noexcept override {
    get_pointer(data)->~T();
  }

  R call(std::byte* data, Args... args) const override {
    return (*get_pointer(data))(std::forward<Args>(args)...);
  }

  void move(std::byte* from_data, std::byte* to_data) const noexcept override {
    new (to_data) T(std::move(*get_pointer(from_data)));
    deconstruct(from_data);
  }

  void copy(const std::byte* from_data, std::byte* to_data) const override {
    new (to_data) T(*get_pointer(from_data));
  }

  static void relocate(T&& data, std::byte* to_data) noexcept {
    new (to_data) T(std::move(data));
  }

  static T* get_pointer(std::byte* data) noexcept {
    return std::launder(reinterpret_cast<T*>(data));
  }

  static const T* get_pointer(const std::byte* data) noexcept {
    return std::launder(reinterpret_cast<const T*>(data));
  }
};

template <typename R, typename... Args>
class empty_descriptor : public descriptor_base<R, Args...> {
  void deconstruct(std::byte*) const noexcept override {}

  R call(std::byte*, Args...) const override {
    throw bad_function_call();
  }

  void move(std::byte*, std::byte*) const noexcept override {}

  void copy(const std::byte*, std::byte*) const noexcept override {}
};

template <typename T, typename R, typename... Args>
inline constexpr descriptor DESCRIPTOR = descriptor<T, R, Args...>();
template <typename R, typename... Args>
inline constexpr empty_descriptor EMPTY_DESCRIPTOR = empty_descriptor<R, Args...>();
} // namespace impl

template <typename F>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
private:
  alignas(std::max_align_t) mutable std::byte data[impl::DATA_SIZE];
  const impl::descriptor_base<R, Args...>* descriptor;

public:
  function() noexcept : descriptor(&impl::EMPTY_DESCRIPTOR<R, Args...>) {}

  template <typename F>
  function(F func) : descriptor(&impl::DESCRIPTOR<F, R, Args...>) {
    impl::DESCRIPTOR<F, R, Args...>.relocate(std::move(func), data);
  }

  function(const function& other) : descriptor(other.descriptor) {
    descriptor->copy(other.data, data);
  }

  function(function&& other) noexcept
      : descriptor(std::exchange(other.descriptor, &impl::EMPTY_DESCRIPTOR<R, Args...>)) {
    descriptor->move(other.data, data);
  }

  function& operator=(const function& other) {
    if (&other != this) {
      *this = function(other);
    }
    return *this;
  }

  function& operator=(function&& other) noexcept {
    if (&other != this) {
      descriptor->deconstruct(data);
      descriptor = std::exchange(other.descriptor, &impl::EMPTY_DESCRIPTOR<R, Args...>);
      descriptor->move(other.data, data);
    }
    return *this;
  }

  ~function() {
    descriptor->deconstruct(data);
  }

  explicit operator bool() const noexcept {
    return descriptor != &impl::EMPTY_DESCRIPTOR<R, Args...>;
  }

  R operator()(Args... args) const {
    return descriptor->call(data, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    if (descriptor == &impl::DESCRIPTOR<T, R, Args...>) {
      return impl::DESCRIPTOR<T, R, Args...>.get_pointer(data);
    }
    return nullptr;
  }

  template <typename T>
  const T* target() const noexcept {
    if (descriptor == &impl::DESCRIPTOR<T, R, Args...>) {
      return impl::DESCRIPTOR<T, R, Args...>.get_pointer(data);
    }
    return nullptr;
  }
};
