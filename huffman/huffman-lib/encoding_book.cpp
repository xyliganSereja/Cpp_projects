#include "encoding_book.h"

#include "decoding_book.h"

#include <queue>

#include <set>

struct code {
  code(uint8_t len, huffman::impl::encoding_type data) noexcept : len(len), data(data) {}

  uint8_t len;
  huffman::impl::encoding_type data;
};

static bool operator<(const code& left, const code& right) noexcept {
  return left.len < right.len || (left.len == right.len && left.data < right.data);
}

using codes_type = std::set<code>;

static void calc_codes_len_dfs(const std::unique_ptr<huffman::impl::node>& current, codes_type& result,
                               uint8_t len = 0) {
  if (current->is_leaf()) {
    result.emplace(len, static_cast<const huffman::impl::leaf*>(current.get())->data);
  } else {
    auto* parent = static_cast<const huffman::impl::parent*>(current.get());
    calc_codes_len_dfs(parent->child_0, result, len + 1);
    calc_codes_len_dfs(parent->child_1, result, len + 1);
  }
}

static codes_type calc_codes_len(const std::unique_ptr<huffman::impl::node>& root) {
  codes_type result;
  if (root) {
    if (root->is_leaf()) {
      result.emplace(1, static_cast<const huffman::impl::leaf*>(root.get())->data);
    } else {
      calc_codes_len_dfs(root, result);
    }
  }
  return result;
}

struct queue_element {
  queue_element(const size_t& freq, std::unique_ptr<huffman::impl::node>&& node) noexcept
      : freq(freq),
        node(std::move(node)){};

  size_t freq;
  std::unique_ptr<huffman::impl::node> node;
};

static bool operator>(const queue_element& left, const queue_element& right) noexcept {
  return left.freq > right.freq;
}

static std::unique_ptr<huffman::impl::node> build_tree(const huffman::impl::histogram& hist) {
  std::priority_queue<queue_element, std::vector<queue_element>, std::greater<>> priority_queue;
  const huffman::impl::histogram& hist1 = hist;
  for (size_t i = 0; i < hist1.size(); ++i) {
    if (!hist1[i]) {
      continue;
    }
    priority_queue.emplace(hist1[i], std::make_unique<huffman::impl::leaf>(i));
  }
  while (priority_queue.size() > 1) {
    auto first = const_cast<queue_element&&>(priority_queue.top());
    priority_queue.pop();
    auto second = const_cast<queue_element&&>(priority_queue.top());
    priority_queue.pop();
    priority_queue.emplace(first.freq + second.freq, std::make_unique<huffman::impl::parent>(first.node, second.node));
  }
  return priority_queue.empty() ? nullptr : const_cast<queue_element&&>(priority_queue.top()).node;
}

static huffman::impl::encoding_book build_encoding_book(const codes_type& cds) {
  huffman::impl::encoding_book result;
  huffman::impl::bit_sequence code;
  for (auto v : cds) {
    ++code;
    code.extend(v.len);
    result[v.data] = code;
  }
  return result;
}

huffman::impl::encoding_book huffman::impl::build_encoding_book(const huffman::impl::histogram& hist) {
  return build_encoding_book(calc_codes_len(build_tree(hist)));
}

void huffman::impl::serialize(const huffman::impl::encoding_book& enc_book, std::ostream& to) {
  char buf[ENCODING_VALUE_COUNT];
  for (size_t i = 0; i < ENCODING_VALUE_COUNT; ++i) {
    buf[i] = static_cast<char>(enc_book[i].size());
  }
  to.write(buf, ENCODING_VALUE_COUNT);
  to.flush();
  if (!to) {
    throw std::runtime_error("unexpected error while writing encoding book");
  }
}

huffman::impl::encoding_book huffman::impl::deserialize(std::istream& from) {
  codes_type cds;
  char buf[ENCODING_VALUE_COUNT];
  from.read(buf, ENCODING_VALUE_COUNT);
  if (from.gcount() != ENCODING_VALUE_COUNT) {
    if (from.eof()) {
      throw std::invalid_argument("incorrect data format: the data has invalid encoding book");
    } else if (!from) {
      throw std::runtime_error("unexpected error while reading encoding book");
    }
  }
  for (size_t i = 0; i < ENCODING_VALUE_COUNT; ++i) {
    cds.emplace(buf[i], i);
  }
  return build_encoding_book(cds);
}
