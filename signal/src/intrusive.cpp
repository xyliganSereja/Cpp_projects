#include "intrusive.h"

#include <algorithm>

signals::impl::intrusive::intrusive() noexcept : prev(this), next(this) {}

signals::impl::intrusive::intrusive(intrusive* list) noexcept : intrusive() {
  insert(list);
}

signals::impl::intrusive::~intrusive() {
  remove();
}

void signals::impl::intrusive::remove() noexcept {
  prev->next = next;
  next->prev = prev;
  prev = next = this;
}

void signals::impl::intrusive::insert(intrusive* list) noexcept {
  remove();
  prev = list;
  next = list->next;
  prev->next = this;
  next->prev = this;
}
