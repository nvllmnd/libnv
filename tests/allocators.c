// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nv/core/algo.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/fmap.h"
#include "nv/memory/heap.h"
#include "nv/memory/mpool.h"
#include "nv/memory/vmem.h"
#include "unity.h"

static constexpr const auto STORAGE_SIZE = 1 << 16;
static byte STORAGE[STORAGE_SIZE] = {};
static Arena ARENA = {};

void setUp(void) { arena_init(&ARENA, STORAGE, STORAGE_SIZE); }

void tearDown(void) {
  memset(STORAGE, 0, STORAGE_SIZE);
  memset(&ARENA, 0, sizeof(Arena));
}

struct Stuff {
  char buf[4096];

  struct Point {
    f32 x;
    f32 y;
  } points[20];

  i64 counter;
};
alias(Stuff);

alias(Point);

void heap_can_allocate(void) {
  Heap* h = heap_new(MB(128));
  TEST_ASSERT_NOT_NULL(h);
  Stuff* x = heap_alloc(h, sizeof(Stuff), alignof(Stuff));
  TEST_ASSERT_NOT_NULL(x);

  TEST_ASSERT_EQUAL(sizeof(Stuff), heap_size_of(h, x));

  x->counter = 6969;

  TEST_ASSERT_EQUAL(6969, x->counter);
  heap_free(h, x);
  x = nullptr;

  heap_destroy(h);
  h = nullptr;
}

void heap_can_reallocate(void) {
  Heap* h = heap_new(MB(128));
  TEST_ASSERT_NOT_NULL(h);

  Stuff* x = heap_alloc(h, sizeof(Stuff) * 4, alignof(Stuff));
  TEST_ASSERT_NOT_NULL(x);

  TEST_ASSERT_EQUAL(sizeof(Stuff) * 4, heap_size_of(h, x));

  x = heap_realloc(h, x, sizeof(Stuff) * 8, alignof(Stuff));
  TEST_ASSERT_NOT_NULL(x);

  TEST_ASSERT_EQUAL(sizeof(Stuff) * 8, heap_size_of(h, x));

  heap_free(h, x);
  x = nullptr;

  heap_destroy(h);
  h = nullptr;
}

void arena_static_mem_works(void) {
  TEST_ASSERT_FALSE(is_none(&ARENA));
  Stuff* val = arena_alloc(&ARENA, mlayout_new(Stuff));
  TEST_ASSERT_NOT_NULL(val);

  const byte* old = ARENA.cursor;
  {
    ScopedArena child = arena_scoped(&ARENA);

    isize len = 0;
    const char* str = arena_fstring(&child, &len, "AYOO WE FORMATTED THIS BI: %li", 4206969L);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING_LEN("AYOO WE FORMATTED THIS BI: 4206969", str, len);
  }

  TEST_ASSERT_EQUAL(old, ARENA.cursor);

  isize len = 0;

  const char* str = arena_fstring(&ARENA, &len, "%s", "TEST AFTER");
  TEST_ASSERT_NOT_NULL(str);
  TEST_ASSERT_EQUAL_STRING_LEN("TEST AFTER", str, len);
}

void fmap_loads_files(void) {
  FileMap fm = {};

  NvError err = NVOK;

  err = fmap_load_all_init(&fm, "../../tests/data/file1.txt", "../../tests/data/file2.txt");
  TEST_ASSERT_EQUAL(Error__Ok, err);

  fmap_destroy(&fm);
}

void mpool_alloc_free(void) {
  static constexpr const i64 STORAGE_SIZE = KB(2);
  char storage[STORAGE_SIZE] = {};

  auto pool = mpool_new(Point, storage, storage + STORAGE_SIZE);

  TEST_ASSERT_TRUE(mpool_is_ok(pool));

  auto mp = &pool;

  static constexpr const f32 XV = 50505050.f;
  static constexpr const f32 YV = XV * 50;

  Point* x = mpool_allocate(mp);
  TEST_ASSERT_NOT_NULL(x);

  x->x = XV;

  Point* y = mpool_zallocate(mp);
  TEST_ASSERT_NOT_NULL(y);

  const f32 sample_x = x->x;

  y->y = YV;

  (void)mpool_allocate(mp);

  const f32 sample_y = y->y;

  mpool_free(mp, x);
  mpool_free(mp, y);

  TEST_ASSERT_EQUAL(XV, sample_x);
  TEST_ASSERT_EQUAL(YV, sample_y);
}

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

  err = vmem_prefault_range(vm, vmem_begin(vm), KB(24));
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
  RUN_TEST(mpool_alloc_free);
  RUN_TEST(fmap_loads_files);
  RUN_TEST(arena_static_mem_works);
  RUN_TEST(heap_can_allocate);
  RUN_TEST(heap_can_reallocate);

  return UNITY_END();
}
