#include "nv/memory/alloc.h"

#include <assert.h>
#include <string.h>

#include "nv/core/log.h"
#include "nv/memory/layout.h"
#include "nv/memory/virt.h"

void* vtable_realloc_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, MemLayout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }

Arena arena_new(byte* begin, i64 size_bytes) {
  assert(begin);
  if UNLIKELY (is_null(begin) || size_bytes <= 0) {
    return ARENA_EMPTY;
  }
  return (Arena){
      .parent = nullptr, .marker = -1, .size = size_bytes, .begin = begin, .end = begin + size_bytes, .cursor = begin};
}

Arena arena_new_in(struct VirtMem* vm, i64 size_bytes) {
  if UNLIKELY (is_null(vm) || size_bytes <= 0) {
    return ARENA_EMPTY;
  }
  const VMarker marker = vmem_checkpoint(vm);
  byte* begin = vmem_allocate(vm, mlayout_bytes(size_bytes));
  if UNLIKELY (is_null(begin)) {
    return ARENA_EMPTY;
  }
  byte* end = begin + size_bytes;
  return (Arena){.parent = vm, .marker = marker, .size = size_bytes, .begin = begin, .end = end, .cursor = begin};
}

void* arena_allocate(Arena* self, MemLayout layout) {
  if UNLIKELY (is_null(self)) {
    LOG_FATAL("Attempted to call method function on a nullptr!");
  }
  const i64 avail = arena_available(self);
  if (layout.size > avail) {
    LOG_ERROR("Arena failed to allocate object of size: %li bytes, but only %li bytes available!", layout.size, avail);
    return nullptr;
  }
  byte* ptr = ptr_alignup(self->cursor, layout.align);
  byte* next = ptr + layout.size;
  if (next >= self->end) {
    LOG_ERROR(
        "Arena has enough space to fit object of size: %li bytes, however because its aligned to %li, the aligned "
        "address wont fit in this Arena! Need more space!",
        layout.size, layout.align);
    return nullptr;
  }

  self->cursor = next;
  self->last_alloc = ptr;
  return ptr;
}

void* arena_zallocate(Arena* self, MemLayout layout) {
  void* ptr = arena_allocate(self, layout);
  if UNLIKELY (is_null(ptr)) {
    return ptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}
void* arena_reallocate(Arena* self, void* ptr, MemLayout old, MemLayout nlayout) {
  assert(self);
  assert(ptr);
  if (nlayout.size == old.size) {
    return ptr;
  }

  if (self->last_alloc == ptr) {
    if (arena_expand(self, ptr, old, nlayout)) {
      return ptr;
    }

    if (nlayout.size < old.size) {
      const i64 delta = old.size - nlayout.size;
      self->cursor -= delta;
      return ptr;
    }
  }

  void* next = arena_allocate(self, nlayout);
  if (is_null(next)) {
    LOG_ERROR("Failed to allocate new memory for object reallocation of size %li bytes to %li bytes", old.size,
              nlayout.size);
    return ptr;
  }
  memcpy(next, ptr, old.size);

  self->last_alloc = next;
  return next;
}

bool arena_expand(Arena* self, void* ptr, MemLayout old, MemLayout nlayout) {
  assert(self);
  assert(ptr);

  if (self->last_alloc == ptr && nlayout.size >= old.size) {
    const i64 delta = nlayout.size - old.size;
    self->cursor += delta;
    return true;
  }

  return false;
}

Allocator arena_allocator(Arena* self) METHOD;

void arena_destroy(Arena* self) {
  if (self && arena_is_owned(self)) {
    vmem_reset_to(self->parent, self->marker);

  }
}
