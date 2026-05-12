#include "core/buffer.h"
#include <assert.h>
#include "memory/alloc.h"
#include "memory/layout.h"


struct Buffer {
  i32 len;
  i32 capacity;
  u8  start[];
};
alias(Buffer);

#define asbuff(self) prefix_offset(self, Buffer)

static inline const u8* buff_ctop(const Buff* self) {
  
  const ptr(Buffer) s = asbuff(self);
  const i32 i = min(s->capacity - 1, s->len);
  return &s->start[i];

}

static inline u8* buff_top(Buff* self) {

  ptr(Buffer) s = asbuff(self);
  /// ensure no overflow
  const i32 i = min(s->capacity - 1, s->len);
  return &s->start[i];
  
}

bool buff_is_empty(const Buff* self) {
  const ptr(Buffer) s = asbuff(self);
  return s->len <= 0;
}

bool buff_is_full(const Buff* self) {
  
  const ptr(Buffer) s = asbuff(self);
  return s->len >= s->capacity;
}


Buff* buff_new(i32 capacity, Allocator alloc) {
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

/// Makes space for @param (MemLayout layout) in this Buff.
/// Returns nullptr if capacity is not large enough to acommadate given layout.
METHOD
void* buff_append(Buff* self, MemLayout layout);

/// Appends given string to back of this Buff. Returns a non-empty [sslice] if there is enough capacity for given
/// string up to n bytes.
METHOD
sslice buff_append_nstr(Buff* self, const char* string, i32 n);

/// Appends given null-terminated c-string into this buffer.
/// Returns non-empty [sslice] upon success
METHOD
sslice buff_append_str(Buff* self, const char* string);

/// Resizes given Buff in @param (Allocator alloc).
/// @param (Allocator alloc) MUST BE the SAME allocator used to create this Buff [buff_new], not doing so
/// is at bets a runtime segfault, at worst UB.
METHOD
Buff* buff_resize(Buff* self, i32 new_capacity, Allocator alloc);

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
  Buffer* s = asbuff(self);
  const i32 size = min(s->len, out_len);
  memcpy(out, s, size);
  return size;
}

/// Clears Buff. Sets its inner field to 0
METHOD
void buff_clear(Buff* self) {
  Buffer* s = asbuff(self);
  s->len = 0;
}

/// Same as [buff_clear], but also memsets this Buff to all zeroes
METHOD
void buff_clear_zeroed(Buff* self) {
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
  Buffer* s = asbuff(self);
  allocator_free(alloc, s);
}



/// Returns true if this Buff has enough capacity to fit a @param (MemLayout layout), otherwise false.
bool buff_has_space_for(const Buff* self, MemLayout layout) {
  const ptr(u8) top = align_ptr(buff_ctop(self), layout.align);
  const ptr(u8) next_top = top + layout.size;
  const ptr(u8) end = buff_cend(self);
  return next_top <= end;

}


u8* buff_end(Buff* self) {
  ptr(Buffer) s = asbuff(self);
  return self + s->capacity;
}


const u8* buff_cend(const Buff* self) {
  const Buffer* s = asbuff(self);
  return self + s->capacity;
}

i32 buff_available(const Buff* self) {
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
  const ptr(Buffer) s = asbuff(self);
  return s->len;
}

i32 buff_capacity(const Buff* self) {
  const ptr(Buffer) s = asbuff(self);
  return s->capacity;
}

