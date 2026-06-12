// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


/// Memory Layout, used for determining size and alignment of Allocator allocations
#include <assert.h>
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core/core_types.h"

/// Simple struct used for sizing memory allocations, inspired from Rust's Layout type
struct Layout {
  /// Size of requested allocation in bytes. Must be non-negative and greater than 0
  isize size;
  /// Alignment of requested allocation. must be a multiple of 2 (or the value 1)
  isize align;
};
typedef struct Layout Layout;

#define mlayout_static(s, a)                          \
  ({                                                  \
    constexpr const __typeof(s) _s = (s);             \
    constexpr const __typeof(a) _a = (a);             \
    static_assert(IS_POWER_OF_2(_a) && _s % _a == 0); \
    make(Layout, .size = _s, .align = _a);       \
  })

#define mlayout_new(T) (mlayout_static(sizeof(T), alignof(T)))
#define mlayout_array(T, N) (mlayout_static(sizeof(T) * N, alignof(T)))
#define mlayout_vec(T, _n) (make(Layout, .size = sizeof(T) * (_n), .align = alignof(T)))
#define mlayout_fma(THeader, flex_member_size) (make(Layout, .size = sizeof(THeader) + (flex_member_size), .align = alignof(THeader)))

/// @brief Creates a new [Layout] appropriate for allocating a buffer of bytes of size `nbytes`
/// @param(i32 nbytes) size in bytes of allocation request. Must be > 0
CONST_FUNC
static inline Layout mlayout_bytes(i32 nbytes) {
  assert(nbytes > 0);
  return make(Layout, .size = nbytes, .align = 1);
}

/// @brief exteneds Layout by count. (if MemLayout represents a single element of a typed array, then MemLayout * count is the MemLayout of that typed array)
CONST_FUNC
static inline Layout mlayout_extend(Layout self, i32 count) {
  assert(count > 0);
  return (Layout){.size = self.size * count, .align = self.align};
}

/// @brief Creates a new MemLayout calculated as such: multiplies self.size * count and adds the rhs.size to the result, takes max of self and rhs alignment
CONST_FUNC
static inline Layout mlayout_extend_with(Layout self, i32 count, Layout rhs) {
  assert(count > 0);
  return (Layout){.size = (self.size * count) + rhs.size, .align = max(self.align, rhs.align)};
}

/// @brief lhs.size + rhs.size, align = max(lhs.align, rhs.align)
CONST_FUNC
static inline Layout mlayout_add(Layout lhs, Layout rhs) {
  return (Layout){.size = lhs.size + rhs.size, .align = max(lhs.align, rhs.align)};
}
