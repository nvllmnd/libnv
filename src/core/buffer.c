#include "nv/core/buffer.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "nv/core/algo.h"
#include "nv/core_types.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/layout.h"

struct Buffer {
  i32 len;
  i32 capacity;
  u8 start[];
};
alias(Buffer);

#define asbuff(self) prefix_offset(self, Buffer)

#define buff_start(self) (&((self)->start[0]))

i32 buff_set_len(Buff* s, i32 new_len) {
  assert(s);

  Buffer* self = asbuff(s);

  const i32 len = self->len;
  const i32 cap = self->capacity;

  if (len == new_len) {
    return len;
  }

  // clamp to capacity so we dont accidently trigger UB
  new_len = clamp(new_len, 0, cap);

  self->len = new_len;

  return self->len;
}

MemError buff_putchar(Buff* self, char c) {
  assert(self);

  Buffer* s = asbuff(self);
  if (s->len < s->capacity) {
    s->start[s->len] = c;
    s->len += 1;
    return OK;
  }

  return MemError__BufferNeedsResize;
}

MemError buff_putbyte(Buff* self, u8 b) {
  assert(self);
  Buffer* s = asbuff(self);

  if (s->len < s->capacity) {
    s->start[s->len] = b;
    s->len += 1;
    return OK;
  }

  return MemError__BufferNeedsResize;
}

static inline const u8* buff_ctop(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  const i32 i = min(s->capacity - 1, s->len);
  return &s->start[i];
}

static inline u8* buff_top(Buff* self) {
  assert(self);
  ptr(Buffer) s = asbuff(self);
  /// ensure no overflow
  const i32 i = min(s->capacity - 1, s->len);
  return &s->start[i];
}

bool buff_is_empty(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  return s->len <= 0;
}

bool buff_is_full(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  return s->len >= s->capacity;
}

Buff* buff_new(i32 capacity, Allocator alloc) {
  assert(vtmask_is_ok(alloc.vtable->mask));
  assert(capacity > 0);
  Buffer* self = allocator_allocate(alloc, mlayout_fma(Buffer, capacity));
  self->len = 0;
  self->capacity = capacity;
  return &self->start[0];
}

Buff* buff_from_mem(u8* start, u8* end) {
  assert(start && end);
  const i32 size = end - start;
  if (size <= buff_min_size()) {
    return nullptr;
  }

  Buffer* self = pcast(Buffer, start);
  self->len = 0;
  self->capacity = size - sizeof(Buffer);
  return &self->start[0];
}

METHOD
static inline u8* buff_aligned_top(Buff* self, MemLayout layout) {
  assert(self);
  ptr(u8) top = ptr_alignup(buff_top(self), layout.align);
  ptr(u8) top_end = top + layout.size;
  ptr(u8) buffer_end = buff_end(self);
  if LIKELY (top_end <= buffer_end) {
    return top;
  }
  return nullptr;
}

/// Makes space for @param (MemLayout layout) in this Buff.
/// Returns nullptr if capacity is not large enough to acommadate given layout.
METHOD
void* buff_append(Buff* self, MemLayout layout) {
  assert(self);
  assert(layout.size > 0);
  assert(IS_POWER_OF_2(layout.align));
  LOG_DBG("appending layout of size: %d and alignment: %d", layout.size, layout.align);

  Buffer* s = asbuff(self);
  u8* next_top = buff_aligned_top(self, layout);
  if LIKELY (is_not_null(next_top)) {
    const u8* top_end = next_top + layout.size;
    s->len = top_end - buff_start(s);
    return next_top;
  }
  LOG_DBG("Aligned top was null! buffer len: %d. buffer cap: %d", s->len, s->capacity);

  return nullptr;
}

/// Appends given string to back of this Buff. Returns a non-empty [sslice] if there is enough capacity for given
/// string up to n bytes.
METHOD
sslice buff_append_nstr(Buff* self, const char* string, i32 n) {
  assert(self);
  assert(string);
  assert(n > 0);
  char* str = buff_append(self, mlayout_bytes(n));
  if UNLIKELY (is_null(str)) {
    return sslice_empty();
  }

  strncpy(str, string, n);
  return sslice_new(str, n);
}

/// Appends given null-terminated c-string into this buffer.
/// Returns non-empty [sslice] upon success
METHOD
sslice buff_append_str(Buff* self, const char* string) {
  assert(self);
  assert(string);
  const i32 len = stringlen(string);
  return buff_append_nstr(self, string, len);
}

/// Resizes given Buff in @param (Allocator alloc).
/// @param (Allocator alloc) MUST BE the SAME allocator used to create this Buff [buff_new], not doing so
/// is at bets a runtime segfault, at worst UB.
METHOD
Buff* buff_resize(Buff* s, i32 new_capacity, Allocator alloc) {
  assert(s);
  assert(new_capacity > 0);
  assert(vtmask_is_ok(alloc.vtable->mask));
  Buffer* self = asbuff(s);
  const i32 curr_cap = self->capacity;

  if UNLIKELY (curr_cap == new_capacity) {
    return s;
  }
  if (new_capacity < curr_cap) {
    self->len = new_capacity;
    return s;
  }

  const auto old = mlayout_fma(Buffer, self->capacity);
  const auto new_layout = mlayout_fma(Buffer, new_capacity);
  if (vtmask_has_expand(alloc.vtable->mask)) {
    self = allocator_expand(alloc, self, old, new_layout);
    if LIKELY (is_not_null(self)) {
      return &self->start[0];
    }
  }

  if (vtmask_has_realloc(alloc.vtable->mask)) {
    self = allocator_reallocate(alloc, self, old, new_layout);
    if LIKELY (is_not_null(self)) {
      return &self->start[0];
    }
    return nullptr;

  } else {
    LOG_DBG(
        "%s[%s::%s]:%d :: Attempted to resize buffer with an allocator that does not have a reallocate method "
        "implemented! Cannot resize buffer. returning nullptr!",
        __FILE__, STRINGIFY(Buff), __func__, __LINE__);
    return nullptr;
  }
}

/// Casts/reinterprets this Buff to a [sslice]
METHOD
PURE_FUNC
sslice buff_as_string(const Buff* self) {
  assert(self);
  const Buffer* s = asbuff(self);
  const sslice sl = sslice_new(.begin = pcast(char, self), .len = s->len);

  return sl;
}

/// Writes up to @param (i32 out_len) bytes from
/// this buffer into given memory located at @param(void* out)
/// Returns nubmer of bytes written to @param (void* out).
METHOD
i32 buff_write(Buff* self, void* out, i32 out_len) {
  assert(self);
  assert(out);
  assert(out_len > 0);
  Buffer* s = asbuff(self);
  const i32 size = min(s->len, out_len);
  memcpy(out, s, size);
  return size;
}

void buff_clear(Buff* self) {
  assert(self);
  Buffer* s = asbuff(self);
  s->len = 0;
}

void buff_clear_zeroed(Buff* self) {
  assert(self);
  Buffer* s = asbuff(self);
  const i32 old_len = s->len;

  buff_clear(self);
  memset(&s->start[0], 0, old_len);
}

i32 buff_grow_to_cap(Buff* s) {
  assert(s);
  Buffer* self = asbuff(s);

  self->len = self->capacity;

  return self->len;
}

const void* buff_cindex(const Buff* s, i32 index) {
  assert(s);
  const Buffer* self = asbuff(s);
  if (index < 0 || index >= self->len) {
    log_fatal(FILE_FMT "Attempted to index Buffer of length: %d with an index that is out of range!: %d",
              FILE_FMT_ARGS(Buff, self->len, index));
  }
  return &self->start[index];
}

void* buff_index(Buff* s, i32 index) {
  assert(s);
  Buffer* self = asbuff(s);
  if (index < 0 || index >= self->len) {
    log_fatal(FILE_FMT "Attempted to index Buffer of length: %d with an index that is out of range!: %d",
              FILE_FMT_ARGS(Buff, self->len, index));
  }
  return &self->start[index];
}

void buff_fatal_error(Buff* s, const char* fmt, ...) {
  va_list args;
  va_start(args);

  Buffer* self = asbuff(s);
  println(
      "Buffer of size: %d bytes, and capacity %d bytes experienced an unrecoverable error when calling one of its "
      "functions!",
      self->len, self->capacity);

  vlog_fatal(fmt, args);
}

void buff_clear_zeroed_cap(Buff* self) {
  // set length to capacity so when we call [buff_clear_zeroed], it memsets the entire capacity of this Buff to 0 and
  // then sets our length field to 0
  buff_set_len(self, buff_capacity(self));
  buff_clear_zeroed(self);
}

/// Releases memory used by this buffer back to the @param (Allocator alloc) that created it.
/// This must be the same allocator that was used to resize/create it. Not doing so is at best a runtime segfault, at
/// worst UB
METHOD
void buff_destroy(Buff* self, Allocator alloc) {
  assert(self);
  assert(vtmask_has_free(alloc.vtable->mask));
  Buffer* s = asbuff(self);
  allocator_free(alloc, s);
}

/// Returns true if this Buff has enough capacity to fit a @param (MemLayout layout), otherwise false.
bool buff_has_space_for(const Buff* self, MemLayout layout) {
  assert(self);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);
  const ptr(u8) top = ptr_alignup((void*)buff_ctop(self), layout.align);
  const ptr(u8) next_top = top + layout.size;
  const ptr(u8) end = buff_cend(self);
  return next_top <= end;
}

u8* buff_end(Buff* self) {
  assert(self);
  ptr(Buffer) s = asbuff(self);
  return self + s->capacity;
}

const u8* buff_cend(const Buff* self) {
  assert(self);
  const Buffer* s = asbuff(self);
  return self + s->capacity;
}

i32 buff_available(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  const i32 len = s->len;
  const i32 cap = s->capacity;
  return cap - len;
}

i32 buff_min_size(void) {
  static constexpr const i32 MIN_SIZE = sizeof(Buffer);
  return MIN_SIZE;
}

i32 buff_len(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  return s->len;
}

i32 buff_capacity(const Buff* self) {
  assert(self);
  const ptr(Buffer) s = asbuff(self);
  return s->capacity;
}
