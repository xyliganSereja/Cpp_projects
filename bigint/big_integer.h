#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

struct big_integer {
public:
  using digit = uint32_t;

private:
  std::vector<digit> data;
  bool sign = false; // sign == (this < 0)
                     //-0 is possible and equals to +0

public:
  big_integer() noexcept;
  big_integer(const big_integer& other);

  template <class T>
  requires std::is_integral_v<T>
  big_integer(T a) : big_integer(a < 0 ? -static_cast<unsigned long long>(a) : a, a < 0) {}

  big_integer(unsigned long long a, bool sign = false);
  explicit big_integer(const std::string& str);
  ~big_integer();

  big_integer& operator=(const big_integer& other);

  big_integer& operator+=(const big_integer& rhs);
  big_integer& operator-=(const big_integer& rhs);
  big_integer& operator*=(const big_integer& rhs);
  big_integer& operator/=(const big_integer& rhs);
  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator&=(const big_integer& rhs);
  big_integer& operator|=(const big_integer& rhs);
  big_integer& operator^=(const big_integer& rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  friend bool operator==(const big_integer& a, const big_integer& b);
  friend bool operator!=(const big_integer& a, const big_integer& b);
  friend bool operator<(const big_integer& a, const big_integer& b);
  friend bool operator>(const big_integer& a, const big_integer& b);
  friend bool operator<=(const big_integer& a, const big_integer& b);
  friend bool operator>=(const big_integer& a, const big_integer& b);

  friend std::string to_string(big_integer a);

private:
  friend void swap(big_integer& a, big_integer& b);
  friend bool unsigned_less(const big_integer& a, const big_integer& b);
  friend big_integer::digit trial(const big_integer& r, const big_integer& d, size_t k, size_t m);
  friend std::pair<big_integer, big_integer> unsigned_div(const big_integer& x, const big_integer& y, size_t m);
  friend std::pair<big_integer, big_integer> div(const big_integer& x, const big_integer& y);
  void shrink();
  big_integer& negate();
  big_integer& true_negate(size_t len);
  bool is_zero() const;
  bool is_negative() const;
  bool is_positive() const;
  void add_unsigned(const big_integer& rhs);
  void add_digit(digit rhs);
  void subtract_unsigned(const big_integer& rhs);
  void subtract_digit(digit rhs);
  void difference(const big_integer& rhs, size_t k, size_t m);
  void multiply_digit(digit rhs);
  big_integer& resize(size_t len);
  template <class BinaryOperation>
  big_integer& binary(const big_integer& rhs, const BinaryOperation& op);
  template <class T>
  void parse(T t);
  digit to_digit() const;
  digit get(size_t index) const;
  // returns reminder
  digit div_digit(digit rhs);
};

big_integer operator+(const big_integer& a, const big_integer& b);
big_integer operator-(const big_integer& a, const big_integer& b);
big_integer operator*(const big_integer& a, const big_integer& b);
big_integer operator/(const big_integer& a, const big_integer& b);
big_integer operator%(const big_integer& a, const big_integer& b);

big_integer operator&(const big_integer& a, const big_integer& b);
big_integer operator|(const big_integer& a, const big_integer& b);
big_integer operator^(const big_integer& a, const big_integer& b);

big_integer operator<<(const big_integer& a, int b);
big_integer operator>>(const big_integer& a, int b);

bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(big_integer a);
std::ostream& operator<<(std::ostream& out, const big_integer& a);

void swap(big_integer& a, big_integer& b);
