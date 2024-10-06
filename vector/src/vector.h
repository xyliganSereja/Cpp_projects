#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>

template <typename T>
class vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  vector() noexcept : _data(nullptr), _size(0), _capacity(0) {} // O(1) nothrow

  vector(const vector& other) : vector(other, other.size()) {} // O(N) strong

  vector& operator=(const vector& other) & {
    vector tmp(other);
    swap(tmp);
    return *this;
  } // O(N) strong

  ~vector() noexcept {
    clear();
    operator delete(_data);
  } // O(N) nothrow

  reference operator[](size_t index) {
    assert(index < size());
    return data()[index];
  } // O(1) nothrow

  const_reference operator[](size_t index) const {
    assert(index < size());
    return data()[index];
  } // O(1) nothrow

  pointer data() noexcept {
    return _data;
  } // O(1) nothrow

  const_pointer data() const noexcept {
    return _data;
  } // O(1) nothrow

  size_t size() const noexcept {
    return _size;
  } // O(1) nothrow

  reference front() {
    assert(!empty());
    return *begin();
  } // O(1) nothrow

  const_reference front() const {
    assert(!empty());
    return *begin();
  } // O(1) nothrow

  reference back() {
    assert(!empty());
    return *(end() - 1);
  } // O(1) nothrow

  const_reference back() const {
    assert(!empty());
    return *(end() - 1);
  } // O(1) nothrow

  void push_back(const T& value) {
    if (size() == capacity()) {
      vector tmp(*this, size() * 2 + 1);
      tmp.push_back(value);
      swap(tmp);
    } else {
      new (&_data[size()]) T(value);
      ++_size;
    }
  } // O(1)* strong

  void pop_back() {
    assert(!empty());
    back().~T();
    --_size;
  } // O(1) nothrow

  bool empty() const noexcept {
    return size() == 0;
  } // O(1) nothrow

  size_t capacity() const noexcept {
    return _capacity;
  } // O(1) nothrow

  void reserve(size_t new_capacity) {
    if (capacity() < new_capacity) {
      vector tmp(*this, new_capacity);
      swap(tmp);
    }
  } // O(N) strong

  void shrink_to_fit() {
    if (size() != capacity()) {
      vector tmp(*this, size());
      swap(tmp);
    }
  } // O(N) strong

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  } // O(N) nothrow

  void swap(vector& other) noexcept {
    std::swap(_data, other._data);
    std::swap(_size, other._size);
    std::swap(_capacity, other._capacity);
  } // O(1) nothrow

  iterator begin() noexcept {
    return data();
  } // O(1) nothrow

  iterator end() noexcept {
    return data() + size();
  } // O(1) nothrow

  const_iterator begin() const noexcept {
    return data();
  } // O(1) nothrow

  const_iterator end() const noexcept {
    return data() + size();
  } // O(1) nothrow

  iterator insert(const_iterator pos, const T& value) {
    size_t pos_index = pos - begin();
    push_back(value);
    for (size_t i = size() - 1; i > pos_index; --i) {
      std::swap(operator[](i - 1), operator[](i));
    }
    return begin() + pos_index;
  } // O(N) strong

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  } // O(N) nothrow(swap)

  iterator erase(const_iterator first, const_iterator last) {
    size_t first_index = first - begin();
    size_t last_index = last - begin();
    size_t diff = last_index - first_index;
    for (size_t i = first_index; i < size() - diff; ++i) {
      std::swap(operator[](i), operator[](i + diff));
    }
    for (size_t i = 0; i < diff; i++) {
      pop_back();
    }
    return begin() + first_index;
  } // O(N) nothrow(swap)

private:
  vector(const vector& other, size_t capacity)
      : _data(capacity ? static_cast<T*>(operator new(capacity * sizeof(T))) : nullptr),
        _size(other.size()),
        _capacity(capacity) {
    try {
      assert(size() <= capacity);
    } catch (...) {
      operator delete(_data);
      throw;
    }
    for (size_t i = 0; i < size(); i++) {
      try {
        new (_data + i) T(other[i]);
      } catch (...) {
        while (i > 0) {
          _data[--i].~T();
        }
        operator delete(_data);
        throw;
      }
    }
  } // O(N) strong

private:
  T* _data;
  size_t _size;
  size_t _capacity;
};
