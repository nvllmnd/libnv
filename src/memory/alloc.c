#include "memory/alloc.h"

#include <asm-generic/errno.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core_types.h"

// static void* global_vtable_alloc(void*, MemLayout layout) {  }

// static void* global_vtable_realloc(void*, void* ptr, MemLayout old, MemLayout newlayout) {
// }

// static void* global_vtable_zalloc(void*, MemLayout layout) {
// }


// static void global_vtable_free(void*, void* ptr) {
// }



// static const AllocVTable GLOBAL_ALLOC_VTABLE =
//     alloc_vtable_new(.allocate = global_vtable_alloc, .reallocate = global_vtable_realloc,
//                      .zallocate = global_vtable_zalloc,
//                      .free = global_vtable_free);

// const AllocVTable* global_allocator_vtable(void) { return &GLOBAL_ALLOC_VTABLE; }

// Allocator global_allocator(void) { return (Allocator){.ctx = nullptr, .vtable = global_allocator_vtable()}; }

Arena arena_new(isize capacity) {
  u8* mem = calloc(1, capacity);
  if (is_null(mem)) {
    // return zeroed Arena to indicate allocation/init error.
    return (Arena){};
  }

  return (Arena){
      .mem = mem,
      .used = 0,
      .capacity = capacity,
  };
}

Arena arena_new_in(isize capacity, Allocator alloc) {
  u8* mem = allocator_allocate(alloc, mlayout_bytes(capacity));
  if (is_null(mem)) {
    return (Arena){};
  }
  return (Arena){.mem = mem, .used = 0, .capacity = capacity};
}

static void* arena_vtable_alloc(void* ctx, MemLayout layout) {
  Arena* self = pcast(Arena, ctx);

  const isize next_used = (self->used + layout.size);
  if (next_used >= self->capacity) {
    return nullptr;
  }
  void* ptr = &self->mem[self->used];
  self->used = next_used;

  return ptr;
}

static void* arena_vtable_zalloc(void* ctx, MemLayout layout) {
  Arena* self = pcast(Arena, ctx);

  void* ptr = arena_allocate(self, layout);
  if (is_null(ptr)) {
    return nullptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}

void* arena_allocate(Arena* self, MemLayout layout) { return arena_vtable_alloc(pcast(void, self), layout); }

void* arena_zallocate(Arena* self, MemLayout layout) { return arena_vtable_zalloc(pcast(Arena, self), layout); }

void* vtable_alloc_no_impl(void*, MemLayout){ return NO_IMPL_METHOD_RESULT; }
void* vtable_realloc_no_impl(void*, void*, MemLayout, MemLayout) { return NO_IMPL_METHOD_RESULT; }
void* vtable_zalloc_no_impl(void*, MemLayout){ return NO_IMPL_METHOD_RESULT; }
void* vtable_expand_no_impl(void*, void*, MemLayout, MemLayout){ return NO_IMPL_METHOD_RESULT; }
void vtable_free_no_impl(void*, void*) {}




static const AllocVTable ARENA_ALLOC_VTABLE =
    alloc_vtable_new(.allocate = arena_vtable_alloc, .reallocate = NO_IMPL_REALLOCATE, .zallocate = arena_vtable_zalloc,
                      .free = NO_IMPL_FREE);

const AllocVTable* arena_alloc_vtable(void) { return &ARENA_ALLOC_VTABLE; }

void arena_destroy(Arena* self) {
  if (self->mem) {
    free(self->mem);

    *self = (Arena){};
  }
}

/// cleans up memory used by [Arena]. Must use the same [Allocator] that was used to create this [Arena]!
void arena_destroy_in(Arena* self, Allocator alloc) {
  if (self->mem) {
    allocator_free(alloc, self->mem);
    *self = (Arena){};
  }
}

void arena_clear(Arena* self) {
  self->used = 0;
}

void arena_clear_zeroed(Arena* self) {
  arena_clear(self);
  memset(self->mem, 0, self->capacity);
}



