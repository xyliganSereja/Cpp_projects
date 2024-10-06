#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  class data_buffer {
  private:
    size_t _capacity;
    size_t _follow_count;
    T _data[0];

  public:
    explicit data_buffer(size_t capacity) : _capacity(capacity), _follow_count(1) {}

    data_buffer(const data_buffer&) = delete;
    data_buffer& operator=(const data_buffer&) = delete;

    bool single_follower() const noexcept {
      return _follow_count == 1;
    }

    pointer data() noexcept {
      return _data;
    }

    const_pointer data() const noexcept {
      return _data;
    }

    data_buffer* follow() noexcept {
      ++_follow_count;
      return this;
    }

    void unfollow() noexcept {
      --_follow_count;
    }

    size_t capacity() const noexcept {
      return _capacity;
    }
  };

public:
  socow_vector() noexcept : _is_small(true), _size(0) {}

  socow_vector(const socow_vector& other) : socow_vector() {
    *this = other;
  }

  socow_vector(size_t capacity) : _is_small(capacity <= SMALL_SIZE), _size(0) {
    if (!is_small()) {
      _big_data = new (operator new(sizeof(data_buffer) + capacity * sizeof(T))) data_buffer(capacity);
    }
  }

  socow_vector& operator=(const socow_vector& other) {
    if (this == &other) {
      return *this;
    }
    if (other.is_small() && is_small()) {
      socow_vector tmp;
      tmp.put_back(other.cbegin(), std::min(size(), other.size()));
      if (size() < other.size()) {
        put_back(other.cbegin() + size(), other.size() - size());
      } else {
        remove_back(size() - other.size());
      }
      std::swap_ranges(tmp.raw_begin(), tmp.raw_end(), raw_begin());
    } else if (other.is_small()) {
      safe_copy(other.capacity(), other.cbegin(), other.cend());
    } else {
      release_data();
      _big_data = other._big_data->follow();
      _size = other.size();
      _is_small = other.is_small();
    }
    return *this;
  }

  ~socow_vector() noexcept {
    release_data();
  }

  reference operator[](size_t index) {
    assert(index < size());
    return data()[index];
  }

  const_reference operator[](size_t index) const {
    assert(index < size());
    return data()[index];
  }

  pointer data() {
    if (copy_required()) {
      reserve(size());
    }
    return is_small() ? _small_data : _big_data->data();
  }

  const_pointer data() const noexcept {
    return is_small() ? _small_data : _big_data->data();
  }

  size_t size() const noexcept {
    return _size;
  }

  reference front() {
    assert(!empty());
    return *begin();
  }

  const_reference front() const {
    assert(!empty());
    return *begin();
  }

  reference back() {
    assert(!empty());
    return *(end() - 1);
  }

  const_reference back() const {
    assert(!empty());
    return *(end() - 1);
  }

  void push_back(const T& value) {
    insert(cend(), value);
  }

  void pop_back() {
    assert(!empty());
    erase(cend() - 1);
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t capacity() const noexcept {
    return is_small() ? SMALL_SIZE : _big_data->capacity();
  }

  void reserve(size_t new_capacity) {
    if ((size() <= new_capacity && copy_required()) || capacity() < new_capacity) {
      set_capacity(new_capacity);
    }
  }

  void shrink_to_fit() {
    if (!is_small() && size() != capacity()) {
      set_capacity(size());
    }
  }

  void clear() {
    if (copy_required()) {
      _big_data->unfollow();
      _is_small = true;
      _size = 0;
    } else {
      remove_back(size());
    }
  }

  void swap(socow_vector& other) {
    if (is_small() && other.is_small()) {
      socow_vector& smaller = size() <= other.size() ? *this : other;
      socow_vector& bigger = size() <= other.size() ? other : *this;
      size_t size_diff = bigger.size() - smaller.size();
      smaller.put_back(bigger.cbegin() + smaller.size(), size_diff);
      bigger.remove_back(size_diff);
      std::swap_ranges(bigger.raw_begin(), bigger.raw_end(), smaller.raw_begin());
    } else {
      socow_vector& big = is_small() ? other : *this;
      socow_vector& another = is_small() ? *this : other;

      auto tmp = big;
      big = another;
      another = tmp;
    }
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return data() + size();
  }

  const_iterator begin() const noexcept {
    return cbegin();
  }

  const_iterator end() const noexcept {
    return cend();
  }

  const_iterator cbegin() const noexcept {
    return data();
  }

  const_iterator cend() const noexcept {
    return data() + size();
  }

  reverse_iterator rbegin() {
    return std::make_reverse_iterator(end());
  }

  reverse_iterator rend() {
    return std::make_reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const noexcept {
    return crbegin();
  }

  const_reverse_iterator rend() const noexcept {
    return crend();
  }

  const_reverse_iterator crbegin() const noexcept {
    return std::make_reverse_iterator(cend());
  }

  const_reverse_iterator crend() const noexcept {
    return std::make_reverse_iterator(cbegin());
  }

  iterator insert(const_iterator pos, const T& value) {
    size_t pos_index = pos - cbegin();
    if (copy_required() || size() == capacity()) {
      socow_vector tmp(size() < capacity() ? capacity() : capacity() * 2);
      tmp.put_back(cbegin(), pos);
      tmp.push_back(value);
      tmp.put_back(pos, cend());
      *this = tmp;
    } else {
      new (raw_end()) T(value);
      ++_size;
      std::rotate(raw_begin() + pos_index, raw_end() - 1, raw_end());
    }
    return raw_begin() + pos_index;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t first_index = first - cbegin();
    size_t diff = last - first;
    if (copy_required()) {
      safe_copy(size() - diff, cbegin(), first, last, cend());
    } else if (diff) {
      for (size_t i = first_index; i < size() - diff; ++i) {
        std::iter_swap(raw_begin() + i, raw_begin() + i + diff);
      }
      remove_back(diff);
    }
    return raw_begin() + first_index;
  }

private:
  bool is_small() const noexcept {
    return _is_small;
  }

  void set_capacity(size_t new_capacity) {
    if (new_capacity > SMALL_SIZE) {
      socow_vector tmp(new_capacity);
      tmp.put_back(cbegin(), cend());
      *this = tmp;
    } else if (!is_small()) {
      safe_copy(new_capacity, cbegin(), cend());
    }
  }

  void safe_copy(size_t new_capacity, const_iterator first1, const_iterator last1) {
    safe_copy(new_capacity, first1, last1, cend(), cend());
  }

  void safe_copy(size_t new_capacity, const_iterator first1, const_iterator last1, const_iterator first2,
                 const_iterator last2) {
    assert(!is_small());
    socow_vector tmp = *this;
    clear();
    reserve(new_capacity);
    try {
      put_back(first1, last1);
      put_back(first2, last2);
    } catch (...) {
      *this = tmp;
      throw;
    }
  }

  void release_data() noexcept {
    if (copy_required()) {
      _big_data->unfollow();
    } else {
      std::destroy(raw_rbegin(), raw_rend());
      if (!is_small()) {
        _big_data->~data_buffer();
        operator delete(_big_data);
      }
    }
  }

  bool copy_required() const noexcept {
    return !is_small() && !_big_data->single_follower();
  }

  void put_back(const_iterator first, size_t n) {
    put_back(first, first + n);
  }

  void put_back(const_iterator first, const_iterator last) {
    size_t diff = last - first;
    assert(size() + diff <= capacity() && !copy_required());
    std::uninitialized_copy(first, last, raw_end());
    _size += diff;
  }

  void remove_back(size_t n) {
    assert(n <= size() && !copy_required());
    std::destroy_n(raw_rbegin(), n);
    _size -= n;
  }

  pointer raw_data() noexcept {
    return is_small() ? _small_data : _big_data->data();
  }

  pointer raw_begin() noexcept {
    return raw_data();
  }

  pointer raw_end() noexcept {
    return raw_begin() + size();
  }

  const_reverse_iterator raw_rbegin() noexcept {
    return std::make_reverse_iterator(raw_end());
  }

  const_reverse_iterator raw_rend() noexcept {
    return std::make_reverse_iterator(raw_begin());
  }

private:
  bool _is_small;
  size_t _size;

  union {
    data_buffer* _big_data;
    T _small_data[SMALL_SIZE];
  };
};
