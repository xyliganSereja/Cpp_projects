#pragma once

#include <array>
#include <istream>

namespace huffman::impl {
class buffered_reader {
private:
  static const size_t BUF_SIZE = 1024ull;
  std::istream& stream;
  std::array<char, BUF_SIZE> buffer;
  size_t current = 0;
  size_t readed = 0;

  void next_buffer() {
    stream.read(buffer.begin(), BUF_SIZE);
    readed = stream.gcount();
    current = 0;
  }

public:
  buffered_reader(std::istream& stream) noexcept : stream(stream) {}

  char get() {
    if (current == readed) {
      next_buffer();
    }
    return buffer[current];
  }

  char read() {
    char tmp = get();
    ++current;
    return tmp;
  }

  operator bool() noexcept {
    return !error() && !eof();
  }

  bool eof() {
    return current >= readed && (next_buffer(), current >= readed);
  }

  bool error() const noexcept {
    return !stream && !stream.eof();
  }

  void drop() {
    stream.clear();
    stream.seekg(0);
    readed = 0;
    current = 0;
  }
};
} // namespace huffman::impl
