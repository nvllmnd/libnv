#pragma once

#include <stdarg.h>
#include "nv/core/attributes.h"
#include "nv/iter/buff.h"
#include "nv/iter/vec.h"



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
static HEDLEY_ALWAYS_INLINE char string_putchar(String self, char c) {
  return buff_putchar((Buff*)self,  c);
}

METHOD
static HEDLEY_ALWAYS_INLINE char string_putbyte(String self, u8 b) {
  return buff_putbyte((Buff*)self,  b);
}

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE i32 string_len(const String self) { return vec_len(self); }

METHOD
PURE_FUNC
static  HEDLEY_ALWAYS_INLINE sslice string_slice(const String self) {
  const i32 len = string_len(self); 
  return sslice_new(.begin = self, .len = len);
}

METHOD
PURE_FUNC
static HEDLEY_ALWAYS_INLINE sslice string_subslice(const String self, i32 from, i32 to) {
  const i32 len = string_len(self);
  const i32 in_len = to - from;
  if LIKELY (in_len >= 0 && in_len < len) {
    return sslice_from_range(self,  from,  to);
  }
  return sslice_empty();
}


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

HEDLEY_PRINTF_FORMAT(3, 4)
METHOD
sslice string_fpush(String self, char terminal, const char* fmt, ...);


sslice string_vfpush(String self, char terminal, const char* fmt, va_list args);

HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice string_fpush_nl(String self, const char* fmt, ...);

METHOD
HEDLEY_PRINTF_FORMAT(2, 3)
sslice string_fpush_null(String self, const char* fmt, ...);

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

