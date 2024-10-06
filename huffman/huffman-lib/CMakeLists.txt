cmake_minimum_required(VERSION 3.21)
project(huffman-lib)

set(CMAKE_CXX_STANDARD 20)

add_library(huffman-lib STATIC
        bit_sequence.cpp
        bit_sequence.h
        util.cpp
        util.h
        decoding_book.cpp
        decoding_book.h
        constants.h
        encoding_book.cpp
        encoding_book.h buffered_reader.h)
