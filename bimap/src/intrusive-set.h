#pragma once

#include <cassert>
#include <iterator>

namespace impl {
template <typename Tag>
class empty_node_base {
public:
  empty_node_base* left = nullptr;
  empty_node_base* right = nullptr;
  empty_node_base* parent = nullptr;

  bool is_leaf() const noexcept {
    return !has_left_child() && !has_right_child();
  }

  bool is_root() const noexcept {
    return parent == nullptr;
  }

  bool is_left_child() const noexcept {
    return !is_root() && (parent->left == this);
  }

  bool is_right_child() const noexcept {
    return !is_root() && (parent->right == this);
  }

  bool has_left_child() const noexcept {
    return left != nullptr;
  }

  bool has_right_child() const noexcept {
    return right != nullptr;
  }

  empty_node_base* left_leaf() noexcept {
    auto* result = this;
    while (result->has_left_child()) {
      result = result->left;
    }
    return result;
  }

  empty_node_base* right_leaf() noexcept {
    auto* result = this;
    while (result->has_right_child()) {
      result = result->right;
    }
    return result;
  }

  void unlink_from_parent() noexcept {
    if (is_left_child()) {
      parent->left = nullptr;
    } else if (is_right_child()) {
      parent->right = nullptr;
    }
    parent = nullptr;
  }

  void link_left(empty_node_base* node) noexcept {
    if (node) {
      node->unlink_from_parent();
      node->parent = this;
    }
    left = node;
  }

  void link_right(empty_node_base* node) noexcept {
    if (node) {
      node->unlink_from_parent();
      node->parent = this;
    }
    right = node;
  }

  void exchange(empty_node_base* from) {
    unlink_from_parent();
    if (from->is_root()) {
      parent = nullptr;
    } else if (from->is_left_child()) {
      from->parent->link_left(this);
    } else {
      from->parent->link_right(this);
    }
    if (from->left) {
      assert(!has_left_child());
      link_left(from->left);
    }
    if (from->right) {
      assert(!has_right_child());
      link_right(from->right);
    }
    from->parent = from->left = from->right = nullptr;
  }

  empty_node_base() noexcept = default;
};

template <typename T, typename Tag>
class node_base : public empty_node_base<Tag> {
public:
  T key;

  explicit node_base(const T& key) : key(key) {}

  explicit node_base(T&& key) noexcept : key(std::move(key)) {}

  node_base(node_base&& other) noexcept = default;
  node_base(const node_base& other) = default;

  node_base& operator=(node_base&& other) noexcept {
    if (this != &other) {
      key = std::move(other.key);
    }
    return *this;
  }

  node_base& operator=(const node_base& other) {
    if (this != &other) {
      key = other.key;
    }
    return *this;
  }
};

template <typename T, typename Tag, typename Compare>
class intrusive_set {
public:
  using node_t = empty_node_base<Tag>;
  using value_node_t = node_base<T, Tag>;

  class iterator {
    friend intrusive_set;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using reference = const T&;
    using pointer = const T*;
    using difference_type = ptrdiff_t;

    iterator() noexcept : node_(nullptr) {}

    explicit iterator(const node_t* node) noexcept : node_(node) {}

    reference operator*() const {
      return *operator->();
    }

    pointer operator->() const {
      return &get_value_node().key;
    }

    const value_node_t& get_value_node() const {
      assert(!is_end());
      return *to_value_node(node_);
    }

    const node_t& get_node() const noexcept {
      return *node_;
    }

    bool is_end() const noexcept {
      return node_->is_root() && !node_->has_right_child();
    }

    iterator& operator++() noexcept {
      if (node_->has_right_child()) {
        node_ = node_->right->left_leaf();
      } else {
        while (node_->is_right_child()) {
          node_ = node_->parent;
        }
        node_ = node_->parent;
      }
      return *this;
    }

    iterator operator++(int) noexcept {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator& operator--() noexcept {
      if (node_->has_left_child()) {
        node_ = node_->left->right_leaf();
      } else {
        while (node_->is_left_child()) {
          node_ = node_->parent;
        }
        node_ = node_->parent;
      }
      return *this;
    }

    iterator operator--(int) noexcept {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    friend bool operator==(const iterator& left, const iterator& right) noexcept {
      return left.node_ == right.node_;
    }

    friend bool operator!=(const iterator& left, const iterator& right) noexcept {
      return !(left == right);
    }

    friend void swap(iterator& left, iterator& right) noexcept {
      std::swap(left.node_, right.node_);
    }

  private:
    const node_t* node_;
  };

  explicit intrusive_set(Compare&& compare, node_t& bound) noexcept : compare(std::move(compare)), bound(&bound) {}

  intrusive_set(const intrusive_set& other) = delete;
  intrusive_set& operator=(const intrusive_set& other) = delete;

  intrusive_set(intrusive_set&& other) noexcept = default;

  intrusive_set& operator=(intrusive_set&& other) noexcept {
    swap(*this, other);
    return *this;
  }

  ~intrusive_set() noexcept = default;

  friend void swap(intrusive_set& lhs, intrusive_set& rhs) noexcept {
    auto* left_root = lhs.bound->left;
    auto* right_root = rhs.bound->left;
    lhs.bound->link_left(right_root);
    rhs.bound->link_left(left_root);
    std::swap(lhs.compare, rhs.compare);
  }

  // lower_bound(node) should be given as it
  // returns iterator to the node
  iterator insert_at(iterator it, value_node_t& node) noexcept {
    auto lwb_node = const_cast<node_t*>(it.node_);
    if (lwb_node->has_left_child()) {
      lwb_node->left->right_leaf()->link_right(&node);
    } else {
      lwb_node->link_left(&node);
    }
    return iterator(&node);
  }

  iterator erase(iterator it) noexcept {
    auto node = const_cast<node_t*>(it.node_);
    if (node->is_leaf()) {
      auto result = node->parent;
      node->unlink_from_parent();
      return iterator(result);
    }
    node_t* candidate;
    if (node->has_right_child()) {
      candidate = node->right->left_leaf();
      if (candidate->has_right_child()) {
        candidate->right->exchange(candidate);
      }
    } else {
      candidate = node->left->right_leaf();
      if (candidate->has_left_child()) {
        candidate->left->exchange(candidate);
      }
    }
    candidate->exchange(node);
    return iterator(candidate);
  }

  iterator find(const T& t) const {
    auto it = lower_bound(t);
    if (it == end() || !is_same_key(it.node_, t)) {
      return end();
    }
    return it;
  }

  iterator lower_bound(const T& t) const {
    auto* current = bound;
    auto* result = current;
    while (current != nullptr) {
      if (current == bound || !compare(to_value_node(current)->key, t)) {
        result = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }
    return iterator(result);
  }

  iterator upper_bound(const T& t) const {
    iterator lower = lower_bound(t);
    if (lower != end() && is_same_key(lower.node_, t)) {
      ++lower;
    }
    return lower;
  }

  iterator begin() const noexcept {
    return iterator(bound->left_leaf());
  }

  iterator end() const noexcept {
    return iterator(bound);
  }

  void set_bound(node_t* bound_) noexcept {
    bound_->link_left(bound->left);
    bound = bound_;
  }

  void clear() noexcept {
    bound->link_left(nullptr);
  }

  const Compare& get_comparator() const noexcept {
    return compare;
  }

  bool is_same_key(const node_t* node, const T& key) const {
    if (node == bound) {
      return false;
    }
    return compare(to_value_node(node)->key, key) == compare(key, to_value_node(node)->key);
  }

private:
  [[no_unique_address]] Compare compare;
  node_t* bound;

  static const value_node_t* to_value_node(const node_t* node_) noexcept {
    return static_cast<const value_node_t*>(node_);
  }

  static const value_node_t* to_value_node(node_t* node_) noexcept {
    return static_cast<value_node_t*>(node_);
  }
};
} // namespace impl
