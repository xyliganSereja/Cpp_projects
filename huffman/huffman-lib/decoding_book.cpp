#include "decoding_book.h"

#include "encoding_book.h"

#include <queue>

std::optional<huffman::impl::encoding_type> huffman::impl::decoding_book::iterator::operator()(bool value) {
  if (!root) {
    throw std::invalid_argument("incorrect data format: the data has empty encoding book, but it is not empty itself");
  }
  if (root->is_leaf()) {
    if (value) {
      throw std::invalid_argument("incorrect data format: the encoding book contains a single character. "
                                  "only 0 are expected in encoded data");
    }
    return static_cast<const leaf*>(root)->data;
  }
  auto current_p = static_cast<const parent*>(current);
  current = value ? current_p->child_1.get() : current_p->child_0.get();
  if (current->is_leaf()) {
    auto res = std::make_optional<>(static_cast<const leaf*>(current)->data);
    current = root;
    return res;
  }
  return std::nullopt;
}

static std::unique_ptr<huffman::impl::node>& make_path(std::unique_ptr<huffman::impl::node>& root,
                                                       const huffman::impl::bit_sequence& seq) {
  if (!root) {
    root = std::make_unique<huffman::impl::parent>(nullptr, nullptr);
  }
  auto* current = &root;
  for (size_t j = 0; j < seq.size(); ++j) {
    if (current->get()->is_leaf()) {
      throw std::invalid_argument("incorrect data format: the data has invalid encoding book");
    }
    auto& child = seq[j] ? static_cast<huffman::impl::parent*>(current->get())->child_1
                         : static_cast<huffman::impl::parent*>(current->get())->child_0;
    if (!child) {
      child = std::make_unique<huffman::impl::parent>(nullptr, nullptr);
    }
    current = &child;
  }
  return *current;
}

static void expect_correct(const std::unique_ptr<huffman::impl::node>& root) {
  if (root && !root->is_leaf()) {
    auto root_p = static_cast<const huffman::impl::parent*>(root.get());
    if (!root_p->child_0 || !root_p->child_1) {
      throw std::invalid_argument("incorrect data format: the data has invalid encoding book");
    }
    expect_correct(root_p->child_0);
    expect_correct(root_p->child_1);
  }
}

static void handle_single_node(std::unique_ptr<huffman::impl::node>& root) noexcept {
  if (root && !root->is_leaf()) {
    auto* parent = static_cast<huffman::impl::parent*>(root.get());
    if (parent->child_0 == nullptr ^ parent->child_1 == nullptr) {
      if (parent->child_0 && parent->child_0->is_leaf()) {
        root = std::move(parent->child_0);
      } else if (parent->child_1 && parent->child_1->is_leaf()) {
        root = std::move(parent->child_1);
      }
    }
  }
}

huffman::impl::decoding_book::decoding_book(const impl::encoding_book& enc_book) : decoding_book() {
  for (size_t i = 0; i < ENCODING_VALUE_COUNT; ++i) {
    const auto& seq = enc_book[i];
    if (seq.empty()) {
      continue;
    }
    auto& leaf_node = make_path(root, seq);
    if (leaf_node->is_leaf()) {
      throw std::invalid_argument("incorrect data format: the data has invalid encoding book");
    } else {
      auto* parent_node = static_cast<const parent*>(leaf_node.get());
      if (parent_node->child_0 || parent_node->child_1) {
        throw std::invalid_argument("incorrect data format: the data has invalid encoding book");
      }
    }
    leaf_node = std::make_unique<leaf>(i);
  }
  handle_single_node(root);
  expect_correct(root);
}

huffman::impl::decoding_book::iterator huffman::impl::decoding_book::iter() const noexcept {
  return {root.get()};
}

bool huffman::impl::decoding_book::iterator::in_root() const noexcept {
  return current == root;
}

huffman::impl::leaf::leaf(huffman::impl::encoding_type data) noexcept : data(data) {}

huffman::impl::parent::parent(std::unique_ptr<huffman::impl::node>&& child_0,
                              std::unique_ptr<huffman::impl::node>&& child_1) noexcept
    : child_0(std::move(child_0)),
      child_1(std::move(child_1)) {}

huffman::impl::parent::parent(std::unique_ptr<node>& child_0, std::unique_ptr<node>& child_1) noexcept
    : parent(std::move(child_0), std::move(child_1)) {}
