#pragma once

#include "nv/core/attributes.h"
#include "nv/core/sslice.h"
#include "nv/core/intdefs.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

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

METHOD
PURE_FUNC
RETURNS_NON_NULL
const void* buff_cindex(const Buff* self, i32 index);

METHOD
PURE_FUNC
RETURNS_NON_NULL
void* buff_index(Buff* self, i32 index);

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

METHOD
MemError buff_putchar(Buff* self, char c);

METHOD
MemError buff_putbyte(Buff* self, u8 b);

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
i32 buff_write(Buff* self, void* out, i32 out_len);

/// Clears Buff. Sets its inner field to 0
METHOD
void buff_clear(Buff* self);

/// Same as [buff_clear], but also memsets this Buff to zeroes from start up to current length before callig
/// [buff_clear]
METHOD
void buff_clear_zeroed(Buff* self);

/// same as [buff_clear_zeroed], but clears the entire capacity of this buff
METHOD
void buff_clear_zeroed_cap(Buff* self);

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

/// Sets This Buff's length to @param (i32 new_len).
/// @param (i32 new_len) is clamped at current Buff capacity
/// Returns the length that this [Buff] was set to.
METHOD
i32 buff_set_len(Buff* self, i32 new_len);

/// Sets this [Buff]'s length field to be equal to its capacity field'
/// Returns the value of the new length
METHOD
i32 buff_grow_to_cap(Buff* self);

METHOD
PURE_FUNC
i32 buff_available(const Buff* self);

METHOD
u8* buff_end(Buff* self);

METHOD
PURE_FUNC
const u8* buff_cend(const Buff* self);

NORETURN
METHOD
HEDLEY_PRINTF_FORMAT(2, 3)
/// Used so we odnt have to include "log.h" in vec_index macro...
void buff_fatal_error(Buff* self, const char* fmt, ...);

#define buff_for_i(_self_, _index_name_) /* convienence macro for iterating over a vec. second parameter is just \
the loop index variable name. [vec_foreach] uses i' by default */                                                \
  for (int _index_name_ = 0; _index_name_ < buff_len((_self_)); _index_name_++)

#define buff_for(_self_) /* same as [vec_for_i] macro, but sets _index_name_ = i*/ buff_for_i(_self_, i)

#define buff_foreach_iter(_self_, _iter_name_) \
  for (__typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < buff_end((_self_)); _iter_name_++)

#define buff_foreach(_self_) buff_foreach_iter(_self_, iter)

#define buff_foreach_iter_const(_self_, _iter_name_) \
  for (const __typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < buff_end((_self_)); _iter_name_++)

#define buff_foreach_const(_self_) buff_foreach_iter_const(_self_, iter)

#define Vec(T) ptr(T)

#define vec_new(T, _init_capacity_, alloc) ((Vec(T))buff_new(sizeof(T) * (_init_capacity_), (alloc)))

#define vec_append(self) ((Vec(__typeof(*(self))))buff_append((Buff*)(self), mlayout_new(__typeof(*(self)))))

#define vec_push(SELF, VAL) do {\
  static_assert(sizeof(__typeof(*(SELF))) == sizeof(__typeof((VAL)))); \
  __typeof_unqual((VAL))* _elem = vec_append(SELF); \
  if (_elem) {\
      *_elem = (VAL); \
  }\
} while(0) 


#define vec_len(self) ((buff_len(pcast(u8, (self)))) / (i32)sizeof(__typeof(*(self))))
#define vec_capacity(self) ((buff_capacity(pcast(u8, (self)))) / (i32)(sizeof(__typeof(*(self)))))
#define vec_cend(self) ((const __typeof(self))(buff_cend(pcast(const Buff, (self)))))
#define vec_end(self) ((__typeof(self))(buff_end(pcast(Buff, (self)))))

#define vec_set_len(_self_, _new_len_) (buff_set_len((_self_), _new_len_ * sizeof(__typeof(*(_self_)))))

#define vec_grow_to_cap(_self_) (buff_grow_to_cap((Buff*)(_self_)))

#define vec_available(self) (buff_available(pcast(const Buff, (self))) / (i32)sizeof(__typeof(*(self))))

#define vec_needs_resize_for(self, T, n) \
  (buff_needs_space_for(pcast(const Buff, (self)), make(MemLayout, .size = sizeof(T) * (n), .align = alignof(T))))

#define vec_has_space_for(self, T, n) \
  (buff_has_space_for(pcast(const Buff, (self)), make(MemLayout, .size = sizeof(T) * (n), .align = alignof(T))))

#define vec_destroy(self, alloc) (buff_destroy(pcast(Buff, (self)), (alloc)))

#define vec_clear_zeroed(self) (buff_clear_zeroed(pcast(Buff, (self))))

#define vec_clear_zeroed_cap(_self_) (buff_clear_zeroed_cap(pcast(Buff, (_self_))))

#define vec_clear(self) (buff_clear(pcast(Buff, (self))))

#define vec_index(_self_, _i_)                                                                                 \
  ({                                                                                                           \
    const i32 _len_ = vec_len((_self_));                                                                       \
    const i32 _index_ = (_i_);                                                                                 \
    if (_index_ < 0 || _index_ >= _len_) {                                                                     \
      buff_fatal_error((Buff*)(_self_),                                                                        \
                       "Attempted to index vec of length: %d with an index that is out of bounds!: %d", _len_, \
                       _index_);                                                                               \
    }                                                                                                          \
    &(_self_)[_index_];                                                                                        \
  })

#define vec_write(self, out, out_len) \
  (buff_write(pcast(Buff, (self)), (out), (out_len) * (i32)sizeof(__typeof(*(self)))))
#define vec_resize(self, new_capacity, alloc) \
  ((__typeof(*(self))*)(buff_resize(pcast(Buff, (self)), (new_capacity) * (i32)sizeof(__typeof(*(self))), (alloc))))

#define vec_from_mem(start, end) (buff_from_mem(pcast(u8, (start)), pcast(u8, (end)))
#define vec_is_full(self) (buff_is_full(pcast(const Buff, (self))))

#define vec_is_empty(self) (buff_is_empty(pcast(const Buff, (self))))
#define vec_min_size buff_min_size

#define vec_for_i(_self_, _index_name_) /* convienence macro for iterating over a vec. second parameter is just \
the loop index variable name. [vec_foreach] uses i' by default */                                               \
  for (int _index_name_ = 0; _index_name_ < vec_len((_self_)); _index_name_++)

#define vec_for(_self_) /* same as [vec_for_i] macro, but sets _index_name_ = i*/ vec_for_i(_self_, i)

#define vec_foreach_iter(_self_, _iter_name_) \
  for (__typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define vec_foreach(_self_) vec_foreach_iter(_self_, iter)

#define vec_foreach_iter_const(_self_, _iter_name_) \
  for (const __typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define vec_foreach_const(_self_) vec_foreach_iter_const(_self_, iter)

/// A manually resizable String that may or may not be null terminated
typedef Vec(char) String;

static HEDLEY_ALWAYS_INLINE String string_new(i32 capacity, Allocator alloc) { return vec_new(char, capacity, alloc); }

METHOD
static HEDLEY_ALWAYS_INLINE sslice string_npush(String self, const char* string, i32 n) {
  return buff_append_nstr((Buff*)self, string, n);
}

METHOD
static HEDLEY_ALWAYS_INLINE sslice string_push(String self, const char* string) {
  return buff_append_str((Buff*)self, string);
}

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE i32 string_len(const String self) { return vec_len(self); }

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE i32 string_capacity(const String self) { return vec_capacity(self); }

METHOD
static HEDLEY_ALWAYS_INLINE char* string_end(String self) { return vec_end(self); }

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE const char* string_cend(const String self) { return vec_cend(self); }

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE i32 string_available(const String self) { return buff_available((const Buff*)self); }

METHOD
static HEDLEY_ALWAYS_INLINE void string_destroy(String self, Allocator alloc) { buff_destroy((Buff*)self, alloc); }

METHOD
static HEDLEY_ALWAYS_INLINE void string_clear_zeroed(String self) { buff_clear_zeroed((Buff*)self); }

METHOD
static HEDLEY_ALWAYS_INLINE void string_clear(String self) { (buff_clear(pcast(Buff, (self)))); }

METHOD
static HEDLEY_ALWAYS_INLINE String string_resize(String self, i32 new_capacity, Allocator alloc) {
  return (String)(buff_resize(pcast(Buff, (self)), (new_capacity), (alloc)));
}

static HEDLEY_ALWAYS_INLINE String string_from_mem(char* start, char* end) {
  return (String)buff_from_mem(pcast(u8, start), pcast(u8, end));
}

PURE_FUNC
METHOD
static HEDLEY_ALWAYS_INLINE bool string_is_full(const String self) { return vec_is_full(self); }

PURE_FUNC
METHOD
static HEDLEY_ALWAYS_INLINE bool string_is_empty(const String self) { return vec_is_empty(self); }

#define string_min_size buff_min_size

#define string_for_i(_self_, _index_name_) /* convienence macro for iterating over a vec. second parameter is just \
the loop index variable name. [vec_foreach] uses i' by default */                                                  \
  for (int _index_name_ = 0; _index_name_ < vec_len((_self_)); _index_name_++)

#define string_for(_self_) /* same as [vec_for_i] macro, but sets _index_name_ = i*/ vec_for_i(_self_, i)

#define string_foreach_iter(_self_, _iter_name_) \
  for (__typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define string_foreach(_self_) vec_foreach_iter(_self_, iter)

#define string_foreach_iter_const(_self_, _iter_name_) \
  for (const __typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define string_foreach_const(_self_) vec_foreach_iter_const(_self_, iter)
