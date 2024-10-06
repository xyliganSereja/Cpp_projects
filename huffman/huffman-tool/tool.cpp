
#include "../huffman-lib/util.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>

enum class exit_code {
  SUCCESS = 0,
  INVALID_ARGUMENT = 1,
  ERROR_OPENING_FILE = 2,
  INTERNAL_ERROR = 3
};

int int_code(exit_code code) {
  return static_cast<int>(code);
}

const std::string COMPRESS_FLAG = "--compress";
const std::string DECOMPRESS_FLAG = "--decompress";
const std::string HELP_FLAG = "--help";
const std::string INPUT_FLAG = "--input";
const std::string OUTPUT_FLAG = "--output";

void print_help() {
  const size_t column_width = 30;
  const std::string IF_PLACEHOLDER = "<src>";
  const std::string OF_PLACEHOLDER = "<dest>";
  std::cout << "huffman-tool " << INPUT_FLAG << " " << IF_PLACEHOLDER << " " << OUTPUT_FLAG << " " << OF_PLACEHOLDER
            << " " << COMPRESS_FLAG << "\n"
            << "huffman-tool " << INPUT_FLAG << " " << IF_PLACEHOLDER << " " << OUTPUT_FLAG << " " << OF_PLACEHOLDER
            << " " << DECOMPRESS_FLAG << "\n"
            << "huffman-tool " << HELP_FLAG << "\n"
            << "\n"
            << std::left << std::setw(column_width) << COMPRESS_FLAG << "Compress " << IF_PLACEHOLDER
            << " and write to " << OF_PLACEHOLDER << "\n"
            << std::setw(column_width) << DECOMPRESS_FLAG << "Decompress " << IF_PLACEHOLDER << " and write to "
            << OF_PLACEHOLDER << "\n"
            << std::setw(column_width) << HELP_FLAG << "Display this information\n"
            << std::setw(column_width) << INPUT_FLAG + " " + IF_PLACEHOLDER << "Path to the input file\n"
            << std::setw(column_width) << OUTPUT_FLAG + " " + OF_PLACEHOLDER << "Path to the output file\n"
            << std::setw(column_width) << ""
            << "Result of writing to the input file is undefined\n";
}

exit_code process_file(const std::string& from, const std::string& to, bool compress) {
  std::ifstream in(from, std::ios::in | std::ios::binary);
  if (!in) {
    std::cerr << "error opening input file \"" << from << "\"\n";
    return exit_code::ERROR_OPENING_FILE;
  }
  std::ofstream out(to, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out) {
    std::cerr << "error opening output file \"" << to << "\"\n";
    return exit_code::ERROR_OPENING_FILE;
  }
  try {
    if (compress) {
      huffman::encode(in, out);
    } else {
      huffman::decode(in, out);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return exit_code::INTERNAL_ERROR;
  } catch (...) {
    std::cerr << "unknown error: " << typeid(std::current_exception()).name() << "\n";
    return exit_code::INTERNAL_ERROR;
  }
  return exit_code::SUCCESS;
}

exit_code handle_file_name_expected(const std::string& flag) {
  std::cerr << "file name expected after " << flag << " flag\n\n";
  print_help();
  return exit_code::INVALID_ARGUMENT;
}

int main(int argc, char** argv) {
  std::string from;
  std::string to;
  bool compress = false;
  bool decompress = false;
  bool help = false;
  for (size_t i = 1; i < argc; ++i) {
    auto arg = std::string(argv[i]);
    if (arg == COMPRESS_FLAG) {
      compress = true;
    } else if (arg == DECOMPRESS_FLAG) {
      decompress = true;
    } else if (arg == HELP_FLAG) {
      help = true;
    } else if (arg == INPUT_FLAG) {
      if (i + 1 == argc) {
        return int_code(handle_file_name_expected(arg));
      }
      from = argv[++i];
    } else if (arg == OUTPUT_FLAG) {
      if (i + 1 == argc) {
        return int_code(handle_file_name_expected(arg));
      }
      to = argv[++i];
    } else {
      std::cerr << "unknown flag " << arg << " found\n\n";
      print_help();
      return int_code(exit_code::INVALID_ARGUMENT);
    }
  }
  if (help) {
    print_help();
    return int_code(exit_code::SUCCESS);
  }
  if (from.empty()) {
    std::cerr << "path to input file should be given\n\n";
    print_help();
    return int_code(exit_code::INVALID_ARGUMENT);
  }
  if (to.empty()) {
    std::cerr << "path to output file should be given\n";
    print_help();
    return int_code(exit_code::INVALID_ARGUMENT);
  }
  if (!compress && !decompress) {
    std::cerr << COMPRESS_FLAG << " or " << DECOMPRESS_FLAG << " flag should be given\n\n";
    print_help();
    return int_code(exit_code::INVALID_ARGUMENT);
  } else if (compress && decompress) {
    std::cerr << COMPRESS_FLAG << " and " << DECOMPRESS_FLAG << " found. Exactly one flag should be given\n\n";
    print_help();
    return int_code(exit_code::INVALID_ARGUMENT);
  }
  return int_code(process_file(from, to, compress));
}
