#pragma once

#include "matrix.h"

#include <gtest/gtest.h>

struct element {
  void* operator new[](size_t count) {
    allocations += count / sizeof(element);
    return ::operator new[](count);
  }

  element() : value(0) {}

  element(size_t value) : value(value) {}

  friend element operator+(const element& left, const element& right) {
    return {left.value + right.value};
  }

  friend element operator-(const element& left, const element& right) {
    return {left.value - right.value};
  }

  friend element operator*(const element& left, const element& right) {
    return {left.value * right.value};
  }

  element& operator+=(const element& other) {
    value += other.value;
    return *this;
  }

  element& operator-=(const element& other) {
    value -= other.value;
    return *this;
  }

  element& operator*=(const element& other) {
    value *= other.value;
    return *this;
  }

  friend bool operator==(const element&, const element&) = default;

public:
  inline static size_t allocations;

  static void reset_allocations() {
    allocations = 0;
  }

public:
  size_t value;
};

inline bool operator==(matrix<element>::const_iterator it, matrix<element>::const_col_iterator col_it) {
  return it == col_it.operator->();
}

inline void expect_allocations(size_t expected_allocations) {
  EXPECT_GE(expected_allocations, element::allocations);
}

inline size_t elem(size_t i, size_t j) {
  return i * 101 + j * 7;
}

template <class T>
void fill(matrix<T>& a) {
  for (size_t i = 0; i < a.rows(); ++i) {
    for (size_t j = 0; j < a.cols(); ++j) {
      a(i, j) = elem(i, j);
    }
  }
}

template <class T>
void expect_empty(const matrix<T>& m) {
  EXPECT_EQ(0, m.rows());
  EXPECT_EQ(0, m.cols());
  EXPECT_EQ(0, m.size());
  EXPECT_TRUE(m.empty());
  EXPECT_EQ(nullptr, m.data());
}

template <class T>
void expect_equal(const matrix<T>& expected, const matrix<T>& actual) {
  EXPECT_EQ(expected.rows(), actual.rows());
  EXPECT_EQ(expected.cols(), actual.cols());
  EXPECT_EQ(expected.size(), actual.size());
  EXPECT_EQ(expected.empty(), actual.empty());
  EXPECT_EQ(expected.data() == nullptr, actual.data() == nullptr);

  for (size_t i = 0; i < expected.rows(); ++i) {
    for (size_t j = 0; j < expected.cols(); ++j) {
      EXPECT_EQ(expected(i, j), actual(i, j)) << "  where i = " << i << ", j = " << j;
    }
  }
}
