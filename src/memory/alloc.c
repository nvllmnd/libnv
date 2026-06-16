// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/memory/alloc.h"

#include <assert.h>
#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/core/stb_sprintf.h"
#include "nv/iter/iterators.h"

void* vtable_realloc_no_impl(void*, void*, Layout, Layout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, Layout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, Layout, Layout) { return nullptr; }

void* ptr_expect_(const void* ptr, const char* msg) {
  if UNLIKELY (nullptr == ptr) {
    log_fatal("%s", msg);
  }
  // NOTE: We dont mutate this pointer at all, so its safe to cast this back to non-const, since
  // we cast it back to exactly the same type as the pointer was before being passed to this function though the
  // implementation macro
  return pcast(void, ptr);
}

isize ptr_align_offset(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const u64ptr mask = align - 1;
  return cast(isize, cast(u64ptr, ptr) & mask);
}

bool ptr_is_aligned(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

void* ptr_alignup(void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  LOG_DBG("Pointer: %p not currently aligned! aligning to: %p", ptr, (void*)aligned);

  return pcast(void, aligned);
}

u8* ptr_alignto(u8* ptr, u8* end, Layout layout) {
  assert(ptr);
  assert(end);
  assert(end >= ptr);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  u8* const top = ptr_alignup(ptr, layout.align);

  if (top + layout.size >= end) {
    return nullptr;
  }

  return top;
}

u8* ptr_alignin(u8* ptr, i32* space, Layout layout) {
  assert(ptr);
  assert(space);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  const i32 avail = *space;

  if (avail < layout.size) {
    return nullptr;
  }

  u8* const end = ptr + *space;

  u8* const aligned = ptr_alignto(ptr, end, layout);

  if (is_null(aligned)) {
    return nullptr;
  }

  const i32 delta = aligned - ptr;
  *space -= delta;

  return aligned;
}



void* ptr_nonnull_(const void* ptr) {
  return ptr_expect_(ptr, " Expected given pointer to be non-null, but was nullptr! Aborting program!");
}
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

char* va_strdup(IterByte* self, const char* str) {
  const i64 len = stringlen(str);
  return strndup_raw(self, str, len);
}

char* strndup_raw(IterByte* self, const char* str, i32 len) {
  assert(self);
  assert(str);

  const i64 avail = iter_tail(*self) - 1;
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Cannot dup string: %.*s of length %d in Vallocator with only %li bytes available!", len, str, len,
              avail);
    return nullptr;
  }
  if UNLIKELY (len > avail) {
    DWARN("String: %.*s of length: %d will be truncated to %.*s to fit inside vallocator with %li bytes available!",
          len, str, len, (i32)avail, str, avail);
    len = avail;
  }

  char* ptr = allocate_raw(self, mlayout_bytes(len + 1));

  strncpy(ptr, str, len);
  ptr[len] = 0;
  return ptr;
}

sslice sslice_dup_raw(IterByte* self, const char* str, i32 len) {
  const char* ptr = strndup_raw(self, str, len);
  if UNLIKELY (is_null(ptr)) {
    return sslice_empty();
  }
  return sslice_new(.begin = ptr, .len = len);
}

sslice fslice_raw(IterByte* self, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;
  va_start(args);

  const sslice str = vfslice_raw(self, fmt, args);

  va_end(args);

  return str;
}

sslice vfslice_raw(IterByte* self, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  i64 len = 0;
  const char* begin = vfstring_raw(self, &len, fmt, args);

  if UNLIKELY (is_null(begin)) {
    return sslice_new(.begin = begin, .len = len);
  }

  return sslice_new(.begin = begin, .len = len);
}

char* strdup_raw(IterByte* self, const char* str) {
  const i64 len = stringlen(str);
  return strndup_raw(self, str, len);
}

char* fstring_raw(IterByte* self, i64* len_out, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;

  va_start(args);

  char* str = vfstring_raw(self, len_out, fmt, args);
  va_end(args);
  return str;
}

char* vfstring_raw(IterByte* self, i64* len_out, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  const i64 avail = iter_tail(*self);
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Byte Iterator used for string formatting of size: %li bytes has no free memory!", itersize(*self));
    return nullptr;
  }

  const i32 len = min(avail, vfstring_length(fmt, args) + 1);  // +1 for null terminator!

  char* str = punwrap(allocate_raw(self, mlayout_bytes(len)));

  const i64 n = stbsp_vsnprintf(str, len, fmt, args);

  if (n >= len) {
    LOG_DBG(
        "vfstring_raw expanded format string is longer than expected! A truncation has most likely occured with "
        "unexpanded format string: %s",
        fmt);
    *len_out = len - 1;
  } else {
    *len_out = n;
  }

  if (len_out) {
    *len_out = len - 1;  // dont include null terminal in length calc
  }
  return str;
}
