// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>--license=GPL-3.0-or-later
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>

#include "nv/core/algo.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/spad.h"
#include "nv/core/core_types.h"
#include "nv/memory/vmem.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void move_memory_helpers(void) {
  typedef struct Resource {
    const char* buf;
  } Resource;

  {
    static const char* INPUT = "test";
    static const char* OLD = "old value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    b.buf = move_exchange(a.buf, OLD);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_EQUAL_STRING(a.buf, OLD);
  }
  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    b.buf = move(a.buf);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }

  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    static constexpr const char* none = nullptr;
    b.buf = move_exchange(a.buf, none);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }

  {
    static const char* INPUT = "input value";

    Resource a = make(Resource, INPUT);
    Resource b = make_zeroed(Resource);

    move_into(a.buf, b.buf);

    TEST_ASSERT_EQUAL_STRING(b.buf, INPUT);
    TEST_ASSERT_NULL(a.buf);
  }
}

void tagged_pointers(void) {
  typedef struct Point {
    double x;
    double y;
  } Point;

  const Point val = make(Point, 1000., 0.5265236);

  const Point* original = &val;

  const Point* tptr = tptr_new(original, true);
  const Point* untagged = tptr_ptr(tptr);
  TEST_ASSERT_EQUAL(untagged, original);

  const bool tag = tptr_tag(tptr);

  TEST_ASSERT_TRUE(tag);
}

void stringpad_builds_string(void) {
  Vallocator vm = va_new(MEGABYTES(24), false);
  TEST_ASSERT_TRUE(va_isok(&vm));

    static constexpr const i32 BLEH_COUNT = 200;
    // allocate random space so we can test building strings in the middle of using VirtMem for other stuff
    i32* bleh = va_alloc_array(&vm, i32, BLEH_COUNT);
    TEST_ASSERT_NOT_NULL(bleh);
    for (i32 i = 0; i < BLEH_COUNT; i++) {
      bleh[i] = (i * i * i) ^ i;
    }


  StringPad sp = spad_new(MEGABYTES(24));

  spad_build_start(&sp);

  sslice sl = spad_fappend(&sp, "asdf ayooo %d ", 540);

  TEST_ASSERT_EQUAL_STRING_LEN("asdf ayooo 540 ", sl.begin, sl.len);

  sl = spad_fappend(&sp, "%s", "interpolate!");

  TEST_ASSERT_EQUAL_STRING_LEN("interpolate!", sl.begin, sl.len);

  sl = spad_append(&sp, " we building!");

  TEST_ASSERT_EQUAL_STRING_LEN(" we building!", sl.begin, sl.len);

  char buf[255] = {};

  // spad_length does not count the ending null character,
  // so +1 here to match the value returned from spad_build_end_into below
  const i64 size = spad_length(&sp) + 1;

  const i32 n = spad_build_end_into(&sp, buf, 255);
  UNUSED(n);
  LOG("BUF: %.*s, LEN: %li", (i32)size, buf, size);
  TEST_ASSERT_EQUAL(size, n);

  TEST_ASSERT_EQUAL_STRING("asdf ayooo 540 interpolate! we building!", buf);

  spad_destroy(&sp);

  const i32 avail = va_available(&vm);
  const i32 used = va_used_bytes(&vm);

  LOG("AVAIL: %d", avail);
  LOG("USED: %d", used);

}

void spad_clones_into_arena(void) {}

i32 main(void) {
  UNITY_BEGIN();

  // RUN_TEST(string_compare);
  RUN_TEST(move_memory_helpers);
  RUN_TEST(tagged_pointers);
  RUN_TEST(stringpad_builds_string);

  return UNITY_END();
}
