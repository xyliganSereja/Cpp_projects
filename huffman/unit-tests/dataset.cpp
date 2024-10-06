#include "dataset.h"

#include <sstream>
#include <string>
#include <vector>

std::string dataset::equal_freq() {
  std::string res;
  res.reserve(EQUAL_FREQ_CHAR_CNT * 10);
  for (size_t i = 0; i < EQUAL_FREQ_CHAR_CNT; ++i) {
    for (size_t j = 0; j < 10; ++j) {
      res += static_cast<char>(i);
    }
  }
  return res;
}

std::string dataset::exp_freq() {
  std::string res;
  res.reserve((1 << EXP_FREQ_CHAR_CNT) - 1);
  for (size_t i = 0; i < EXP_FREQ_CHAR_CNT; ++i) {
    for (size_t j = 0; j < 1 << i; ++j) {
      res += static_cast<char>(i);
    }
  }
  return res;
}

std::string dataset::linear_freq() {
  std::string res;
  res.reserve(LINEAR_FREQ_CHAR_CNT * (LINEAR_FREQ_CHAR_CNT + 1) / 2);
  for (size_t i = 0; i < LINEAR_FREQ_CHAR_CNT; ++i) {
    for (size_t j = 0; j < i; ++j) {
      res += static_cast<char>(i);
    }
  }
  return res;
}

std::vector<std::string> dataset::generate_data_to_encode() {
  std::vector<std::string> dataset;
  dataset.push_back("");
  dataset.push_back("a");
  dataset.push_back(equal_freq());
  dataset.push_back(linear_freq());
  dataset.push_back(exp_freq());
  return dataset;
}

template <class T>
static void add(std::ostream& to, T t, size_t times) {
  while (times--) {
    to << t;
  }
}

std::vector<std::stringstream> dataset::generate_incorrect_data_to_decode() {
  std::vector<std::stringstream> dataset;
  dataset.emplace_back("");
  {
    std::stringstream in;
    add(in, '\x0', 250);
    dataset.emplace_back(std::move(in));
  }
  {
    std::stringstream in;
    add(in, '\x0', 256);
    dataset.emplace_back(std::move(in));
  }
  {
    std::stringstream in;
    add(in, '\x0', 256);
    in << '\x0' << 'A';
    dataset.emplace_back(std::move(in));
  }
  {
    std::stringstream in;
    add(in, '\x0', 255);
    in << '\x1';
    in << '\x0' << '\x80';
    dataset.emplace_back(std::move(in));
  }
  {
    std::stringstream in;
    add(in, '\x8', 256);
    in << '\x3' << 'A';
    dataset.emplace_back(std::move(in));
  }
  {
    std::stringstream in;
    add(in, '\x1', 256);
    in << '\x7' << 'A';
    dataset.emplace_back(std::move(in));
  }
  return dataset;
}

std::vector<std::pair<std::stringstream, std::string>> dataset::generate_correct_data_to_decode() {
  std::vector<std::pair<std::stringstream, std::string>> dataset;
  {
    std::stringstream in;
    add(in, '\x0', 256);
    in << '\xff';
    dataset.emplace_back(std::move(in), "");
  }
  {
    std::stringstream in;
    add(in, '\x0', 255);
    in << '\x1';
    in << '\x0' << '\x0';
    dataset.emplace_back(std::move(in), "\xff");
  }
  {
    std::stringstream in;
    add(in, '\x0', 255);
    in << '\x1';
    in << '\x8' << '\x0';
    dataset.emplace_back(std::move(in), "\xff");
  }
  {
    std::stringstream in;
    add(in, '\x0', 255);
    in << '\x1';
    in << '\x7' << '\x0';
    dataset.emplace_back(std::move(in), "\xff\xff\xff\xff\xff\xff\xff\xff");
  }
  {
    std::stringstream in;
    add(in, '\x8', 256);
    in << '\x0';
    dataset.emplace_back(std::move(in), "");
  }
  {
    std::stringstream in;
    add(in, '\x8', 256);
    in << '\x7' << 'A';
    dataset.emplace_back(std::move(in), "A");
  }
  {
    std::stringstream in;
    add(in, '\x8', 256);
    in << '\xff' << 'A';
    dataset.emplace_back(std::move(in), "A");
  }
  return dataset;
}

std::vector<huffman::impl::encoding_book> dataset::generate_incorrect_encoding_book() {
  std::vector<huffman::impl::encoding_book> result;
  {
    huffman::impl::encoding_book enc;
    enc[0] = huffman::impl::bit_sequence(0b0, 1);
    enc[1] = huffman::impl::bit_sequence(0b0, 1);
    result.push_back(enc);
  }
  {
    huffman::impl::encoding_book enc;
    enc[0] = huffman::impl::bit_sequence(0b0, 1);
    enc[1] = huffman::impl::bit_sequence(0b01, 2);
    result.push_back(enc);
  }
  {
    huffman::impl::encoding_book enc;
    enc[0] = huffman::impl::bit_sequence(0b01, 2);
    enc[1] = huffman::impl::bit_sequence(0b0, 1);
    result.push_back(enc);
  }
  {
    huffman::impl::encoding_book enc;
    enc[0] = huffman::impl::bit_sequence(0b001, 3);
    result.push_back(enc);
  }
  {
    huffman::impl::encoding_book enc;
    enc[0] = huffman::impl::bit_sequence(0b0, 1);
    enc[1] = huffman::impl::bit_sequence(0b10, 2);
    result.push_back(enc);
  }
  return result;
}
