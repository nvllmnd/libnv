// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/ctypes.h"
#include "nv/memory/alloc.h"
#include "nvxx/alloc.hpp"
#include "nvxx/vmem.hpp"
#include "unity.h"

#include <iostream>

void setUp(void) {}

void tearDown(void) {}

constexpr void fun() {
  int x;
  defer {
    x = 50;
    std::cout << x << "\n";
  };

  auto t = nv::map_memory(50);
  (void)t;
}

void chunk_arena_works() {
  fun();
  nv::Arena ca = {};
  nv::Allocator all = ca.allocator();
  (void)all;
}

// void ptr_and_result_types() {
//   Ptr<const i32> arr{new i32[50]()};
//
//   take(*arr);
//
//   delete[] arr.data;
// }
//
i32 main(void) {
  UNITY_BEGIN();

  // RUN_TEST(opt_works_with_nonnull);
  // RUN_TEST(ptr_and_result_types);

  return UNITY_END();
}
