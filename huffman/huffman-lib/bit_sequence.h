#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <set>
#include <vector>

namespace huffman::impl {
class bit_sequence {
public:
  using raw_type = uint8_t;
  static const size_t RAW_SIZE = std::numeric_limits<raw_type>::digits;

private:
  std::vector<raw_type> _data;
  size_t _size;

public:
  template <class T>
  requires std::is_integral_v<T>
  bit_sequence(T value) : bit_sequence(static_cast<uint64_t>(value), sizeof(T) * CHAR_BIT) {}

  bit_sequence(bool value);
  bit_sequence(uint64_t value, size_t size);
  bit_sequence();

  template <class It>
  requires std::forward_iterator<It>
  bit_sequence(It begin, It end) : _data(begin, end),
                                   _size(_data.size() * RAW_SIZE) {}

  size_t size() const noexcept;

  size_t empty() const noexcept;

  bool operator[](size_t i) const;

  const std::vector<raw_type>& raw_data() const noexcept;

  void extend(size_t new_size);

  void cut_to_tail() noexcept;

  bit_sequence& operator++() noexcept;

  bit_sequence& operator|=(const bit_sequence& op);

  friend bit_sequence operator|(const bit_sequence& left, const bit_sequence& right);
  friend bool operator==(const bit_sequence& left, const bit_sequence& right) noexcept;
  friend bool operator!=(const bit_sequence& left, const bit_sequence& right) noexcept;
};

bit_sequence operator|(const bit_sequence& left, const bit_sequence& right);
bool operator==(const bit_sequence& left, const bit_sequence& right) noexcept;
bool operator!=(const bit_sequence& left, const bit_sequence& right) noexcept;
} // namespace huffman::impl
