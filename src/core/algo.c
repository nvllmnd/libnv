#include "core/algo.h"

#include <assert.h>

isize ptr_align_offset(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const u64ptr mask = align - 1;
  return cast(isize, cast(u64ptr, ptr) & mask);
  return 0;
}

bool ptr_is_aligned(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

 void* ptr_alignup( void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  return pcast(void, aligned);
}

u8* ptr_alignto(u8* ptr, u8* end, MemLayout layout) {
  assert(ptr);
  assert(end);
  assert(end >= ptr);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  u8* const top = ptr_alignup(ptr, layout.align);

  if (top + layout.size >= end) {
    return nullptr;
  }

  return top;

}

u8* ptr_alignin(u8* ptr, i32* space, MemLayout layout) {
  assert(ptr);
  assert(space);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);


  const i32 avail = *space;

  if (avail < layout.size) {
    return nullptr;
  }

  u8* const end = ptr + *space;

  u8* const aligned = ptr_alignto(ptr, end, layout);

  if (is_null(aligned)) {
    return nullptr;
  }

  const i32 delta = aligned - ptr;
  *space -= delta;
  
  return aligned;

}
