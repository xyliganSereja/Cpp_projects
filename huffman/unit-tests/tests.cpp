#include "../huffman-lib/bit_sequence.h"
#include "../huffman-lib/util.h"
#include "dataset.h"

#include <gtest/gtest.h>

#include <random>

TEST(bit_seq, empty) {
  huffman::impl::bit_sequence seq;
  EXPECT_EQ(seq.size(), 0);
}

TEST(bit_seq, bool_constructor) {
  huffman::impl::bit_sequence seq_true(true);
  EXPECT_EQ(seq_true.size(), 1);
  EXPECT_EQ(seq_true[0], 1);

  huffman::impl::bit_sequence seq_false(false);
  EXPECT_EQ(seq_false.size(), 1);
  EXPECT_EQ(seq_false[0], 0);
}

TEST(bit_seq, int_constructor) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist;
  unsigned int val = dist(dev);
  huffman::impl::bit_sequence seq(val);
  EXPECT_EQ(seq.size(), UINT_WIDTH);
  for (size_t i = 0; i < UINT_WIDTH; ++i) {
    EXPECT_EQ((val >> (UINT_WIDTH - i - 1)) & 1u, seq[i]);
  }
}

TEST(bit_seq, equality) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist;
  for (size_t iter = 0; iter < 1000; ++iter) {
    auto value = dist(dev);
    auto size = dist(dev) % UINT32_WIDTH;
    huffman::impl::bit_sequence seq(value, size);
    huffman::impl::bit_sequence seq2(value, size);
    EXPECT_TRUE(seq == seq2);
    EXPECT_FALSE(seq != seq2);
    huffman::impl::bit_sequence seq3 = seq;
    EXPECT_TRUE(seq3 == seq2);
    EXPECT_FALSE(seq3 != seq2);
  }
}

TEST(bit_seq, addition_assignment) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist;
  for (size_t iter = 0; iter < 1000; ++iter) {
    uint32_t val = dist(dev);
    huffman::impl::bit_sequence seq;
    for (size_t i = 0; i < UINT_WIDTH; ++i) {
      seq |= static_cast<bool>((val >> (UINT_WIDTH - 1 - i)) & 1u);
    }
    EXPECT_EQ(UINT_WIDTH, seq.size());
    for (size_t i = 0; i < UINT_WIDTH; ++i) {
      EXPECT_EQ((val >> (UINT_WIDTH - 1 - i)) & 1u, seq[i]);
    }
  }
}

TEST(bit_seq, addition_assignment2) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
  for (size_t iter = 0; iter < 1000; ++iter) {
    huffman::impl::bit_sequence seq(dist(dev), dist(dev) % UINT32_WIDTH);
    huffman::impl::bit_sequence seq2(dist(dev), dist(dev) % UINT32_WIDTH);
    auto sum = seq | seq2;
    EXPECT_EQ(sum.size(), seq.size() + seq2.size());
    for (size_t i = 0; i < seq.size(); ++i) {
      EXPECT_EQ(seq[i], sum[i]);
    }
    for (size_t i = 0; i < seq2.size(); ++i) {
      EXPECT_EQ(seq2[i], sum[seq.size() + i]);
    }
  }
}

TEST(bit_seq, extend_emply) {
  huffman::impl::bit_sequence seq;
  EXPECT_EQ(seq.size(), 0);
  seq.extend(0);
  EXPECT_EQ(seq.size(), 0);
}

TEST(bit_seq, extend_emply2) {
  huffman::impl::bit_sequence seq;
  EXPECT_EQ(seq.size(), 0);
  seq.extend(8);
  EXPECT_EQ(seq.size(), 8);
  for (size_t i = 0; i < 8; ++i) {
    EXPECT_EQ(seq[i], 0);
  }
}

TEST(bit_seq, extend) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
  for (size_t iter = 0; iter < 1000; ++iter) {
    huffman::impl::bit_sequence seq(dist(dev), dist(dev) % UINT32_WIDTH);
    auto seq2 = seq;
    size_t new_size = seq.size() + dist(dev) % (10 * UINT32_WIDTH);
    seq2.extend(new_size);
    EXPECT_EQ(seq2.size(), new_size);
    for (size_t i = 0; i < seq.size(); ++i) {
      EXPECT_EQ(seq[i], seq2[i]);
    }
    for (size_t i = seq.size(); i < seq2.size(); ++i) {
      EXPECT_EQ(seq2[i], 0);
    }
  }
}

TEST(bit_seq, inc_empty) {
  huffman::impl::bit_sequence seq;
  EXPECT_EQ(seq.size(), 0);
  EXPECT_EQ((++seq).size(), 0);
  EXPECT_EQ(seq.size(), 0);
}

TEST(bit_seq, inc) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX - 1);
  for (size_t iter = 0; iter < 1000; ++iter) {
    uint32_t value = dist(dev);
    huffman::impl::bit_sequence seq(value);
    auto seq2 = seq;
    for (size_t i = 0; i < seq.size(); ++i) {
      EXPECT_EQ(seq[i], seq2[i]);
    }
    ++seq2;
    seq = huffman::impl::bit_sequence(value + 1);
    for (size_t i = 0; i < seq.size(); ++i) {
      EXPECT_EQ(seq[i], seq2[i]);
    }
  }
}

TEST(bit_seq, cut_to_tail) {
  std::random_device dev;
  std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
  for (size_t iter = 0; iter < 1000; ++iter) {
    auto value = dist(dev);
    auto size = dist(dev) % UINT32_WIDTH;
    huffman::impl::bit_sequence seq(value, size);
    seq.cut_to_tail();
    huffman::impl::bit_sequence seq2(static_cast<uint8_t>(value), size % UINT8_WIDTH);
    EXPECT_EQ(seq.size(), seq2.size());
    for (size_t i = 0; i < seq.size(); ++i) {
      EXPECT_EQ(seq[i], seq2[i]);
    }
  }
}

TEST(utils, histogram_stringstream) {
  std::stringstream ss(dataset::linear_freq());
  huffman::impl::buffered_reader reader(ss);
  auto hist = huffman::impl::calc_histogram(reader);
  for (size_t i = 0; i < dataset::LINEAR_FREQ_CHAR_CNT; ++i) {
    EXPECT_EQ(hist[i], i);
  }
  for (size_t i = dataset::LINEAR_FREQ_CHAR_CNT; i < huffman::impl::ENCODING_VALUE_COUNT; ++i) {
    EXPECT_EQ(hist[i], 0);
  }
}

TEST(utils, encoding_book_equal_frequency) {
  std::stringstream ss(dataset::equal_freq());
  huffman::impl::buffered_reader reader(ss);
  auto hist = huffman::impl::calc_histogram(reader);
  huffman::impl::encoding_book eb = huffman::impl::build_encoding_book(hist);
  for (size_t i = 0; i < dataset::EQUAL_FREQ_CHAR_CNT; ++i) {
    EXPECT_EQ(eb[i].size(), std::bit_width(dataset::EQUAL_FREQ_CHAR_CNT) - 1);
  }
}

TEST(utils, encoding_book_exp_frequency) {
  std::stringstream ss(dataset::exp_freq());
  huffman::impl::buffered_reader reader(ss);
  auto hist = huffman::impl::calc_histogram(reader);
  huffman::impl::encoding_book eb = huffman::impl::build_encoding_book(hist);
  EXPECT_EQ(eb[0].size(), dataset::EXP_FREQ_CHAR_CNT - 1);
  for (size_t i = 1; i < dataset::EXP_FREQ_CHAR_CNT; ++i) {
    EXPECT_EQ(eb[i].size(), dataset::EXP_FREQ_CHAR_CNT - i);
  }
}

TEST(utils, correct_format) {
  for (auto& s : dataset::generate_correct_data_to_decode()) {
    std::stringstream& in = s.first;
    std::stringstream out;
    EXPECT_NO_THROW(huffman::decode(in, out));
    EXPECT_EQ(s.second, out.str());
  }
}

TEST(utils, incorrect_format) {
  for (auto& s : dataset::generate_incorrect_data_to_decode()) {
    std::stringstream& in = s;
    std::stringstream out;
    EXPECT_THROW(huffman::decode(in, out), std::invalid_argument);
  }
}

TEST(utils, encoding_book_decoding_book) {
  for (auto& s : dataset::generate_data_to_encode()) {
    std::stringstream ss(s);
    huffman::impl::buffered_reader reader(ss);
    auto hist = huffman::impl::calc_histogram(reader);
    huffman::impl::encoding_book eb = huffman::impl::build_encoding_book(hist);
    huffman::impl::decoding_book db(eb);
    auto iter = db.iter();
    for (size_t i = 0; i < huffman::impl::ENCODING_VALUE_COUNT; ++i) {
      if (!hist[i]) {
        continue;
      }
      auto& seq = eb[i];
      for (size_t j = 0; j < seq.size(); ++j) {
        auto res = iter(seq[j]);
        if (j != seq.size() - 1) {
          EXPECT_FALSE(res.has_value());
        } else {
          EXPECT_TRUE(res.has_value());
          EXPECT_EQ(res.value(), i);
        }
      }
    }
  }
}

TEST(utils, encoding_decoding) {
  for (auto& s : dataset::generate_data_to_encode()) {
    std::stringstream original(s);
    std::stringstream encoded;
    huffman::encode(original, encoded);
    std::stringstream decoded;
    huffman::decode(encoded, decoded);
    EXPECT_EQ(original.str(), decoded.str());
  }
}

TEST(utils, canonical_huffman) {
  huffman::impl::histogram hist;
  hist.fill(0);
  hist['a'] = 3;
  hist['b'] = 4;
  hist['c'] = 1;
  hist['d'] = 1;
  auto res = huffman::impl::build_encoding_book(hist);
  EXPECT_EQ(res['a'], huffman::impl::bit_sequence(0b10, 2));
  EXPECT_EQ(res['b'], huffman::impl::bit_sequence(0b0, 1));
  EXPECT_EQ(res['c'], huffman::impl::bit_sequence(0b110, 3));
  EXPECT_EQ(res['d'], huffman::impl::bit_sequence(0b111, 3));
}

TEST(utils, incorrect_encoding_book) {
  for (auto& enc : dataset::generate_incorrect_encoding_book()) {
    EXPECT_ANY_THROW(huffman::impl::decoding_book db(enc));
  }
}
