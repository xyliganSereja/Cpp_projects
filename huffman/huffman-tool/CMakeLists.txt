cmake_minimum_required(VERSION 3.21)
project(huffman-tool)

set(CMAKE_CXX_STANDARD 20)

add_executable(huffman-tool tool.cpp)

target_link_libraries(huffman-tool PUBLIC huffman-lib)
