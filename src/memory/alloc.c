// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/memory/alloc.h"

#include <assert.h>
#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/iter/iterators.h"

void* vtable_realloc_no_impl(void*, void*, Layout, Layout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, Layout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, Layout, Layout) { return nullptr; }

void* allocate_raw(IterByte* self, const Layout layout) {
  assert(iter_is_ok(self));
  assert(self->cursor);

  byte* ptr = (byte*)ptr_alignup(self->cursor, layout.align);
  byte* next = ptr + layout.size;

  if UNLIKELY (next >= self->end) {
    LOG_ERROR("Iterator Raw Allocation Failed! Not enough space left between cursor and end of byte iterator!");
    return nullptr;
  }

  self->cursor = next;
  return ptr;
}

void* zallocate_raw(IterByte* self, const Layout layout) {
  void* ptr = allocate_raw(self, layout);
  if UNLIKELY (is_null(ptr)) {
    return ptr;
  }

  memset(ptr, 0, layout.size);
  return ptr;
}

bool resize_raw(IterByte* self, void* ptr, Layout old, Layout new) {
  assert(self);
  assert(ptr);
  if (old.size == new.size) {
    return false;
  }
  byte* last = (self->cursor - old.size);
  if (last == (byte*)ptr) {
    // Shrink if newsize is greater oldsize, otherwise grow
    const i64 delta = new.size - old.size;
    self->cursor += delta;

    return true;
  }

  return false;
}

void* reallocate_raw(IterByte* self, void* ptr, Layout old, Layout new) {
  if UNLIKELY (old.size == new.size) {
    return ptr;
  }

  if (resize_raw(self, ptr, old, new)) {
    return ptr;
  }

  if (new.size < old.size) {
    return ptr;
  }

  void* res = allocate_raw(self, new);
  if UNLIKELY (is_null(res)) {
    LOG_ERROR("Failed to Reallocate Memory of size %li bytes to size %li bytes", old.size, new.size);
    return nullptr;
  }

  memcpy(res, ptr, old.size);

  return res;
}

#if LIBNV_INTERNAL == 0

#ifndef LIBNV_FREE_RAW_WARN
#define LIBNV_FREE_RAW_WARN 1
#endif

#else
// NOTE: Suppress warnings for internal use
#define LIBNV_FREE_RAW_WARN 0
#undef LIBNV_SUPPRESS_MILD_ERRORS
#define LIBNV_SUPPRESS_MILD_ERRORS 0

#endif

#ifndef LIBNV_SUPPRESS_MILD_ERRORS
#define LIBNV_SUPPRESS_MILD_ERRORS 0
#endif

static inline void warn_usage(void);

void free_raw(IterByte* self, void* ptr, Layout layout, u64 pattern) {
  assert(self);

  warn_usage();

  if (ptr && contains(*self, ptr)) {
    memset(ptr, pattern, layout.size); 
  }
  
}



void warn_usage(void) {
#if LIBNV_FREE_RAW_WARN == 1

#if LIBNV_SUPPRESS_MILD_ERRORS == 0
#warning \
    "function: free_raw  only zeroes memory, it does not actually release memory. you can suppress this message by defining LIBNV_FREE_RAW_WARN as 0. If you have are using -Werror, you must also define LIBNV_SUPPRESS_MILD_ERRORS to a non-zero value";
#endif  // LIBNV_SUPPRESS_MILD_ERRORS == 0

  LOG_INFO(
      "RAW FREE => Attempting to zero memory of size: %li bytes and alignment: %li with byte pattern: %lu. If you are "
      "sure of what you are doing, and are aware that calling raw_free does not release any memory, instead is used to "
      "mark memory as available for reuse, you can safely ignore this. You can also turn this message off by compiling "
      "libnv with the flag: '-DLIBNV_FREE_RAW_WARN=0'",
      layout.size, layout.align, pattern);
#endif  // LIBNV_FREE_RAW_WARN == 1
}
