#pragma once

#include "bit_sequence.h"
#include "constants.h"

#include <ostream>
#include <set>

namespace huffman::impl {

using encoding_book = std::array<bit_sequence, ENCODING_VALUE_COUNT>;

encoding_book build_encoding_book(const histogram& hist);

void serialize(const encoding_book& enc_book, std::ostream& to);

encoding_book deserialize(std::istream& from);
} // namespace huffman::impl
