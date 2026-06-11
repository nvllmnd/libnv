#pragma once


/// Memory Layout, used for determining size and alignment of Allocator allocations
#include <assert.h>
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core_types.h"

/// Simple struct used for sizing memory allocations, inspired from Rust's Layout type
struct MemLayout {
  /// Size of requested allocation in bytes. Must be non-negative and greater than 0
  isize size;
  /// Alignment of requested allocation. must be a multiple of 2 (or the value 1)
  isize align;
};
typedef struct MemLayout MemLayout;

#define mlayout_static(s, a)                          \
  ({                                                  \
    constexpr const __typeof(s) _s = (s);             \
    constexpr const __typeof(a) _a = (a);             \
    static_assert(IS_POWER_OF_2(_a) && _s % _a == 0); \
    make(MemLayout, .size = _s, .align = _a);       \
  })

#define mlayout_new(T) (mlayout_static(sizeof(T), alignof(T)))
#define mlayout_array(T, N) (mlayout_static(sizeof(T) * N, alignof(T)))
#define mlayout_vec(T, _n) (make(MemLayout, .size = sizeof(T) * (_n), .align = alignof(T)))
#define mlayout_fma(THeader, flex_member_size) (make(MemLayout, .size = sizeof(THeader) + (flex_member_size), .align = alignof(THeader)))

/// @brief Creates a new [MemLayout] appropriate for allocating a buffer of bytes of size `nbytes`
/// @param(i32 nbytes) size in bytes of allocation request. Must be > 0
CONST_FUNC
static inline MemLayout mlayout_bytes(i32 nbytes) {
  assert(nbytes > 0);
  return make(MemLayout, .size = nbytes, .align = 1);
}

/// @brief exteneds MemLayout by count. (if MemLayout represents a single element of a typed array, then MemLayout * count is the MemLayout of that typed array)
CONST_FUNC
static inline MemLayout mlayout_extend(MemLayout self, i32 count) {
  assert(count > 0);
  return (MemLayout){.size = self.size * count, .align = self.align};
}

/// @brief Creates a new MemLayout calculated as such: multiplies self.size * count and adds the rhs.size to the result, takes max of self and rhs alignment
CONST_FUNC
static inline MemLayout mlayout_extend_with(MemLayout self, i32 count, MemLayout rhs) {
  assert(count > 0);
  return (MemLayout){.size = (self.size * count) + rhs.size, .align = max(self.align, rhs.align)};
}

/// @brief lhs.size + rhs.size, align = max(lhs.align, rhs.align)
CONST_FUNC
static inline MemLayout mlayout_add(MemLayout lhs, MemLayout rhs) {
  return (MemLayout){.size = lhs.size + rhs.size, .align = max(lhs.align, rhs.align)};
}
