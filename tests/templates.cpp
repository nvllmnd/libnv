// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/intdefs.h"
#include "nvxx/mod.hpp"
#include "unity.h"

using namespace nv;
using namespace nv::ptr;

void setUp(void) {}

void tearDown(void) {}

void opt_works_with_nonnull() {
  i32 x = 50;
  const auto nn = nonnull(&x).value();
  TEST_ASSERT_TRUE(nn.is_not_null());
  *nn = 100;

  TEST_ASSERT_EQUAL(100, *nn);

  const i32* p = &x;
  const NonNull<const i32> np = nonnull(*p);
  TEST_ASSERT_TRUE(np.is_not_null());
}

void take(const i32&) {}

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

  RUN_TEST(opt_works_with_nonnull);
  // RUN_TEST(ptr_and_result_types);

  return UNITY_END();
}
