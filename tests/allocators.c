// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>--license=GPL-3.0-or-later
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/vmem.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

struct Stuff {
  char buf[4096];

  struct Point {
    f32 x;
    f32 y;
  } points[20];

  i64 counter;
};
alias(Stuff);

void vmem_alloc_and_cleanup(void) {
  VArena vm = va_new_ex(GB(2), false);

  TEST_ASSERT_TRUE(va_isok(&vm));

  for (i32 i = 0; i < 50; i++) {
    char* b1 = va_zallocate(&vm, mlayout_bytes(1024));
    char* b2 = va_zallocate(&vm, mlayout_bytes(2048));

    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);

    strncpy(b1, "ayooo", sizeof("ayooo"));

    TEST_ASSERT_EQUAL_STRING(b1, "ayooo");
  }

  va_destroy(&vm);
}

void varena_lock_and_commit(void) {
  VMem* vm = vmem_new(GB(2));
  TEST_ASSERT_NOT_NULL(vm);

  NvError err = vmem_ram_lock(vm, vmem_begin(vm), KB(12));
  TEST_ASSERT_EQUAL(Error__Ok, err);

  err = vmem_ram_release(vm, vmem_begin(vm), KB(12));
  TEST_ASSERT_EQUAL(Error__Ok, err);

  err = vmem_prefault_range(vm, vmem_begin(vm),  KB(24));
  TEST_ASSERT_EQUAL(Error__Ok, err);

  vmem_destroy(vm);
}
  
void varena_marker_and_reset(void) {
  VArena arena = va_new(GB(2));

  TEST_ASSERT_TRUE(va_isok(&arena));

  const VMark marker = va_checkpoint(&arena);

  TEST_ASSERT(marker >= 0);

  static constexpr const i64 INTCOUNT = 25;

  for (i32 i = 0; i < INTCOUNT; i++) {
    i32* x = va_allocate(&arena, mlayout_new(i32));
    TEST_ASSERT_NOT_NULL(x);
    *x = i * i;
  }

  static constexpr const i64 ALLOCSIZE = INTCOUNT * sizeof(i32);

  const i64 size = va_reset_to(&arena, marker);
  TEST_ASSERT_EQUAL(ALLOCSIZE, size);

  va_destroy(&arena);
}

i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(vmem_alloc_and_cleanup);
  RUN_TEST(varena_marker_and_reset);
  RUN_TEST(varena_lock_and_commit);

  return UNITY_END();
}
