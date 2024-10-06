#pragma once

#include "../huffman-lib/encoding_book.h"

#include <string>
#include <vector>

namespace dataset {

const size_t EQUAL_FREQ_CHAR_CNT = 256;
static_assert((EQUAL_FREQ_CHAR_CNT & (EQUAL_FREQ_CHAR_CNT - 1)) == 0, "EQUAL_FREQ_CHAR_CNT must be a power of 2");

std::string equal_freq();

const size_t EXP_FREQ_CHAR_CNT = 15;
std::string exp_freq();

const size_t LINEAR_FREQ_CHAR_CNT = 256;
std::string linear_freq();

std::vector<std::string> generate_data_to_encode();
std::vector<std::stringstream> generate_incorrect_data_to_decode();
std::vector<std::pair<std::stringstream, std::string>> generate_correct_data_to_decode();
std::vector<huffman::impl::encoding_book> generate_incorrect_encoding_book();
} // namespace dataset
