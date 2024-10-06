#include "socow-vector.h"
#include "test-utils.h"

#include <gtest/gtest.h>

using std::as_const;

class custom : public base_test {};

TEST_F(custom, erase_range_reallocation) {
  container a;
  for (size_t i = 0; i < 10; ++i) {
    a.push_back(i + 100);
  }
  immutable_guard g(a);

  container b = a;
  element::reset_counters();

  b.erase(std::as_const(b).begin() + 2, std::as_const(b).begin() + 7);
  EXPECT_EQ(5, element::get_copy_counter());
  EXPECT_EQ(0, element::get_swap_counter());
}

TEST_F(custom, erase_big_to_small) {
  container a;
  for (size_t i = 0; i < 6; ++i) {
    a.push_back(i + 100);
  }
  immutable_guard g(a);

  container b = a;
  element::reset_counters();

  b.erase(std::as_const(b).begin(), std::as_const(b).begin() + 3);
  EXPECT_EQ(3, element::get_copy_counter());
  EXPECT_EQ(0, element::get_swap_counter());
  expect_static_storage(b);
}

TEST_F(custom, erase_big_to_small_throw) {
  container a;
  for (size_t i = 0; i < 6; ++i) {
    a.push_back(i + 100);
  }

  container b = a;
  immutable_guard g(a, b);
  element::reset_counters();
  element::set_copy_throw_countdown(2);
  EXPECT_THROW(b.erase(std::as_const(b).begin(), std::as_const(b).begin() + 3), std::runtime_error);
}

TEST_F(custom, erase_big_reallocation_throw) {
  container a;
  for (size_t i = 0; i < 10; ++i) {
    a.push_back(i + 100);
  }

  container b = a;
  immutable_guard g(a, b);
  element::reset_counters();
  element::set_copy_throw_countdown(2);
  EXPECT_THROW(b.erase(std::as_const(b).begin(), std::as_const(b).begin() + 4), std::runtime_error);
  element::set_copy_throw_countdown(4);
  EXPECT_THROW(b.erase(std::as_const(b).begin() + 2, std::as_const(b).begin() + 4), std::runtime_error);
}

TEST_F(custom, insert_reallocation) {
  container a;
  for (size_t i = 0; i < 10; ++i) {
    a.push_back(i + 100);
  }
  immutable_guard g(a);

  container b = a;
  element::reset_counters();

  b.insert(std::as_const(b).begin(), 1);
  EXPECT_EQ(11, element::get_copy_counter());
  EXPECT_EQ(0, element::get_swap_counter());
}