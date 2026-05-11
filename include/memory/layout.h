#pragma once


/// Memory Layout, used for determining size and alignment of Allocator allocations
#include "attributes.h"
#include "core_types.h"
#include "intdefs.h"
struct MemLayout {
  /// Size of requested allocation in bytes. must be a multiple of alignment
  i32 size;
  /// Alignment of requested allocation. must be a multiple of 2
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

#define mlayout_fma(THeader, flex_member_size) (make(MemLayout, .size = sizeof(THeader) + (flex_member_size), .align = alignof(Block)))

CONST_FUNC
static inline MemLayout mlayout_bytes(isize nbytes) {
  return make(MemLayout, .size = nbytes, .align = alignof(u8[nbytes]));
}
