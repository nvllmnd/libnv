#include "core/buffer.h"

#include <assert.h>
#include <string.h>

#include "core_types.h"
#include "log.h"
#include "memory/alloc.h"
#include "memory/cstr.h"
#include "memory/layout.h"

struct Buffer {
  i32 len;
  i32 capacity;
  u8 start[];
};
alias(Buffer);

#define asbuff(self) prefix_offset(self, Buffer)
#define buff_start(self) (&((self)->start[0]))

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
  ptr(u8) top = align_ptr(buff_top(self), layout.align);
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

  u8* next_top = buff_aligned_top(self, layout);
  if LIKELY (is_not_null(next_top)) {
    Buffer* s = asbuff(self);

    const u8* top_end = next_top + layout.size;
    s->len = top_end - buff_start(s);
    return next_top;
  }

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
i32 buff_into(Buff* self, void* out, i32 out_len) {
  assert(self);
  assert(out);
  assert(out_len > 0);
  Buffer* s = asbuff(self);
  const i32 size = min(s->len, out_len);
  memcpy(out, s, size);
  return size;
}

/// Clears Buff. Sets its inner field to 0
METHOD
void buff_clear(Buff* self) {
  assert(self);
  Buffer* s = asbuff(self);
  s->len = 0;
}

/// Same as [buff_clear], but also memsets this Buff to all zeroes
METHOD
void buff_clear_zeroed(Buff* self) {
  assert(self);
  Buffer* s = asbuff(self);
  const i32 old_len = s->len;

  buff_clear(self);
  memset(&s->start[0], 0, old_len);
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
  const ptr(u8) top = align_ptr(buff_ctop(self), layout.align);
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
