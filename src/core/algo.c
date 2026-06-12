// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/algo.h"

#include <assert.h>
#include <string.h>
#include "nv/core/core_types.h"
#include "nv/core/log.h"
#include "nv/core/stb_sprintf.h"

void* ptr_expect_(const void* ptr, const char* msg) {

  if UNLIKELY (nullptr == ptr) {
    log_fatal("%s", msg);
  }
  // NOTE: We dont mutate this pointer at all, so its safe to cast this back to non-const, since
  // we cast it back to exactly the same type as the pointer was before being passed to this function though the implementation macro
  return pcast(void, ptr);  
}

bool stringeq(const char* left, const char* right) {
  if (left == right) {
    return true;
  }

  const i32 llen = stringlen(left);
  const i32 rlen = stringlen(right);

  if (llen == rlen) {
    return strncmp(left, right, llen) == 0;
  }

  return false;
}

static constexpr const u32 PRIME32 = 0x010001930;
static constexpr const u32 OFFSET32 = 0x811c9dc5;
static constexpr const u64 PRIME64 = 0x00000100000001b3;
static constexpr const u64 OFFSET64 = 0xcbf29ce484222325;


u32 fnv_hash32(const char* string, isize len) {
  assert(string);
  assert(len > 0);
  
  u32 hash = OFFSET32;
  for (i32 i = 0; i < len; i++) {
    const u32 c = string[i];
    hash = (hash ^ c) * PRIME32;
  }
  return hash;
}

u64 fnv_hash64(const char* string, isize len) {
  assert(string);
  assert(len > 0);

  u64 hash = OFFSET64;

  for (i32 i = 0; i < len; i++) {
    const u64 c = string[i];
    hash = (hash ^ c) * PRIME64;
  }
  return hash;
}



isize str_len(const char* string, isize max_len) {
  if UNLIKELY (is_null(string)) {
    return 0;
  }

  isize len = 0;
  while ((string[len] != 0) && (len <= max_len)) {
    len++;
  }
  return len;
}


bool sslice_eq(sslice left, sslice right) {
  if (left.len == right.len) {
    return sslice_cmp(left, right) == 0;
  }
  return false;
}


i32 sslice_cmp(sslice left, sslice right) {

  if (left.begin == nullptr) { return -1; }
  if (right.begin == nullptr) { return 1; }
  return strncmp(left.begin, right.begin, min(left.len, right.len));
}

void* ptr_nonnull_(const void* ptr) {
  return ptr_expect_(ptr, " Expected given pointer to be non-null, but was nullptr! Aborting program!");
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

 void* ptr_alignup( void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

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

i32 fstring_length(const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const i32 len = vfstring_length(fmt, args);

  va_end(args);

  return len;
  
}

i32 vfstring_length(const char* fmt, va_list args) {
  va_list cpy = {};
  va_copy(cpy, args);

  const i32 len =  stbsp_vsnprintf(nullptr, 0, fmt, cpy);

  va_end(cpy);
  return len;
  
}

