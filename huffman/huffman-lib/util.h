#pragma once

#include "buffered_reader.h"
#include "constants.h"
#include "decoding_book.h"

#include <istream>

namespace huffman {
namespace impl {
histogram calc_histogram(huffman::impl::buffered_reader& reader);
}

void encode(std::istream& from, std::ostream& to);

void decode(std::istream& from, std::ostream& to);

} // namespace huffman
