#pragma once

#include "constants.h"
#include "encoding_book.h"

#include <iostream>
#include <memory>
#include <optional>
#include <variant>

namespace huffman::impl {
class node {
public:
  virtual bool is_leaf() const noexcept = 0;
  virtual ~node() = default;
};

class parent : public node {
public:
  parent(std::unique_ptr<node>&& child_0, std::unique_ptr<node>&& child_1) noexcept;
  parent(std::unique_ptr<node>& child_0, std::unique_ptr<node>& child_1) noexcept;

public:
  std::unique_ptr<node> child_0 = nullptr;
  std::unique_ptr<node> child_1 = nullptr;

  bool is_leaf() const noexcept override {
    return false;
  };
};

class leaf : public node {
public:
  explicit leaf(encoding_type data) noexcept;

public:
  encoding_type data = 0;

  bool is_leaf() const noexcept override {
    return true;
  };
};

class decoding_book {
private:
  std::unique_ptr<node> root = nullptr;

  class iterator {
  private:
    const node* root;
    const node* current;

    iterator(const node* root) : root(root), current(root) {}

    friend decoding_book;

  public:
    std::optional<encoding_type> operator()(bool value);

    bool in_root() const noexcept;
  };

public:
  decoding_book() noexcept = default;
  explicit decoding_book(const encoding_book& enc_book);

  iterator iter() const noexcept;
};
} // namespace huffman::impl
