#pragma once

#include <stddef.h>

#include "attributes.h"
#include "intdefs.h"
#include "memory/error.h"
#include "memory/layout.h"
#define IS_POWER_OF_2(n) ((n & (n - 1)) == 0)

CONST_FUNC
static inline bool is_power_of_2(isize n) { return IS_POWER_OF_2(n); }

PARAMS_NONNULL(1)
isize ptr_align_offset(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));
// {
//   if LIKELY (IS_POWER_OF_2(align)) {
//     const u64ptr mask = align - 1;
//     return cast(isize, cast(u64ptr, ptr) & mask);
//   }
//   return 0;
// }

/// Checks if a given pointer is aligned to given alignment.
/// @param (align) MUST BE A POWER OF 2. If it is not this funciton returns
/// false
PURE_FUNC
PARAMS_NONNULL(1)
bool ptr_is_aligned(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));
// {
//   const auto addr = cast(uintptr_t, ptr);
//   const uintptr_t mask = align - 1;
//   return (addr & mask) == 0;
// }

/// Aligns pointer up to given alignment, or returns the same pointer if it
/// already is aligned
/// @param (align) MUST BE A POWER OF 2.  If it is not, then this function
/// returns the exact same pointer, doing no calulations and possiby causing
/// confusion if given pointer is misaligned
PARAMS_NONNULL(1)
RETURNS_NON_NULL
PURE_FUNC
 void* ptr_alignup(void* ptr, isize align) WHERE(IS_POWER_OF_2(align));
// {
//   if (ptr_is_aligned(ptr, align)) {
//     return ptr;
//   }

//   const uintptr_t addr = cast(uintptr_t, ptr);
//   const uintptr_t mask = align - 1;

//   const uintptr_t aligned = (addr + mask) & (~mask);

//   return pcast(void, aligned);
// }

// {
//   if (!IS_POWER_OF_2(align)) {
//     return ApiError__ParameterValueNotPowerOf2;
//   }
//   if (is_null(ptr_out)) {
//     return ApiError__NullParameter;
//   }

//   const void* ptr = *ptr_out;

//   const uintptr_t addr = cast(uintptr_t, ptr);
//   const uintptr_t mask = align - 1;

//   const uintptr_t aligned = (addr + mask) & (~mask);
//   *ptr_out = cast(void*, aligned);
//   return OK;
// }

PARAMS_NONNULL(1, 2)
PURE_FUNC
u8* ptr_alignto(u8* ptr, u8* end, MemLayout layout) WHERE(IS_POWER_OF_2(layout.align) && end >= ptr);

/// behaves similarly to C++'s std::align
PARAMS_NONNULL(1, 2)
u8* ptr_alignin(u8* ptr, i32* space, MemLayout layout);

// {
//   assert(top);
//   assert(end);
//   u8* top = align_ptr(self->top, layout.align);kkk

//   if (top + layout.size >= self->end) {
//     return nullptr;
//   }

//   return top;

// }

PARAMS_NONNULL(1)
static inline void* move(void** from) {
  void* tmp = *from;
  *from = nullptr;
  return tmp;
}
#define move(from) (move((void**)&from))

PARAMS_NONNULL(1, 2)
static inline void* move_into(void** from, void** to) {
  *to = move(*from);
  return *to;
}
#define move_into(from, to) (move_into((void**)&from, (void**)&to))

PARAMS_NONNULL(1, 2)
static inline void* move_exchange(void** obj, void** new_value) {
  void* tmp = *obj;
  *obj = *new_value;
  return tmp;
}
#define move_exchange(from, to) (move_exchange((void**)&from, (void**)&to))
