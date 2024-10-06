#pragma once

#include "intrusive.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using slot = std::function<void(Args...)>;

  friend struct connection;

  struct connection : protected impl::intrusive {
    template <typename>
    friend struct signal;

  private:
    slot slot_;
    signal* signal_ = nullptr;

    connection(signal* signal_, slot slot_, intrusive* list)
        : intrusive(list),
          slot_(std::move(slot_)),
          signal_(signal_) {}

  public:
    connection() = default;

    connection(const connection& that) = delete;
    connection& operator=(const connection& thanot) = delete;

    connection(connection&& that) noexcept : connection() {
      *this = std::move(that);
    }

    connection& operator=(connection&& that) noexcept {
      if (this != &that) {
        disconnect();
        insert(that.prev);
        signal_ = that.signal_;
        slot_ = std::move(that.slot_);
        that.disconnect();
      }
      return *this;
    }

    void disconnect() noexcept {
      if (signal_) {
        for (auto it = signal_->iterators.next; it != &signal_->iterators; it = it->next) {
          auto& it2 = static_cast<iter*>(it)->it;
          if (it2 == this) {
            it2 = next;
          }
        }
        signal_ = nullptr;
      }
      remove();
      slot_ = slot();
    }

    ~connection() {
      disconnect();
    }
  };

private:
  struct iter : impl::intrusive {
    iter(intrusive* it, intrusive* list) noexcept : intrusive(list), it(it) {}

    intrusive* it;
  };

  impl::intrusive slots;
  mutable impl::intrusive iterators;

public:
  signal() = default;

  signal(const signal&) = delete;
  signal& operator=(const signal&) = delete;

  ~signal() {
    for (auto it = slots.next; it != &slots;) {
      static_cast<connection*>(std::exchange(it, it->next))->disconnect();
    }
  }

  connection connect(slot slot_) noexcept {
    return connection(this, std::move(slot_), slots.prev);
  }

  void operator()(Args... args) const {
    iter it(slots.next, iterators.prev);
    while (it.it != &slots) {
      auto it2 = it.it;
      static_cast<const connection*>(it.it)->slot_(std::forward<Args>(args)...);
      if (it.it == it2) {
        it.it = it.it->next;
      }
    }
  }
};
} // namespace signals
