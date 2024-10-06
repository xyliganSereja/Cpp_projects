#pragma once

#include <cstddef>
#include <memory>
#include <utility>

template <typename T>
class shared_ptr;

namespace impl {

class ctrl_block_base {
public:
  virtual ~ctrl_block_base() = default;

  void add_strong_ref() noexcept {
    ++_strong_refs;
  }

  void release_strong_ref() noexcept {
    if (!(--_strong_refs)) {
      delete_data();
      release_weak_ref();
    }
  }

  void add_weak_ref() noexcept {
    ++_weak_refs;
  }

  void release_weak_ref() noexcept {
    if (!(--_weak_refs)) {
      delete this;
    }
  }

  size_t get_strong_refs() const noexcept {
    return _strong_refs;
  }

private:
  virtual void delete_data() noexcept = 0;

private:
  size_t _strong_refs = 1;
  size_t _weak_refs = 1;
};

template <typename T, typename Deleter>
class external_ctrl_block : public ctrl_block_base {

public:
  external_ctrl_block(T* ptr, Deleter deleter) noexcept : _data(ptr), _deleter(std::move(deleter)) {}

private:
  void delete_data() noexcept override {
    _deleter(_data);
  }

private:
  T* _data;
  [[no_unique_address]] Deleter _deleter;
};

template <typename T>
class inplace_control_block : public ctrl_block_base {
  template <typename T2>
  friend class ::shared_ptr;

public:
  template <typename... Args>
  inplace_control_block(Args&&... args) {
    std::construct_at(&_storage.data, std::forward<Args>(args)...);
  }

private:
  void delete_data() noexcept override {
    std::destroy_at(&_storage.data);
  }

  union storage {
    storage() {}

    ~storage() {}

    T data;
  } _storage;
};

} // namespace impl

template <typename T>
class shared_ptr {
  template <typename>
  friend class shared_ptr;

  template <typename>
  friend class weak_ptr;

  template <typename T2, typename... Args>
  friend shared_ptr<T2> make_shared(Args&&... args);

public:
  ~shared_ptr() {
    if (_ctrl_block) {
      _ctrl_block->release_strong_ref();
    }
  }

  shared_ptr() noexcept = default;

  shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}

  template <typename Y, typename Deleter = std::default_delete<Y>>
    requires std::is_convertible_v<Y*, T*>
  explicit shared_ptr(Y* ptr, Deleter deleter = std::default_delete<Y>()) : _ptr(ptr) {
    try {
      _ctrl_block = new impl::external_ctrl_block(ptr, std::move(deleter));
    } catch (...) {
      deleter(ptr);
      throw;
    }
  }

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other, T* ptr) noexcept : shared_ptr(other._ctrl_block, ptr) {}

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other, T* ptr) noexcept
      : _ctrl_block(std::exchange(other._ctrl_block, nullptr)),
        _ptr(ptr) {
    other._ptr = nullptr;
  }

  shared_ptr(const shared_ptr& other) noexcept : shared_ptr(other, other._ptr) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr(const shared_ptr<Y>& other) noexcept : shared_ptr(other, other._ptr) {}

  shared_ptr(shared_ptr&& other) noexcept
      : _ctrl_block(std::exchange(other._ctrl_block, nullptr)),
        _ptr(std::exchange(other._ptr, nullptr)) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr(shared_ptr<Y>&& other) noexcept
      : _ctrl_block(std::exchange(other._ctrl_block, nullptr)),
        _ptr(std::exchange(other._ptr, nullptr)) {}

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (this != &other) {
      shared_ptr(other).swap(*this);
    }
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    shared_ptr(other).swap(*this);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr& operator=(shared_ptr<Y>&& other) noexcept {
    shared_ptr(std::move(other)).swap(*this);
    return *this;
  }

  T* get() const noexcept {
    return _ptr;
  }

  operator bool() const noexcept {
    return _ptr != nullptr;
  }

  T& operator*() const noexcept {
    return *get();
  }

  T* operator->() const noexcept {
    return get();
  }

  std::size_t use_count() const noexcept {
    return _ctrl_block ? _ctrl_block->get_strong_refs() : 0;
  }

  void reset() noexcept {
    shared_ptr().swap(*this);
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  void reset(Y* new_ptr) {
    shared_ptr(new_ptr).swap(*this);
  }

  template <typename Y, typename Deleter>
    requires std::is_convertible_v<Y*, T*>
  void reset(Y* new_ptr, Deleter deleter) {
    shared_ptr(new_ptr, std::move(deleter)).swap(*this);
  }

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return lhs._ptr == rhs._ptr;
  }

  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return !(lhs == rhs);
  }

  void swap(shared_ptr& other) noexcept {
    std::swap(_ctrl_block, other._ctrl_block);
    std::swap(_ptr, other._ptr);
  }

private:
  impl::ctrl_block_base* _ctrl_block = nullptr;
  T* _ptr = nullptr;

  explicit shared_ptr(impl::inplace_control_block<T>* ctrlBlock) noexcept
      : _ctrl_block(ctrlBlock),
        _ptr(ctrlBlock ? &ctrlBlock->_storage.data : nullptr) {}

  shared_ptr(impl::ctrl_block_base* ctrlBlock, T* ptr) noexcept : _ctrl_block(ctrlBlock), _ptr(ptr) {
    if (_ctrl_block) {
      _ctrl_block->add_strong_ref();
    }
  }
};

template <typename T>
class weak_ptr {
  template <typename>
  friend class weak_ptr;

public:
  ~weak_ptr() {
    if (_ctrl_block) {
      _ctrl_block->release_weak_ref();
    }
  }

  weak_ptr() noexcept = default;

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(const shared_ptr<Y>& other) noexcept : weak_ptr(other._ctrl_block, other._ptr) {}

  weak_ptr(const weak_ptr& other) noexcept : weak_ptr(other._ctrl_block, other._ptr) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(const weak_ptr<Y>& other) noexcept : weak_ptr(other._ctrl_block, other._ptr) {}

  weak_ptr(weak_ptr&& other) noexcept
      : _ctrl_block(std::exchange(other._ctrl_block, nullptr)),
        _ptr(std::exchange(other._ptr, nullptr)) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(weak_ptr<Y>&& other) noexcept
      : _ctrl_block(std::exchange(other._ctrl_block, nullptr)),
        _ptr(std::exchange(other._ptr, nullptr)) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    if (this != &other) {
      weak_ptr(other).swap(*this);
    }
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(const weak_ptr<Y>& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(weak_ptr<Y>&& other) noexcept {
    weak_ptr(std::move(other)).swap(*this);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    return expired() ? shared_ptr<T>() : shared_ptr(_ctrl_block, _ptr);
  }

  void reset() noexcept {
    weak_ptr().swap(*this);
  }

  void swap(weak_ptr& other) noexcept {
    std::swap(_ctrl_block, other._ctrl_block);
    std::swap(_ptr, other._ptr);
  }

  bool expired() const noexcept {
    return !_ctrl_block || !_ctrl_block->get_strong_refs();
  }

private:
  impl::ctrl_block_base* _ctrl_block = nullptr;
  T* _ptr = nullptr;

  weak_ptr(impl::ctrl_block_base* ctrlBlock, T* ptr) noexcept : _ctrl_block(ctrlBlock), _ptr(ptr) {
    if (_ctrl_block) {
      _ctrl_block->add_weak_ref();
    }
  }
};

template <typename T2, typename... Args>
shared_ptr<T2> make_shared(Args&&... args) {
  auto* cb = new impl::inplace_control_block<T2>(std::forward<Args>(args)...);
  return shared_ptr(cb);
}
