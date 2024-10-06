#include "bit_sequence.h"

#include <cassert>

huffman::impl::bit_sequence::bit_sequence() : _size(0) {}

huffman::impl::bit_sequence::bit_sequence(bool value) : bit_sequence(static_cast<uint64_t>(value), 1) {}

huffman::impl::bit_sequence::bit_sequence(uint64_t value, size_t size) : _size(size) {
  value <<= (std::numeric_limits<uint64_t>::digits - size);
  for (size_t i = 0; i < (size + RAW_SIZE - 1) / RAW_SIZE; ++i) {
    _data.push_back(value >> (std::numeric_limits<uint64_t>::digits - RAW_SIZE));
    value <<= RAW_SIZE;
  }
}

size_t huffman::impl::bit_sequence::size() const noexcept {
  return _size;
}

static bool get_bit(huffman::impl::bit_sequence::raw_type value, size_t i) noexcept {
  return value & (1ull << (huffman::impl::bit_sequence::RAW_SIZE - i - 1));
}

bool huffman::impl::bit_sequence::operator[](size_t i) const {
  assert(i < _size);
  return get_bit(_data[i / RAW_SIZE], i % RAW_SIZE);
}

const std::vector<huffman::impl::bit_sequence::raw_type>& huffman::impl::bit_sequence::raw_data() const noexcept {
  return _data;
}

huffman::impl::bit_sequence& huffman::impl::bit_sequence::operator|=(const huffman::impl::bit_sequence& op) {
  if (!(_size % RAW_SIZE)) {
    _data.emplace_back(0);
  }
  for (auto v : op.raw_data()) {
    _data.back() |= v >> (_size % RAW_SIZE);
    _data.push_back(v << (RAW_SIZE - _size % RAW_SIZE));
  }
  _size += op.size();
  if ((_size + RAW_SIZE - 1) / RAW_SIZE < _data.size()) {
    _data.pop_back();
  }
  return *this;
}

huffman::impl::bit_sequence huffman::impl::operator|(const huffman::impl::bit_sequence& left,
                                                     const huffman::impl::bit_sequence& right) {
  auto tmp = left;
  tmp |= right;
  return tmp;
}

bool huffman::impl::operator!=(const huffman::impl::bit_sequence& left,
                               const huffman::impl::bit_sequence& right) noexcept {
  return !(left == right);
}

bool huffman::impl::operator==(const huffman::impl::bit_sequence& left,
                               const huffman::impl::bit_sequence& right) noexcept {
  return left.raw_data() == right.raw_data() && left.size() == right.size();
}

void huffman::impl::bit_sequence::extend(size_t new_size) {
  assert(new_size >= size());
  _data.resize((new_size + RAW_SIZE - 1) / RAW_SIZE, 0);
  _size = new_size;
}

huffman::impl::bit_sequence& huffman::impl::bit_sequence::operator++() noexcept {
  if (!empty()) {
    auto it = _data.rbegin();
    *it += 1ull << ((RAW_SIZE - size() % RAW_SIZE) % RAW_SIZE);
    while (*it == 0 && ++it != _data.rend()) {
      *it += 1;
    }
  }
  return *this;
}

size_t huffman::impl::bit_sequence::empty() const noexcept {
  return !size();
}

void huffman::impl::bit_sequence::cut_to_tail() noexcept {
  _data.erase(_data.begin(), _data.begin() + size() / RAW_SIZE);
  _size %= RAW_SIZE;
}
