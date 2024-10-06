#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>

template <class T>
class matrix {
private:
  template <class K>
  class col_iter {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using reference = K&;
    using pointer = K*;

    friend col_iter<const K>;
    friend matrix<T>;

    col_iter() = default;

  private:
    col_iter(K* element, size_t col, size_t col_count) : element(element), col(col), col_count(col_count) {}

  public:
    template <class K2>
    requires(std::is_const_v<K> || !std::is_const_v<K2>)
    col_iter(const col_iter<K2>& other) : col_iter(other.element, other.col, other.col_count) {}

    template <class K2>
    requires(std::is_const_v<K> || !std::is_const_v<K2>)
    col_iter& operator=(const col_iter<K2>& other) {
      col_iter copy = other;
      swap(copy, *this);
      return (*this);
    }

    ~col_iter() = default;

    reference operator*() const {
      return *(element + col);
    }

    pointer operator->() const {
      return &(operator*());
    }

    col_iter operator++(int) {
      col_iter old = *this;
      ++(*this);
      return old;
    }

    col_iter& operator++() {
      return (*this) += 1;
    }

    col_iter operator--(int) {
      col_iter old = *this;
      --(*this);
      return old;
    }

    col_iter& operator--() {
      return (*this) -= 1;
    }

    friend col_iter operator+(const col_iter& left, difference_type right) {
      col_iter copy(left);
      return copy += right;
    }

    friend col_iter operator+(difference_type left, const col_iter& right) {
      col_iter copy(right);
      return copy + left;
    }

    friend col_iter operator-(const col_iter& left, difference_type right) {
      col_iter copy(left);
      return copy -= right;
    }

    friend difference_type operator-(const col_iter& left, const col_iter& right) {
      assert(left.col_count == right.col_count);
      return static_cast<ptrdiff_t>(left.element - right.element) / static_cast<ptrdiff_t>(left.col_count);
    }

    reference operator[](difference_type n) const {
      return *(*this + n);
    }

    friend bool operator==(const col_iter& left, const col_iter& right) {
      return (left - right) == 0;
    }

    friend bool operator!=(const col_iter& left, const col_iter& right) {
      return !(left == right);
    }

    col_iter& operator+=(difference_type n) {
      element += static_cast<difference_type>(n * col_count);
      return (*this);
    }

    col_iter& operator-=(difference_type n) {
      return (*this += -n);
    }

    friend bool operator<(const col_iter& left, const col_iter& right) {
      return (left - right) < 0;
    }

    friend bool operator>(const col_iter& left, const col_iter& right) {
      return right < left;
    }

    friend bool operator>=(const col_iter& left, const col_iter& right) {
      return !(left < right);
    }

    friend bool operator<=(const col_iter& left, const col_iter& right) {
      return !(left > right);
    }

    friend void swap(col_iter& left, col_iter& right) {
      std::swap(left.element, right.element);
      std::swap(left.col, right.col);
      std::swap(left.col_count, right.col_count);
    }

  private:
    K* element;
    size_t col;
    size_t col_count;
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

  using row_iterator = pointer;
  using const_row_iterator = const_pointer;

  using col_iterator = col_iter<T>;
  using const_col_iterator = col_iter<const T>;

public:
  matrix() : _data(nullptr), _rows(0), _cols(0) {}

  matrix(size_t rows, size_t cols)
      : _data((rows * cols) != 0 ? new value_type[rows * cols]{} : nullptr),
        _rows(cols != 0 ? rows : 0),
        _cols(rows != 0 ? cols : 0) {}

  template <size_t Rows, size_t Cols>
  matrix(const T (&init)[Rows][Cols]) : matrix(Rows, Cols) {
    for (size_t row = 0; row < rows(); row++) {
      std::copy_n(init[row], cols(), row_begin(row));
    }
  }

  matrix(const matrix& other) : matrix(other.rows(), other.cols()) {
    std::copy(other.begin(), other.end(), begin());
  }

  matrix(matrix&& other) : _data(other.data()), _rows(other.rows()), _cols(other.cols()) {}

  matrix& operator=(const matrix& other) {
    if (other != *this) {
      matrix copy = other;
      swap(copy, *this);
    }
    return (*this);
  }

  matrix& operator=(matrix&& other) {
    swap(other, *this);
    return (*this);
  }

  ~matrix() {
    delete[] _data;
  }

  friend void swap(matrix& left, matrix& right) {
    std::swap(left._data, right._data);
    std::swap(left._rows, right._rows);
    std::swap(left._cols, right._cols);
  }

  // Iterators

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  iterator end() {
    return data() + size();
  }

  const_iterator end() const {
    return data() + size();
  }

  row_iterator row_begin(size_t row) {
    return data() + row * cols();
  }

  const_row_iterator row_begin(size_t row) const {
    return data() + row * cols();
  }

  row_iterator row_end(size_t row) {
    return row_begin(row) + cols();
  }

  const_row_iterator row_end(size_t row) const {
    return row_begin(row) + cols();
  }

  col_iterator col_begin(size_t col) {
    return col_iterator(data(), col, cols());
  }

  const_col_iterator col_begin(size_t col) const {
    return const_col_iterator(data(), col, cols());
  }

  col_iterator col_end(size_t col) {
    return col_iterator(data(), col, cols()) + rows();
  }

  const_col_iterator col_end(size_t col) const {
    return const_col_iterator(data(), col, cols()) + rows();
  }

  // Size

  size_t rows() const {
    return _rows;
  }

  size_t cols() const {
    return _cols;
  }

  size_t size() const {
    return rows() * cols();
  }

  bool empty() const {
    return !size();
  }

  // Elements access

  reference operator()(size_t row, size_t col) {
    return data()[row * cols() + col];
  }

  const_reference operator()(size_t row, size_t col) const {
    return data()[row * cols() + col];
  }

  pointer data() {
    return _data;
  }

  const_pointer data() const {
    return _data;
  }

  // Comparison

  friend bool operator==(const matrix& left, const matrix& right) {
    return left.rows() == right.rows() && left.cols() == right.cols() &&
           std::equal(left.begin(), left.end(), right.begin());
  }

  friend bool operator!=(const matrix& left, const matrix& right) {
    return !(left == right);
  }

  // Arithmetic operations

  matrix& operator+=(const matrix& other) {
    assert(cols() == other.cols() && rows() == other.rows());
    for (size_t index = 0; index < size(); index++) {
      data()[index] += other.data()[index];
    }
    return (*this);
  }

  matrix& operator-=(const matrix& other) {
    assert(cols() == other.cols() && rows() == other.rows());
    for (size_t index = 0; index < size(); index++) {
      data()[index] -= other.data()[index];
    }
    return (*this);
  }

  matrix& operator*=(const matrix& other) {
    matrix res = *this * other;
    swap(res, *this);
    return (*this);
  }

  matrix& operator*=(const_reference factor) {
    for (size_t index = 0; index < size(); index++) {
      data()[index] *= factor;
    }
    return (*this);
  }

  friend matrix operator+(const matrix& left, const matrix& right) {
    return matrix(left) += right;
  }

  friend matrix operator-(const matrix& left, const matrix& right) {
    return matrix(left) -= right;
  }

  friend matrix operator*(const matrix& left, const matrix& right) {
    assert(left.cols() == right.rows());
    matrix res(left.rows(), right.cols());
    for (size_t i = 0; i < left.rows(); i++) {
      for (size_t j = 0; j < right.cols(); j++) {
        for (size_t k = 0; k < left.cols(); k++) {
          res(i, j) += left(i, k) * right(k, j);
        }
      }
    }
    return res;
  }

  friend matrix operator*(const matrix& left, const_reference right) {
    return matrix(left) *= right;
  }

  friend matrix operator*(const_reference left, const matrix& right) {
    return matrix(right) *= left;
  }

private:
  pointer _data;
  size_t _rows;
  size_t _cols;
};
