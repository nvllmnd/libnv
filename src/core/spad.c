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

  assert(size_bytes > 0);
  Vallocator mem = va_new(size_bytes);
  if UNLIKELY (!va_isok(&mem)) {
    LOG_ERROR("Filed to create VArena of size: %li bytes!", size_bytes);
    return zeroed(StringPad); 
  }

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
  assert(s);
  assert(len > 0);
  assert_inuse(self);

  const i64 avail = spad_available(self);

  const i64 size = min(len, avail);

  // strncat((char*)self->mem.cursor, s, size - 1);

  char* begin = self->begin;
  const i64 slen = spad_length(self);
  const i64 cap = spad_capacity(self);


  // if (begin[slen] != 0) {
  //   LOG_FATAL("Cannot append source string: %.*s into dest string: %.*s. end character at index: %d, but %c resides at that location instead!", len, s, (i32)slen, begin, );
  // }

  const i64 full_len = stringcat(begin, slen, cap, s, size);
  if UNLIKELY (full_len < 0) {
    LOG_ERROR("Failed to concatenate string for StringPad! %.*s, len: %li with source string: %.*s (len: %li)",
              (i32)slen, begin, slen, (i32)size, s, size);
    return sslice_empty();
  }

  // update VArena cursor for future used/cap/size calculations
  self->cursor += size;
  return sslice_new(.begin = begin, .len = full_len);

  // // NOTE: we want to allocate -1 full requested size,
  // // so that when we write a null character to the end of this appended string,
  // // that null character will get overwritten by the next append (if any).
  // // the size is truncated to available size if this string is too long, minus 1 to account for
  // // terminal null character,
  // const i64 full_size = min(len + 1, avail - 1);

  // // dont tell allocator about the null character
  // // NOTE: This is kind of hacky, but i lowkey like it lol, what wrong with a public struct eh?? =P
  // char* str = va_allocate(&self->mem, mlayout_bytes(full_size - 1));
  // if UNLIKELY (is_null(str)) {
  //   LOG_ERROR("Failed to append string %.*s into StringPad with only %li bytes available", len, s,
  //             va_available(&self->mem));
  //   return sslice_empty();
  // }

  // strncpy(str, s, full_size - 1);
  // // ensure we always  have a null character at the end of this string-pads build string
  // str[full_size] = 0;

  // return sslice_new(.begin = str, .len = full_size - 1);
}


sslice spad_vfappend(StringPad* self, const char* fmt, va_list args) {
  char* dst = self->begin;
  i64 dlen = spad_length(self);
  const i64 cap = spad_capacity(self);


  
  const sslice sl = vfconcat(dst, dlen,  cap, fmt, args);
  self->cursor = self->begin + sl.len;
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


