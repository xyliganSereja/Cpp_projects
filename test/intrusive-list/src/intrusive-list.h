#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace intrusive {

namespace impl {
struct default_tag;
}

template <typename T, typename Tag = impl::default_tag>
class list;

namespace impl {
class list_element_base {
  template <typename T, typename Tag_>
  friend class intrusive::list;

protected:
  list_element_base* prev;
  list_element_base* next;

  list_element_base() noexcept;

  list_element_base(const list_element_base&) noexcept;

  list_element_base& operator=(const list_element_base& other) noexcept;

  list_element_base(list_element_base&& other) noexcept;

  list_element_base& operator=(list_element_base&& other) noexcept;

  bool is_linked() const noexcept;

  void unlink() noexcept;

  ~list_element_base();

  static void link(impl::list_element_base* first, impl::list_element_base* second) noexcept;
};
} // namespace impl

template <class Tag = impl::default_tag>
class list_element : impl::list_element_base {
  template <typename T, typename Tag_>
  friend class list;

public:
  list_element() = default;
};

template <typename T, typename Tag>
class list {
  using element_t = list_element<Tag>;

  static_assert(std::is_base_of_v<element_t, T>, "T must derive from list_element");

  template <class K>
  class list_iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = K;
    using reference = K&;
    using pointer = K*;
    using difference_type = ptrdiff_t;

    friend list_iterator<const K>;
    friend list;

    list_iterator() = default;

  private:
    explicit list_iterator(const impl::list_element_base* el) : element(const_cast<impl::list_element_base*>(el)) {}

  public:
    operator list_iterator<const K>() const noexcept {
      return list_iterator<const K>(element);
    }

    reference operator*() const {
      return *operator->();
    }

    pointer operator->() const {
      return static_cast<K*>(static_cast<element_t*>(element));
    }

    list_iterator operator++(int) noexcept {
      list_iterator old = *this;
      ++(*this);
      return old;
    }

    list_iterator& operator++() noexcept {
      element = element->next;
      return *this;
    }

    list_iterator operator--(int) noexcept {
      list_iterator old = *this;
      --(*this);
      return old;
    }

    list_iterator& operator--() noexcept {
      element = element->prev;
      return *this;
    }

    friend bool operator==(const list_iterator& left, const list_iterator& right) noexcept {
      return left.element == right.element;
    }

    friend bool operator!=(const list_iterator& left, const list_iterator& right) noexcept {
      return !(left == right);
    }

    friend void swap(list_iterator& left, list_iterator& right) noexcept {
      std::swap(left.element, right.element);
    }

  private:
    impl::list_element_base* element;
  };

private:
  impl::list_element_base bound;

public:
  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;

public:
  // O(1)
  list() noexcept = default;

  // O(1)
  ~list() noexcept = default;

  list(const list&) = delete;
  list& operator=(const list&) = delete;

  // O(1)
  list(list&& other) noexcept = default;

  // O(1)
  list& operator=(list&& other) noexcept = default;

  // O(1)
  bool empty() const noexcept {
    return begin() == end();
  }

  // O(n)
  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  // O(1)
  T& front() noexcept {
    assert(!empty());
    return *begin();
  }

  // O(1)
  const T& front() const noexcept {
    assert(!empty());
    return *begin();
  }

  // O(1)
  T& back() noexcept {
    assert(!empty());
    return *last();
  }

  // O(1)
  const T& back() const noexcept {
    assert(!empty());
    return *last();
  }

  // O(1)
  void push_front(T& value) noexcept {
    insert(begin(), value);
  }

  // O(1)
  void push_back(T& value) noexcept {
    insert(end(), value);
  }

  // O(1)
  void pop_front() noexcept {
    erase(begin());
  }

  // O(1)
  void pop_back() noexcept {
    erase(last());
  }

  // O(1)
  void clear() noexcept {
    bound.unlink();
  }

  // O(1)
  iterator begin() noexcept {
    return iterator(bound.next);
  }

  // O(1)
  const_iterator begin() const noexcept {
    return const_iterator(bound.next);
  }

  // O(1)
  iterator end() noexcept {
    return iterator(&bound);
  }

  // O(1)
  const_iterator end() const noexcept {
    return const_iterator(&bound);
  }

  // O(1)
  iterator last() noexcept {
    return --end();
  }

  // O(1)
  const_iterator last() const noexcept {
    return --end();
  }

  // O(1)
  iterator insert(const_iterator pos, T& value) noexcept {
    element_t* val = &value;
    auto* next = get_base_ptr(pos);
    if (val != next) {
      val->unlink();
      impl::list_element_base::link(next->prev, val);
      impl::list_element_base::link(val, next);
    }
    return iterator(val);
  }

  // O(1)
  iterator erase(const_iterator pos) noexcept {
    auto* node = get_base_ptr(pos);
    auto* next = node->next;
    node->unlink();
    return iterator(next);
  }

  // O(1)
  void splice(const_iterator pos, list& /*other*/, const_iterator first, const_iterator last) noexcept {
    if (first == last) {
      return;
    }

    auto* first_n = get_base_ptr(first);
    auto* last_n = get_base_ptr(last);
    auto* back_n = last_n->prev;
    auto* pos_n = get_base_ptr(pos);

    impl::list_element_base::link(first_n->prev, last_n);
    impl::list_element_base::link(pos_n->prev, first_n);
    impl::list_element_base::link(back_n, pos_n);
  }

private:
  static impl::list_element_base* get_base_ptr(const_iterator& it) noexcept {
    return const_cast<impl::list_element_base*>(it.element);
  }
};

} // namespace intrusive
