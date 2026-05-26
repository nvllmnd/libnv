#pragma once


/// Memory Layout, used for determining size and alignment of Allocator allocations
#include <assert.h>
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core_types.h"

/// Simple struct used for sizing memory allocations, inspired from Rust's Layout type
struct MemLayout {
  /// Size of requested allocation in bytes. Must be non-negative and greater than 0
  i32 size;
  /// Alignment of requested allocation. must be a multiple of 2 (or the value 1)
  i32 align;
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
