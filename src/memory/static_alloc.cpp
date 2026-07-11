// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/memory/static_alloc.h"

#include <assert.h>
#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"

/// @brief Header used inside
struct StaticAlloc {
  u8* top;
  u8* end;
  void* last_alloc;
  u8 storage[];
};

/// Size of statically atlocated storage in bytes
METHOD
PURE_FUNC
[[maybe_unused]]
static inline i32 salloc_size(const StaticAlloc* self) {
  assert(self);
  // 'self' is pointing to the start of this statically sized buffer's memory, so subtracting from 'self' gives the full
  // size of the static storage in bytes (So for a StaticAlloc4k, this funciton should return 4096)
  const u8* start = pcast(const u8, self);
  return self->end - start;
}

void salloc_init_(StaticAlloc* self, i32 size_bytes) {
  assert(self);
  assert(size_bytes > (i32)sizeof(StaticAlloc));

  self->top = &self->storage[0];
  // size_bytes - sizeof(StaticAlloc), as size_bytes is the full size of static storage for this allocator
  self->end = self->top + (size_bytes - sizeof(StaticAlloc));

  assert(salloc_size(self) == size_bytes);
}

i32 salloc_used(const StaticAlloc* self) {
  assert(self);
  assert(self->top);
  return self->top - (&self->storage[0]);
}

i32 salloc_metadata_size(void) {
  static constexpr const i32 SIZE = sizeof(StaticAlloc);
  return SIZE;
}

void* salloc_allocate(StaticAlloc* self, Layout layout) {
  assert(self);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  u8* aligned = ptr_alignto(self->top, self->end, layout);

  if (is_null(aligned)) {
    LOG_DBG("StaticAllocator of size %d bytes does not have enough available space for object of size: %li!",
            salloc_size(self), layout.size);
    return nullptr;
  }

  self->top = aligned + layout.size;

  self->last_alloc = aligned;

  return aligned;
}

// static VTABLE_ADAPTER_ALLOC(StaticAlloc, salloc_allocate)

void* salloc_zallocate(StaticAlloc* self, Layout layout) {
  void* ptr = salloc_allocate(self, layout);
  if (is_null(ptr)) {
    return nullptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}

void* salloc_expand(StaticAlloc* self, void* ptr, Layout old_layout, Layout new_layout) {
  assert(self);
  assert(ptr);
  assert(IS_POWER_OF_2(new_layout.align));
  assert(old_layout.align == new_layout.align);

  if (self->last_alloc != ptr || new_layout.size <= old_layout.size) {
    return nullptr;
  }

  const i32 delta = new_layout.size - old_layout.size;
  self->top += delta;

  return ptr;
}

void* salloc_reallocate(StaticAlloc* self, void* ptr, Layout old_layout, Layout new_layout) {
  if (new_layout.size == old_layout.size) {
    return ptr;
  }
  if (new_layout.size < old_layout.size) {
    if (self->last_alloc == ptr) {
      const i32 delta = old_layout.size - new_layout.size;
      self->top -= delta;
    }

    return ptr;
  }

  if (self->last_alloc == ptr) {
    return salloc_expand(self, ptr, old_layout, new_layout);
  }

  void* next = salloc_allocate(self, new_layout);
  if UNLIKELY (is_null(next)) {
    LOG_DBG("Ran out of available space in StaticAlloc while trying to reallocate memory of size %li bytes to %li bytes!",
            old_layout.size, new_layout.size);
    return ptr;
  }

  memcpy(next, ptr, old_layout.size);
  return next;
}

char* salloc_strndup(StaticAlloc* self, const char* string, i32 n) {
  assert(self);
  assert(string);
  assert(n > 0);

  n += 1;
  char* res = salloc_allocate(self, mlayout_bytes(n));
  if UNLIKELY (is_null(res)) {
    LOG_DBG("StaticAlloc did not have enough available space to dup string: %.*s of len: %d", n, string, n);
    return nullptr;
  }

  strncpy(res, string, n - 1);
  res[n] = '\0';

  return res;
}
