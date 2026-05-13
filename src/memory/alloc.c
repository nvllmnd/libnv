#include "memory/alloc.h"

#include <asm-generic/errno.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core_types.h"

typedef FixedBuffAlloc FBA;

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

FixedBuffAlloc fba_new(isize capacity) {
  u8* mem = calloc(1, capacity);
  if (is_null(mem)) {
    // return zeroed Arena to indicate allocation/init error.
    return (FBA){};
  }

  return (FBA){
      .mem = mem,
      .used = 0,
      .capacity = capacity,
  };
}

FBA fba_new_in(isize capacity, Allocator alloc) {
  u8* mem = allocator_allocate(alloc, mlayout_bytes(capacity));
  if (is_null(mem)) {
    return (FBA){};
  }
  return (FBA){.mem = mem, .used = 0, .capacity = capacity};
}

static void* arena_vtable_alloc(void* ctx, MemLayout layout) {
  FBA* self = pcast(FBA, ctx);

  const isize next_used = (self->used + layout.size);
  if (next_used >= self->capacity) {
    return nullptr;
  }
  void* ptr = &self->mem[self->used];
  self->used = next_used;

  return ptr;
}

static void* arena_vtable_zalloc(void* ctx, MemLayout layout) {
  FBA* self = pcast(FBA, ctx);

  void* ptr = fba_allocate(self, layout);
  if (is_null(ptr)) {
    return nullptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}

void* fba_allocate(FBA* self, MemLayout layout) { return arena_vtable_alloc(pcast(void, self), layout); }

void* fba_zallocate(FBA* self, MemLayout layout) { return arena_vtable_zalloc(pcast(FBA, self), layout); }

// void* vtable_alloc_no_impl(void*, MemLayout){ return NO_IMPL_METHOD_RESULT; }
void* vtable_realloc_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, MemLayout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }
// void vtable_free_no_impl(void*, void*) {}

void arena_vtable_free(void*, void*) {}

static const AllocVTable ARENA_ALLOC_VTABLE =
    alloc_vtable_new(.allocate = arena_vtable_alloc, .reallocate = NO_IMPL_REALLOCATE, .zallocate = arena_vtable_zalloc,
                     .free = arena_vtable_free, .mask = VT__Allocate | VT__Zallocate | VT__Free);

const AllocVTable* fba_alloc_vtable(void) { return &ARENA_ALLOC_VTABLE; }

void fba_destroy(FBA* self) {
  if (self->mem) {
    free(self->mem);

    *self = (FBA){};
  }
}

/// cleans up memory used by [Arena]. Must use the same [Allocator] that was used to create this [Arena]!
void fba_destroy_in(FBA* self, Allocator alloc) {
  if (self->mem) {
    allocator_free(alloc, self->mem);
    *self = (FBA){};
  }
}

void fba_clear(FBA* self) { self->used = 0; }

void fba_clear_zeroed(FBA* self) {
  fba_clear(self);
  memset(self->mem, 0, self->capacity);
}
