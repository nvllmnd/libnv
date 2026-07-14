// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/spad.h"

#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/core/sslice.h"
#include "nv/memory/alloc.h"
#include "nv/memory/vmem.h"

#ifndef LIBNV_SPAD_STRICT
#define LIBNV_SPAD_STRICT 1
#endif

static inline void assert_inuse(const StringPad* self) {
#if LIBNV_SPAD_STRICT == 1
  if UNLIKELY (!self->inuse) {
    LOG_FATAL("Expected StringPad to be in use when it isnt!");
  }
#endif
}

static inline void assert_not_inuse(const StringPad* self) {
#if LIBNV_SPAD_STRICT == 1
  if UNLIKELY (self->inuse) {
    LOG_FATAL("Expected StringPad to not be in use but it is!");
  }
#endif
}

sslice spad_clone_string(StringPad* self, Allocator alloc) {
  assert_inuse(self);
  const i64 size = spad_length(self);

  char* cpy = allocator_allocate(alloc, mlayout_bytes(size + 1));
  if UNLIKELY (is_null(cpy)) {
    LOG_ERROR("StringPad finished building string of size: %li bytes but failed to clone into given allocator!", size);
    return sslice_empty();
  }
  strncpy(cpy, self->begin, size);
  cpy[size + 1] = 0;
  return sslice_new(.begin = cpy, .len = size);
}

StringPad spad_new(char* begin, char* end) {
  assert(begin < end);
  const i64 size_bytes = end - begin;

  return (StringPad){.inuse = false, .begin = begin, .cursor = begin, .end = begin + size_bytes};
}

void spad_build_start(StringPad* self) {
  assert(self);
  assert_not_inuse(self);

  self->inuse = true;
  spad_clear(self);
}
sslice spad_build_end(StringPad* self, Allocator alloc) {
  assert(self);
  assert_inuse(self);

  self->inuse = false;
  return spad_clone_string(self, alloc);
}

i32 spad_clone_into(StringPad* self, char* buff_out, i32 buff_len) {
  assert(self);
  assert(buff_out);
  assert(buff_len > 0);
  assert_inuse(self);

  const i64 size = spad_length(self);
  const i64 len = min(buff_len - 1, size + 1);

  strncpy(buff_out, self->begin, len);
  buff_out[len] = 0;

  return len;
}

i64 spad_build_end_into(StringPad* self, char* buff_out, i64 buff_len) {
  assert_inuse(self);

  const i64 n = spad_clone_into(self, buff_out, buff_len);

  self->inuse = false;
  return n;
}

sslice spad_nappend(StringPad* self, const char* s, i32 len) {
  assert(self);

  if UNLIKELY (len < 0) {
    LOG_ERROR("expected non-negative length but got: %d!", len);
    return sslice_empty();
  }

  if UNLIKELY (is_null(s)) {
    LOG_ERROR("Cannot append null string!");
    return sslice_empty();
  }

  assert_inuse(self);

  i64 avail = spad_available(self);

  if (avail >= INT32_MAX) {
    LOG_ERROR("INT OVERFLOW!!");
    avail = INT32_MAX - 1;
  } else if (avail < 0) {
    LOG_ERROR("INT UNDERFLOW!");
    avail = 0;
  }
  const i32 size = min(len, avail);


  char* begin = self->begin;
  const i64 slen = spad_length(self);
  const i64 cap = spad_capacity(self);


  assert(self->cursor == &begin[slen] && "Sanity check!");

  const i32 full_len = stringcat(begin, slen, cap, s, size);

  if UNLIKELY (full_len < 0) {
    LOG_ERROR("Failed to concatenate string: %.*s of length: %d into end of StringPad! available: %li, ",
              len, s, begin,  len, len, s);
    return sslice_empty();
  }

  const sslice sl = sslice_new(.begin = self->cursor, .len = len);

  self->cursor += len;

  return sl;

}


sslice spad_vfappend(StringPad* self, const char* fmt, va_list args) {
  const i64 cap_full = spad_capacity(self);
  const i32 cap = cap_full >= INT32_MAX ? INT32_MAX - 1: cap_full;

  const sslice sl = vfconcat(self->cursor, cap, fmt, args);

  self->cursor += sl.len;

  return sl;
}

 sslice spad_fappend(StringPad* self, const char* fmt, ...) {
  va_list args;
  va_start(args);

  const sslice str = spad_vfappend(self, fmt, args);

  va_end(args);
  
  return str;
}



sslice spad_append(StringPad* self, const char* s) {
  assert(self);
  assert(s);

  const i64 len = stringlen(s);
  return spad_nappend(self, s, len);
}

char spad_putchar(StringPad* self, char c) {
  assert(self);
  assert_inuse(self);

  if (spad_available(self) >= 1) {
    *self->cursor = c;
    self->cursor += 1;
    return c;
  }
  LOG_ERROR("StringPad of size: %li bytes has no available space to append character: %c", spad_capacity(self), c);
  return -1;
}

u8 spad_putbyte(StringPad* self, u8 byte) {
  assert(self);
  assert_inuse(self);

  if (spad_available(self) >= 1) {
    *self->cursor = byte;
    self->cursor += 1;
    return byte;
  }
  LOG_ERROR("StringPad of size: %li bytes has no available space to append character: %b", spad_capacity(self), byte);
  return -1;
}

void spad_clear(StringPad* self) {
  assert(self);


  self->cursor = self->begin;
  
}

void spad_clear_zeroed(StringPad* self) {
  assert(self);


  const i64 size = self->cursor - self->end;
  spad_clear(self);

  memset(self->begin, 0, size);
}


