// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>--license=GPL-3.0-or-later
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/constants.h"

#include "nv/iter/vec.h"

#include "nv/core/intdefs.h"
#include "nv/memory/vmem.h"
#include "unity.h"

static VArena ARENA = {};

static Allocator ALLOC;

void setUp(void) {
  ARENA = va_new(GB(2));
  ALLOC = vallocator(&ARENA);

  
}

void tearDown(void) {
  va_destroy(&ARENA);
  ARENA = (VArena){};
}


void vecs_works(void) {
  Vec(i32) v = vec_new(i32, 100, ALLOC);
  TEST_ASSERT_EQUAL(100, vec_capacity(v));

  vec_push(v, 50);
  TEST_ASSERT_EQUAL(1, vec_len(v));

  TEST_ASSERT_EQUAL(50, v[0]);

  vec_push(v, 100);

  TEST_ASSERT_EQUAL(100, v[1]);

  TEST_ASSERT_EQUAL(2, vec_len(v));
  
  
}



i32 main(void) {
  UNITY_BEGIN();

  RUN_TEST(vecs_works);

  return UNITY_END();
}
