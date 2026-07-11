// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/algo.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/spad.h"
#include "nv/memory/alloc.h"
#include "unity.h"

char STORAGE[KB(16)] = {};

void setUp(void) {}

void tearDown(void) {}

// void small_files(void) {
//   // File* file = file_read_mem("/home/nvllmnd/sauce/priv/libnv/tests/data/file1.txt", ALLOC);

//   File* file = file_read_mem("./data/file1.txt", ALLOC);

//   TEST_ASSERT_NOT_NULL(file);

// }

i32 main(void) {
  UNITY_BEGIN();

  // RUN_TEST(small_files);

  return UNITY_END();
}
