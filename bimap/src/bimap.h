#pragma once

#include "intrusive-set.h"

#include <cassert>
#include <stdexcept>
#include <utility>

template <typename, typename, typename, typename>
class bimap;

namespace impl {
template <typename LeftTag, typename RightTag>
class empty_node : public empty_node_base<LeftTag>, public empty_node_base<RightTag> {
  using left_base_t = empty_node_base<LeftTag>;
  using right_base_t = empty_node_base<RightTag>;

  template <typename, typename, typename, typename>
  friend class ::bimap;

public:
  empty_node() noexcept = default;
};

template <typename Left, typename Right, typename LeftTag, typename RightTag>
class node : public node_base<Left, LeftTag>, public node_base<Right, RightTag> {
  using left_base_t = node_base<Left, LeftTag>;
  using right_base_t = node_base<Right, RightTag>;

  template <typename, typename, typename, typename>
  friend class ::bimap;

public:
  node(const Left& left, const Right& right) : left_base_t(left), right_base_t(right) {}

  node(const Left& left, Right&& right) : left_base_t(left), right_base_t(std::move(right)) {}

  node(Left&& left, const RightTag& right) : left_base_t(std::move(left)), right_base_t(right) {}

  node(Left&& left, Right&& right) noexcept : left_base_t(std::move(left)), right_base_t(std::move(right)) {}
};
} // namespace impl

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
class bimap {
  struct left_tag;
  struct right_tag;
  using left_map_t = impl::intrusive_set<Left, left_tag, CompareLeft>;
  using right_map_t = impl::intrusive_set<Right, right_tag, CompareRight>;
  using empty_node_t = impl::empty_node<left_tag, right_tag>;
  using value_node_t = impl::node<Left, Right, left_tag, right_tag>;
  using left_t = Left;
  using right_t = Right;

  template <class base_it, class pair_base_it>
  class iterator {
    friend bimap;

  public:
    using iterator_category = base_it::iterator_category;
    using value_type = base_it::value_type;
    using reference = base_it::reference;
    using pointer = base_it::pointer;
    using difference_type = base_it::difference_type;

    iterator() noexcept = default;

    reference operator*() const {
      return it.operator*();
    }

    pointer operator->() const {
      return it.operator->();
    }

    iterator& operator++() noexcept {
      ++it;
      return *this;
    }

    iterator operator++(int) noexcept {
      return it++;
    }

    iterator& operator--() noexcept {
      --it;
      return *this;
    }

    iterator operator--(int) noexcept {
      return it--;
    }

    iterator<pair_base_it, base_it> flip() const noexcept {
      if (it.is_end()) {
        return pair_base_it(&get_empty_node());
      }
      return pair_base_it(&get_value_node());
    }

    friend bool operator==(const iterator& left, const iterator& right) noexcept {
      return left.it == right.it;
    }

    friend bool operator!=(const iterator& left, const iterator& right) noexcept {
      return !(left == right);
    }

    friend void swap(iterator& left, iterator& right) noexcept {
      swap(left.it, right.it);
    }

  private:
    base_it it;

    iterator(const base_it& base) noexcept : it(base) {}

    operator base_it() noexcept {
      return it;
    }

    const value_node_t& get_value_node() const {
      return static_cast<const value_node_t&>(it.get_value_node());
    }

    value_node_t& get_value_node() {
      return const_cast<value_node_t&>(static_cast<const value_node_t&>(it.get_value_node()));
    }

    const empty_node_t& get_empty_node() const {
      assert(it.is_end());
      return static_cast<const empty_node_t&>(it.get_node());
    }

    empty_node_t& get_empty_node() {
      assert(it.is_end());
      return const_cast<empty_node_t&>(static_cast<const empty_node_t&>(it.get_node()));
    }
  };

public:
  using left_iterator = iterator<typename left_map_t::iterator, typename right_map_t::iterator>;
  using right_iterator = iterator<typename right_map_t::iterator, typename left_map_t::iterator>;

  explicit bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight()) noexcept
      : left_map(std::move(compare_left), bound),
        right_map(std::move(compare_right), bound) {}

  bimap(const bimap& other)
      : bimap(CompareLeft(other.left_map.get_comparator()), CompareRight(other.right_map.get_comparator())) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept
      : left_map(std::move(other.left_map)),
        right_map(std::move(other.right_map)),
        size_(std::exchange(other.size_, 0)) {
    left_map.set_bound(&bound);
    right_map.set_bound(&bound);
    other.left_map.clear();
    other.right_map.clear();
  }

  bimap& operator=(const bimap& other) {
    if (this != &other) {
      bimap tmp(other);
      swap(tmp, *this);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    swap(other, *this);
    return *this;
  }

  ~bimap() {
    clear();
  }

  left_iterator insert(const left_t& left, const right_t& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(const left_t& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, const right_t& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) noexcept {
    --size_;
    auto node = &it.get_value_node();
    right_map.erase(it.flip());
    auto res = left_map.erase(it);
    delete node;
    return res;
  }

  bool erase_left(const left_t& left) {
    left_iterator it = find_left(left);
    if (it != end_left()) {
      erase_left(it);
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) noexcept {
    --size_;
    auto node = &it.get_value_node();
    left_map.erase(it.flip());
    auto res = right_map.erase(it);
    delete node;
    return res;
  }

  bool erase_right(const right_t& right) {
    right_iterator it = find_right(right);
    if (it != end_right()) {
      erase_right(it);
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) noexcept {
    while (first != last) {
      first = erase_left(first);
    }
    return first;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) noexcept {
    while (first != last) {
      first = erase_right(first);
    }
    return first;
  }

  left_iterator find_left(const left_t& left) const {
    return left_map.find(left);
  }

  right_iterator find_right(const right_t& right) const {
    return right_map.find(right);
  }

  const right_t& at_left(const left_t& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("no key found in at_left(key)");
    }
    return *it.flip();
  }

  const left_t& at_right(const right_t& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("no key found in at_left(key)");
    }
    return *it.flip();
  }

  const right_t& at_left_or_default(const left_t& key)
    requires std::is_default_constructible_v<Right>
  {
    auto lit = find_left(key);
    if (lit != end_left()) {
      return *lit.flip();
    } else {
      auto right_default = right_t();
      erase_right(right_default);
      return *insert(key, std::move(right_default)).flip();
    }
  }

  const left_t& at_right_or_default(const right_t& key)
    requires std::is_default_constructible_v<Left>
  {
    auto rit = find_right(key);
    if (rit != end_right()) {
      return *rit.flip();
    } else {
      auto left_default = left_t();
      erase_left(left_default);
      return *insert(std::move(left_default), key);
    }
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left_map.lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_map.upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_map.lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_map.upper_bound(right);
  }

  left_iterator begin_left() const noexcept {
    return left_map.begin();
  }

  left_iterator end_left() const noexcept {
    return left_map.end();
  }

  right_iterator begin_right() const noexcept {
    return right_map.begin();
  }

  right_iterator end_right() const noexcept {
    return right_map.end();
  }

  bool empty() const noexcept {
    return !size();
  }

  std::size_t size() const noexcept {
    return size_;
  }

  void clear() noexcept {
    erase_left(begin_left(), end_left());
  }

  friend bool operator==(const bimap& lhs, const bimap& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (auto lit = lhs.begin_left(), rit = rhs.begin_left(); lit != lhs.end_left(); ++lit, ++rit) {
      bool flag =
          !lhs.left_map.get_comparator()(*lit, *rit) && !lhs.right_map.get_comparator()(*lit.flip(), *rit.flip());
      if (!flag) {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(const bimap& lhs, const bimap& rhs) {
    return !(lhs == rhs);
  }

  friend void swap(bimap& lhs, bimap& rhs) noexcept {
    swap(lhs.left_map, rhs.left_map);
    swap(lhs.right_map, rhs.right_map);
    std::swap(lhs.size_, rhs.size_);
  }

private:
  empty_node_t bound;
  left_map_t left_map;
  right_map_t right_map;
  size_t size_ = 0;

  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) {
    auto left_it = left_map.lower_bound(left);
    if (left_map.is_same_key(&left_it.get_node(), left)) {
      return end_left();
    }
    auto right_it = right_map.lower_bound(right);
    if (right_map.is_same_key(&right_it.get_node(), right)) {
      return end_left();
    }
    auto* node = new value_node_t(std::forward<L>(left), std::forward<R>(right));
    ++size_;
    right_map.insert_at(right_it, *node);
    return left_map.insert_at(left_it, *node);
  }
};
