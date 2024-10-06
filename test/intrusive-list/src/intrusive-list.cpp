#include "intrusive-list.h"

intrusive::impl::list_element_base::list_element_base() noexcept : prev(this), next(this) {}

intrusive::impl::list_element_base::list_element_base(const intrusive::impl::list_element_base&) noexcept
    : list_element_base() {}

intrusive::impl::list_element_base& intrusive::impl::list_element_base::operator=(
    const intrusive::impl::list_element_base& other) noexcept {
  if (&other != this) {
    unlink();
  }
  return *this;
}

intrusive::impl::list_element_base::list_element_base(intrusive::impl::list_element_base&& other) noexcept
    : list_element_base() {
  *this = std::move(other);
}

intrusive::impl::list_element_base& intrusive::impl::list_element_base::operator=(
    intrusive::impl::list_element_base&& other) noexcept {
  if (&other != this) {
    unlink();

    if (other.is_linked()) {
      prev = std::exchange(other.prev, &other);
      next = std::exchange(other.next, &other);
      prev->next = next->prev = this;
    }
  }

  return *this;
}

bool intrusive::impl::list_element_base::is_linked() const noexcept {
  assert((prev == this) == (next == this));
  return prev != this;
}

void intrusive::impl::list_element_base::unlink() noexcept {
  prev->next = next;
  next->prev = prev;
  prev = next = this;
}

intrusive::impl::list_element_base::~list_element_base() {
  unlink();
}

void intrusive::impl::list_element_base::link(intrusive::impl::list_element_base* first,
                                              intrusive::impl::list_element_base* second) noexcept {
  first->next = second;
  second->prev = first;
}
