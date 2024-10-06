#include "big_integer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <vector>

static const big_integer::digit MAX_DIGIT = std::numeric_limits<big_integer::digit>::max();
static const uint64_t BASE = static_cast<uint64_t>(MAX_DIGIT) + 1;
static const size_t DIGIT_LEN = std::numeric_limits<big_integer::digit>::digits;

static const uint32_t POW10[]{1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

static const size_t DEC_DIGIT_LEN = std::numeric_limits<big_integer::digit>::digits10;
static const big_integer::digit DEC_BASE = POW10[DEC_DIGIT_LEN];

big_integer::big_integer() noexcept = default;

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(unsigned long long a, bool sign) : sign(sign) {
  data.reserve(UINT64_WIDTH / DIGIT_LEN);
  while (a != 0) {
    data.push_back(a & MAX_DIGIT);
    a >>= DIGIT_LEN;
  }
}

big_integer::big_integer(const std::string& str) {
  bool _sign = str.starts_with("-");
  size_t begin = str.starts_with("+") || str.starts_with("-") ? 1 : 0;
  if (begin == str.size()) {
    throw std::invalid_argument("number expected at position" + std::to_string(begin));
  }
  for (; begin < str.size(); begin += DEC_DIGIT_LEN) {
    std::string val = str.substr(begin, DEC_DIGIT_LEN);
    if (val.starts_with("+") || val.starts_with("-") || isspace(val[0])) {
      throw std::invalid_argument("no signs or spaces expected at position " + std::to_string(begin));
    }
    multiply_digit(POW10[val.size()]);
    size_t pos = 0;
    add_digit(std::stoi(val, &pos));
    if (pos != val.size()) {
      throw std::invalid_argument("unknown symbols found at position " + std::to_string(begin + pos));
    }
  }
  shrink();
  sign = _sign;
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(const big_integer& other) {
  if (this != &other) {
    big_integer copy(other);
    swap(*this, copy);
  }
  return *this;
}

bool unsigned_less(const big_integer& a, const big_integer& b) {
  if (a.data.size() != b.data.size()) {
    return a.data.size() < b.data.size();
  }
  return std::lexicographical_compare(a.data.rbegin(), a.data.rend(), b.data.rbegin(), b.data.rend());
}

void add_iterative(big_integer::digit& lhs, big_integer::digit rhs, bool& carry) {
  uint64_t new_val = lhs;
  new_val += rhs;
  new_val += carry;
  carry = new_val >> DIGIT_LEN;
  lhs = new_val & MAX_DIGIT;
}

void big_integer::add_unsigned(const big_integer& rhs) {
  resize(std::max(data.size(), rhs.data.size()) + 1);
  bool carry = false;
  for (size_t i = 0; i < data.size() && (i < rhs.data.size() || carry); ++i) {
    add_iterative(data[i], rhs.get(i), carry);
  }
  shrink();
}

void big_integer::add_digit(digit rhs) {
  if (data.empty()) {
    data.push_back(rhs);
    sign = false;
  } else {
    resize(data.size() + 1);
    bool carry = false;
    add_iterative(data.front(), rhs, carry);
    for (size_t i = 1; i < data.size() && carry; ++i) {
      add_iterative(data[i], 0, carry);
    }
    shrink();
  }
}

void subtract_iterative(big_integer::digit& lhs, big_integer::digit rhs, bool& borrow) {
  big_integer::digit new_borrow = lhs < rhs;
  lhs -= rhs;
  new_borrow |= lhs < borrow;
  lhs -= borrow;
  borrow = new_borrow;
}

void big_integer::subtract_unsigned(const big_integer& rhs) {
  difference(rhs, 0, data.size() - 1);
}

void big_integer::subtract_digit(digit rhs) {
  if (data.empty()) {
    data.push_back(rhs);
    sign = true;
  } else {
    bool borrow = false;
    subtract_iterative(data.front(), rhs, borrow);
    for (size_t i = 1; i < data.size() && borrow; ++i) {
      subtract_iterative(data[i], 0, borrow);
    }
    if (borrow) {
      true_negate(data.size());
      sign ^= true;
    }
    shrink();
  }
}

big_integer::digit big_integer::get(size_t index) const {
  return index < data.size() ? data[index] : 0;
}

void big_integer::difference(const big_integer& rhs, size_t k, size_t m) {
  if (data.size() < rhs.data.size()) {
    data.resize(rhs.data.size(), 0);
  }
  bool borrow = false;
  for (size_t i = 0; i <= m && i + k < data.size(); ++i) {
    subtract_iterative(data[i + k], rhs.get(i), borrow);
  }
  if (borrow) {
    true_negate(data.size());
    sign ^= true;
  }
  shrink();
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  if (sign == rhs.sign) {
    add_unsigned(rhs);
  } else {
    subtract_unsigned(rhs);
  }
  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  negate();
  *this += rhs;
  negate();
  return *this;
}

void multiply_iteratively(big_integer::digit& result, big_integer::digit lhs, big_integer::digit rhs,
                          big_integer::digit& carry) {
  uint64_t new_v = lhs;
  new_v *= rhs;
  new_v += result;
  new_v += carry;
  result = new_v & MAX_DIGIT;
  carry = new_v >> DIGIT_LEN;
}

void big_integer::multiply_digit(digit rhs) {
  resize(data.size() + 1);
  digit carry = 0;
  for (auto& v : data) {
    digit v2 = v;
    v = 0;
    multiply_iteratively(v, v2, rhs, carry);
  }
  shrink();
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  size_t old_size = data.size();
  resize(data.size() + rhs.data.size());
  for (size_t l = old_size; l--;) {
    digit d = data[l];
    data[l] = 0;
    digit carry = 0;
    for (size_t r = 0; r < rhs.data.size() || carry; ++r) {
      multiply_iteratively(data.at(l + r), rhs.get(r), d, carry);
    }
  }
  sign ^= rhs.sign;
  shrink();
  return *this;
}

big_integer::digit big_integer::to_digit() const {
  return get(0);
}

big_integer::digit big_integer::div_digit(digit rhs) {
  assert(rhs != 0);
  uint64_t reminder = 0;
  for (auto it = data.rbegin(); it != data.rend(); ++it) {
    reminder <<= DIGIT_LEN;
    reminder += *it;
    *it = (reminder / rhs);
    reminder %= rhs;
  }
  shrink();
  return reminder;
}

big_integer::digit trial(const big_integer& r, const big_integer& d, size_t k, size_t m) {
  size_t km = k + m;
  big_integer r3;
  r3.data = {r.get(km - 2), r.get(km - 1), r.get(km)};
  r3.shrink();
  big_integer d2;
  d2.data = {d.get(m - 2), d.get(m - 1)};
  d2.shrink();

  big_integer::digit left = 0;
  big_integer::digit right = MAX_DIGIT;
  while (right != left) {
    big_integer::digit mid = (right - left) / 2 + (right - left) % 2 + left;
    big_integer prod = d2;
    prod.multiply_digit(mid);
    if (unsigned_less(r3, prod)) {
      right = mid - 1;
    } else {
      left = mid;
    }
  }
  return left;
}

std::pair<big_integer, big_integer> unsigned_div(const big_integer& x, const big_integer& y, size_t m) {
  big_integer::digit f = BASE / (static_cast<uint64_t>(y.get(m - 1)) + 1);
  big_integer r = x;
  r.multiply_digit(f);
  big_integer d = y;
  d.multiply_digit(f);
  big_integer q = 0;
  q.data.reserve(r.data.size() - m + 1);
  for (size_t k = r.data.size() - m + 1; k > 0;) {
    --k;
    big_integer::digit qt = trial(r, d, k, m);
    big_integer dq = d;
    dq.multiply_digit(qt);
    if (m + 1 <= dq.data.size() &&
        std::lexicographical_compare(r.data.crbegin(), r.data.crbegin() + m + 1, dq.data.crbegin(), dq.data.crend())) {
      --qt;
      dq -= d;
    }
    q.data.push_back(qt);
    r.difference(dq, k, m);
  }
  std::reverse(q.data.begin(), q.data.end());
  q.shrink();
  r.div_digit(f);
  return {q, r};
}

std::pair<big_integer, big_integer> div(const big_integer& x, const big_integer& y) {
  assert(!y.is_zero());
  size_t m = y.data.size();
  std::pair<big_integer, big_integer> res;
  if (m == 1) {
    big_integer::digit y1 = y.to_digit();
    big_integer division = x;
    big_integer reminder = division.div_digit(y1);
    res = {division, reminder};
  } else {
    if (m > x.data.size()) {
      res = {0, x};
    } else {
      res = unsigned_div(x, y, m);
    }
  }
  res.first.sign = x.sign ^ y.sign;
  res.second.sign = x.sign;
  return res;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  auto div_mod_res = div(*this, rhs);
  swap(*this, div_mod_res.first);
  return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  auto div_mod_res = div(*this, rhs);
  swap(*this, div_mod_res.second);
  return *this;
}

big_integer::digit true_signed(big_integer::digit data, bool sign, bool& carry) {
  if (sign) {
    data = ~data;
  }
  data += carry;
  carry = !data && carry;
  return data;
}

template <class BinaryOperation>
big_integer& big_integer::binary(const big_integer& rhs, const BinaryOperation& op) {
  size_t len = std::max(data.size(), rhs.data.size());
  resize(len);
  bool res_sign = op(sign, rhs.sign);
  bool carry = sign, rcarry = rhs.sign, res_carry = res_sign;
  for (size_t i = 0; i < len; i++) {
    data[i] = true_signed(op(true_signed(get(i), sign, carry), true_signed(rhs.get(i), rhs.sign, rcarry)), res_sign,
                          res_carry);
  }
  sign = res_sign;
  shrink();
  return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  return binary(rhs, std::bit_and<>());
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  return binary(rhs, std::bit_or<>());
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  return binary(rhs, std::bit_xor<>());
}

big_integer& big_integer::operator<<=(int rhs) {
  assert(rhs >= 0);
  data.reserve(data.size() + (rhs + DIGIT_LEN - 1) / DIGIT_LEN);
  data.insert(data.begin(), rhs / DIGIT_LEN, 0);
  rhs %= DIGIT_LEN;
  if (rhs) {
    digit rest = 0;
    for (auto& val : data) {
      digit new_rest = val >> (DIGIT_LEN - rhs);
      val = (val << rhs) | rest;
      rest = new_rest;
    }
    if (rest) {
      data.push_back(rest);
    }
  }
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  assert(rhs >= 0);
  bool round = false;
  auto del_end = data.begin() + std::min(data.size(), static_cast<size_t>(rhs / DIGIT_LEN));
  for (auto it2 = data.begin(); it2 != del_end && !round; ++it2) {
    round |= *it2;
  }
  data.erase(data.begin(), del_end);
  rhs %= DIGIT_LEN;
  if (rhs) {
    digit rest = 0;
    for (auto it = data.rbegin(); it != data.rend(); it++) {
      digit new_rest = *it << (DIGIT_LEN - rhs);
      *it = (*it >> rhs) | rest;
      rest = new_rest;
    }
    round |= rest;
  }
  if (this->is_negative() && round) {
    --(*this);
  }
  shrink();
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  big_integer tmp = *this;
  tmp.negate();
  return tmp;
}

big_integer big_integer::operator~() const {
  big_integer tmp = *this;
  ++tmp;
  tmp.negate();
  return tmp;
}

big_integer& big_integer::operator++() {
  if (sign) {
    subtract_digit(1);
  } else {
    add_digit(1);
  }
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer tmp = *this;
  ++(*this);
  return tmp;
}

big_integer& big_integer::operator--() {
  negate();
  ++(*this);
  negate();
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer tmp = *this;
  --(*this);
  return tmp;
}

big_integer operator+(const big_integer& a, const big_integer& b) {
  return big_integer(a) += b;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
  return big_integer(a) -= b;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
  return big_integer(a) *= b;
}

big_integer operator/(const big_integer& a, const big_integer& b) {
  return big_integer(a) /= b;
}

big_integer operator%(const big_integer& a, const big_integer& b) {
  return big_integer(a) %= b;
}

big_integer operator&(const big_integer& a, const big_integer& b) {
  return big_integer(a) &= b;
}

big_integer operator|(const big_integer& a, const big_integer& b) {
  return big_integer(a) |= b;
}

big_integer operator^(const big_integer& a, const big_integer& b) {
  return big_integer(a) ^= b;
}

big_integer operator<<(const big_integer& a, int b) {
  return big_integer(a) <<= b;
}

big_integer operator>>(const big_integer& a, int b) {
  return big_integer(a) >>= b;
}

bool operator==(const big_integer& a, const big_integer& b) {
  return (a.is_zero() && b.is_zero()) || (a.sign == b.sign && a.data == b.data);
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  if (a.is_zero() && b.is_zero()) {
    return false;
  }
  if (a.sign != b.sign) {
    return a.sign && !b.sign;
  }
  return a.sign ^ unsigned_less(a, b);
}

bool operator>(const big_integer& a, const big_integer& b) {
  return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}

std::string to_string(big_integer a) {
  std::vector<std::string> resInverted;
  std::string s = (a.is_negative() ? "-" : "");
  while (!a.is_zero()) {
    resInverted.push_back(std::to_string(a.div_digit(DEC_BASE)));
  }
  if (resInverted.empty()) {
    s += "0";
  } else {
    s.reserve(resInverted.size() * DEC_DIGIT_LEN);
    s += resInverted.back();
    for (auto it = resInverted.rbegin() + 1; it != resInverted.rend(); ++it) {
      s += std::string(DEC_DIGIT_LEN - it->size(), '0') + *it;
    }
  }
  return s;
}

bool big_integer::is_zero() const {
  return data.empty();
}

bool big_integer::is_positive() const {
  return !is_zero() && !sign;
}

bool big_integer::is_negative() const {
  return !is_zero() && sign;
}

void big_integer::shrink() {
  while (!data.empty() && !data.back()) {
    data.pop_back();
  }
}

void swap(big_integer& a, big_integer& b) {
  std::swap(a.data, b.data);
  std::swap(a.sign, b.sign);
}

big_integer& big_integer::negate() {
  sign = !sign;
  return *this;
}

big_integer& big_integer::resize(size_t len) {
  data.resize(len, 0);
  return *this;
}

big_integer& big_integer::true_negate(size_t len) {
  resize(len);
  for (auto& v : data) {
    v = ~v;
  }
  add_digit(1);
  return *this;
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
  return out << to_string(a);
}
