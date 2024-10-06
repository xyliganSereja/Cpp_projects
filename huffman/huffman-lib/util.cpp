#include "util.h"

#include "buffered_reader.h"

#include <array>
#include <cstring>
#include <iostream>

static const size_t SEQ_SIZE = 1024ull * CHAR_BIT;

huffman::impl::histogram huffman::impl::calc_histogram(buffered_reader& reader) {
  histogram res;
  res.fill(0);
  while (reader) {
    ++res[static_cast<unsigned char>(reader.read())];
  }
  if (reader) {
    throw std::runtime_error("unexpected error while reading data: " + std::string(std::strerror(errno)));
  }
  reader.drop();
  return res;
}

static void flush(huffman::impl::bit_sequence& seq, std::ostream& to) {
  if (seq.size()) {
    to.write(reinterpret_cast<const char*>(seq.raw_data().data()), seq.size() / huffman::impl::bit_sequence::RAW_SIZE);
    seq.cut_to_tail();
  }
}

static size_t get_encoded_size(const huffman::impl::histogram& hist, const huffman::impl::encoding_book& enc_book) {
  size_t encoded_size = 0;
  for (size_t i = 0; i < huffman::impl::ENCODING_VALUE_COUNT; ++i) {
    encoded_size += hist[i] * enc_book[i].size();
  }
  return encoded_size;
}

static void encode_data(huffman::impl::buffered_reader& reader, std::ostream& to,
                        const huffman::impl::encoding_book& enc_book) {
  huffman::impl::bit_sequence seq;
  while (reader) {
    seq |= enc_book[static_cast<unsigned char>(reader.read())];
    if (seq.size() >= SEQ_SIZE) {
      flush(seq, to);
    }
  }
  flush(seq, to);
  if (!seq.empty()) {
    to.put(static_cast<char>(seq.raw_data().back()));
  }
}

void huffman::encode(std::istream& from, std::ostream& to) {
  impl::buffered_reader reader(from);
  auto hist = impl::calc_histogram(reader);
  auto enc_book = impl::build_encoding_book(hist);
  serialize(enc_book, to);
  to.put(static_cast<char>((get_encoded_size(hist, enc_book) - 1u) % CHAR_BIT));
  encode_data(reader, to, enc_book);
  if (reader.error()) {
    throw std::runtime_error("unexpected error while reading data");
  }
  if (!to) {
    throw std::runtime_error("unexpected error while writing data");
  }
}

static void decode_data(huffman::impl::buffered_reader& reader, std::ostream& to,
                        huffman::impl::decoding_book& dec_book, unsigned char last_size) {
  auto iter = dec_book.iter();
  while (reader) {
    huffman::impl::bit_sequence seq = reader.read();
    reader.get();
    size_t bound = reader ? seq.size() : last_size;
    for (size_t i = 0; i < bound; ++i) {
      auto res = iter(seq[i]);
      if (res.has_value()) {
        to.put(static_cast<char>(res.value()));
      }
    }
  }
  if (!iter.in_root()) {
    throw std::invalid_argument("incorrect data format: the data has undecodable tail");
  }
  to.flush();
}

void huffman::decode(std::istream& from, std::ostream& to) {
  auto enc_book = impl::deserialize(from);
  impl::decoding_book dec_book(enc_book);
  impl::buffered_reader reader(from);
  if (reader.eof()) {
    throw std::invalid_argument("incorrect data format: "
                                "the data size byte expected, but nothing found");
  }
  unsigned char last_size = (static_cast<unsigned char>(reader.read()) % CHAR_BIT) + 1;
  decode_data(reader, to, dec_book, last_size);
  if (!to) {
    throw std::runtime_error("unexpected error while writing data");
  }
}
