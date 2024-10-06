#pragma once

#include "bit_sequence.h"

#include <array>
#include <climits>

namespace huffman::impl {
using encoding_type = uint8_t;
const size_t ENCODING_VALUE_COUNT = 1 << (sizeof(encoding_type) * CHAR_BIT);
using histogram = std::array<uint64_t, ENCODING_VALUE_COUNT>;
} // namespace huffman::impl
