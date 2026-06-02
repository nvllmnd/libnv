#pragma once

/// A Buffer of bytes. Must be manually resized. If created with an allocator ([buff_new]), then it must be resized and
/// destroyed with the same allocator. This is to save metadata space, plus this is how its done in Zig so i think im
/// going to try it out finally  lol A Buff can also be created from caller memory ([buff_from_mem]), and if it is, the
/// memory it uses is expected to be managed by the caller, and as such, should not be passed to [buff_destroy] (or if
/// it is, make sure its a no-op allocator)
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

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

Buff* buff_sized_new(i32 elem_size, i32 capacity, Allocator alloc);

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
NvError buff_putchar(Buff* self, char c);

METHOD
NvError buff_putbyte(Buff* self, u8 b);

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

/// Does this buffer have an element size > 1?
/// Used for Vec
METHOD
PURE_FUNC
bool buff_is_sized(const Buff* self);

METHOD
PURE_FUNC
i32 buff_elem_size(const Buff* self);

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
