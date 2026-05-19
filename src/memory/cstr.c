#include "nv/memory/cstr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nv/core/attributes.h"
#include "nv/core_types.h"

static void init_small_cstr(cstr* self, const char* string, usize len) {
  assert(len <= SMALL_BUF_SIZE);
  self->buf.len = cast(u8, len);
  strncpy(self->buf.mem, string, len);
  self->buf.mem[SMALL_BUF_SIZE] = 0;
}

static u8* alloc_prefix_memory(usize size) {
  const usize alloc_size = size + sizeof(i32) + 1;

  u8* s = calloc(sizeof(u8), alloc_size);
  assert(nullptr != s);

  return s;
}
static prefix_str prefix_str_new(const char* string, usize len) {
  u8* s = alloc_prefix_memory(len);

  i32* prefix_ptr = pcast(i32, s);

  *prefix_ptr = cast(i32, len);

  prefix_str string_begin = pcast(char, prefix_ptr + 1);
  strncpy(string_begin, string, len);
  return string_begin;
}

static void prefix_str_set_len(prefix_str ps, usize new_len) {
  i32* prefix = pcast(i32, ps);
  prefix = &prefix[-1];

  *prefix = cast(i32, new_len);
}

PARAMS_NONNULL(1, 2)
static void init_large_cstr(cstr* self, const char* string, usize len) {
  assert(len > SMALL_BUF_SIZE);

  self->is_large = true;

  prefix_str string_begin = prefix_str_new(string, len);

  self->heap = string_begin;
}

cstr cstr_new(const char* string) {
  const usize len = stringlen(string);
  return cstr_from_slice(string, len);
}

cstr cstr_large_with_capacity(usize capacity) {
  u8* mem = alloc_prefix_memory(capacity);

  i32* prefix_ptr = pcast(i32, mem);
  *prefix_ptr = cast(i32, capacity);

  prefix_str str_begin = pcast(char, prefix_ptr + 1);

  return (cstr){.is_large = true, .heap = str_begin};
}

cstr cstr_from_slice(const char* string, usize len) {
  cstr self = {};
  if (len <= SMALL_BUF_SIZE) {
    init_small_cstr(&self, string, len);
  } else {
    init_large_cstr(&self, string, len);
  }
  return self;
}

CStrError cstr_init_small(cstr* self, const char* string) {
  const usize string_len = stringlen(string);

  if (string_len <= SMALL_BUF_SIZE) {
    self->buf.len = cast(u8, string_len);
    strncpy(self->buf.mem, string, string_len);
    self->buf.mem[SMALL_BUF_SIZE] = 0;
    return CStrError__Ok;
  }

  return CStrError__StringTooBig;
}

cstr cstr_small_new(const char* string) {
  const usize slen = stringlen(string);

  const usize len = min(slen, SMALL_BUF_SIZE);
  cstr self = {};
  init_small_cstr(&self, string, len);
  return self;
}
void cstr_free(cstr* self) {
  // We only have to call free if this is a large
  // cstr that has been allocated on the heap
  if (self->is_large) {
    i32* prefix_end = pcast(i32, self->heap);
    i32* prefix = prefix_end - 1;
    void* data = pcast(void, prefix);
    free(data);

    self->heap = nullptr;
    // *self = (cstr){};
  }
}

bool cstr_shrink_to(cstr* self, usize smaller_size) {
  const usize len = cstr_len(*self);
  // we were given a bogus, insignificant value, bail out
  if (smaller_size >= len) {
    return false;
  }

  if (self->is_large) {
    prefix_str ps = self->heap;
    prefix_str_set_len(ps, smaller_size);
    ps[len] = 0;  // trailing null character
  } else {
    self->buf.len = cast(u8, smaller_size);
    self->buf.mem[self->buf.len] = 0;  // trailing null character
  }

  return true;
}

cstr i64_truncate_into(i64 n) {

  cstr self = {};

  const i32 len = snprintf(self.buf.mem, SMALL_BUF_SIZE, "%13li", n);
  assert(len <= SMALL_BUF_SIZE && len > 0);

  self.buf.len = len;
  return self;  
}

cstr i32_to_cstr(i32 n) {
  cstr self = {};

  const i32 len = snprintf(self.buf.mem, SMALL_BUF_SIZE, "%13d", n);
  assert(len <= SMALL_BUF_SIZE && len > 0);

  self.buf.len = len;
  return self;
}

cstr cstr_concat(const cstr* left, const cstr* right) {
  const usize llen = cstr_len(*left);
  const usize rlen = cstr_len(*right);

  const char* l = cstr_as_ptr(left);
  const char* r = cstr_as_ptr(right);

  const usize capacity = (llen + rlen);

  cstr self = cstr_large_with_capacity(capacity);
  strncpy(self.heap, l, llen);
  strncpy(&self.heap[llen], r, rlen);
  self.heap[capacity] = 0;

  return self;
}

cstr cstr_move_concat(cstr* left, cstr* right) {
  cstr self = cstr_concat(left, right);
  cstr_free(left);
  cstr_free(right);
  return self;
}

void cstr_append_string(cstr* self, const char* s, usize slen) {
  const usize len = cstr_len(*self);
  const usize next_size = len + slen;

  if (self->is_large) {
    const usize size = next_size + sizeof(i32) + 1;
    u8* mem = realloc(pcast(void, self->heap), size);
    assert(nullptr != mem);

    i32* prefix = pcast(i32, mem);
    *prefix = next_size;

    prefix_str ps = pcast(char, prefix + 1);
    // copy over our appended string
    strncpy(&ps[len], s, slen);

    self->heap = ps;

  } else {
    // we have to move our string from the stack onto the heap, appending @param(s)

    if (next_size >= SMALL_BUF_SIZE) {
      // copy our current string along side arg string
      char string[next_size] = {};
      strncpy(string, self->buf.mem, self->buf.len);
      strncpy(&string[self->buf.len], s, slen);
      // shove that into a prefix_string on the heap
      prefix_str ps = prefix_str_new(string, next_size);
      // Set our flags now that this cstr has changed its inner union type
      self->is_large = true;
      // Zero out old static storage
      self->buf = (CStrSmall){};
      // Our  string's new home!
      self->heap = ps;
    }
  }
}

sslice cstr_slice(const cstr* self, isize from, isize to) {
  const isize len = cstr_len(*self);
  const isize slice_len = to - from;

  // We were given bogus values for indicies (negative from, or a to that is less that from, ect...)
  if (slice_len > len || slice_len < 0) {
    return (sslice){};
  }

  const char* string = cstr_as_ptr(self);
  const char* begin = &string[from];
  return sslice_new(.begin = begin, .len = slice_len);
}
