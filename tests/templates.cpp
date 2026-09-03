// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nvxx/common.hpp"
#include "nvxx/alloc.hpp"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void chunk_arena_works() {
  nv::BufferAlloc ca = {};
  nv::Allocator all = ca.allocator();
  (void)all;
}

void static_string_append() {
  static constexpr nv::StaticString<4> x = "left";
  static constexpr nv::StaticString<1> z = "|";
  static constexpr nv::StaticString<5> y = "right";
  static constexpr nv::StaticString<10> xzy = (x + z) + y;
  static_assert(xzy == "left|right");

  TEST_ASSERT_EQUAL_STRING_LEN("left|right", xzy.str().data(), xzy.str().length());
}

i32 main(void) {
  UNITY_BEGIN();
  RUN_TEST(static_string_append);
  // RUN_TEST(ptr_and_result_types);

  return UNITY_END();
}
