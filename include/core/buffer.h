#pragma once

#include "intdefs.h"
#include "memory/alloc.h"

/// A Buffer of bytes. Must be manually resized. If created with an allocator ([buff_new]), then it must be resized and
/// destroyed with the same allocator. This is to save metadata space, plus this is how its done in Zig so i think im
/// going to try it out finally  lol A Buff can also be created from caller memory ([buff_from_mem]), and if it is, the
/// memory it uses is expected to be managed by the caller, and as such, should not be passed to [buff_destroy] (or if
/// it is, make sure its a no-op allocator)
typedef u8 Buff;

CONST_FUNC
i32 buff_min_size(void);

PURE_FUNC
METHOD
i32 buff_len(const Buff* self);

PURE_FUNC
METHOD
i32 buff_capacity(const Buff* self);

PURE_FUNC
METHOD
bool buff_is_empty(const Buff* self);

METHOD
PURE_FUNC
bool buff_is_full(const Buff* self);

Buff* buff_new(i32 capacity, Allocator alloc);

Buff* buff_from_mem(u8* start, u8* end);

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
sslice buff_as_string(const Buff* self);

/// Writes up to @param (i32 out_len) bytes from
/// this buffer into given memory located at @param(void* out)
/// Returns nubmer of bytes written to @param (void* out).
METHOD
i32 buff_into(Buff* self, void* out, i32 out_len);

/// Clears Buff. Sets its inner field to 0
METHOD
void buff_clear(Buff* self);

/// Same as [buff_clear], but also memsets this Buff to all zeroes
METHOD
void buff_clear_zeroed(Buff* self);

/// Releases memory used by this buffer back to the @param (Allocator alloc) that created it.
/// This must be the same allocator that was used to resize/create it. Not doing so is at best a runtime segfault, at
/// worst UB
METHOD
void buff_destroy(Buff* self, Allocator alloc);

/// Returns true if this Buff has enough capacity to fit a @param (MemLayout layout), otherwise false.
METHOD
PURE_FUNC
bool buff_has_space_for(const Buff* self, MemLayout layout);

/// Returns true if this Buffer needs to be resized to fit a @param (Memlayout layout)
METHOD
PURE_FUNC
static inline bool buff_needs_resize_for(const Buff* self, MemLayout layout) {
  return !buff_has_space_for(self, layout);
}

METHOD
PURE_FUNC
i32 buff_available(const Buff* self);

METHOD
u8* buff_end(Buff* self);

METHOD
PURE_FUNC
const u8* buff_cend(const Buff* self);

#define Vec(T) ptr(T)

#define vec_new(T, alloc) ((Vec(T))buff_new(sizeof(T), (alloc)))

#define vec_append(self, T) ((Vec(T))buff_append((self), mlayout_new(T)))

#define vec_push(_self, _val)                                              \
  ({                                                                       \
    static_assert(sizeof(__typeof(*(_self))) == sizeof(__typeof(_val)));     \
    const auto _v = (_val);                                                \
    Buff* _s = (Buff*)(_self);                                             \
    __typeof_unqual(_v)* _elem = buff_append(_s, mlayout_new(typeof(_v))); \
    if (_elem) {                                                           \
      memcpy(_elem, &_v, sizeof(_v));                                      \
    }                                                                      \
  })

#define vec_len(self) ((buff_len(pcast(u8, (self)))) / sizeof(__typeof(*(self))))
#define vec_capacity(self) ((buff_capacity(pcast(u8, (self)))) / (sizeof(__typeof(*(self)))))
#define vec_cend(self) ((const __typeof(self))(buff_cend(pcast(const Buff, (self)))))
#define vec_end(self) ((__typeof(self))(buff_end(pcast(Buff, (self)))))

#define vec_available(self) (buff_available(pcast(const Buff, (self))) / sizeof(__typeof(*(self))))

#define vec_needs_resize_for(self, T) (buff_needs_space_for(pcast(const Buff, (self)), mlayout_new(T)))

#define vec_has_space_for(self, T) (buff_has_space_for(pcast(const Buff, (self)), mlayout_new(T)))

#define vec_destroy(self, alloc) (buff_destroy(pcast(Buff, (self)), (alloc)))

#define vec_clear_zeroed(self) (buff_clear_zeroed(pcast(Buff, (self))))

#define vec_clear(self) (buff_clear(pcast(Buff, (self))))

#define vec_into(self, out, out_len) (buff_into(pcast(Buff, (self)), (out), (out_len) * sizeof(__typeof(*(self)))))
#define vec_resize(self, new_capacity, alloc) \
  (buff_resize(pcast(Buff, (self)), (new_capacity) * sizeof(__typeof(*(self))), (alloc)))

#define vec_from_mem(start, end) (buff_from_mem(pcast(u8, (start)), pcast(u8, (end)))
#define vec_is_full(self) (buff_is_full(pcast(const Buff, (self))))

#define vec_is_empty(self) (buff_is_empty(pcast(const Buff, (self))))
#define vec_min_size buff_min_size
