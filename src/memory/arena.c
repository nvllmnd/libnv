#include "memory/arena.h"

#include <assert.h>
#include <string.h>

#include "algo.h"
#include "attributes.h"
#include "core_types.h"
#include "log.h"
#include "memory/alloc.h"
#include "memory/virt.h"

struct Block {
  struct Block* prev;
  i64 used;
  i64 capacity;
  u8 mem[];
};
typedef struct Block Block;

static inline Block* palloc_block_new(VirtMem* self, Block* current, isize size) {
  const isize capacity = sizeof(Block) + size;

  Block* block = vmem_allocate(self, mlayout_bytes(capacity));

  if (UNLIKELY(is_null(block))) {
    ELOG_DBG(
        "ArenaHeap::palloc_block_new Failed to allocate block from parent allocator! Parent allocator returned "
        "nullptr!");

    assert(block);
    return nullptr;
  }

  block->capacity = capacity;
  block->prev = current;
  return block;
}

struct Arena {
  /// indicates if this arena was created with an exclusive reference to its VirtMem field (ArenaHeap was created by
  /// calling arena_heap_new, as opposed to arena_heap_in_vmem)
  /// this allows us to better reuse memory, as if this was created by an outside VirtMem, use is most likely not
  /// interesting in clearing/resetting. If they do this is still fine, clearing/resetting zeros memory before setting
  /// chained block pointers to nullptr, so everything still works, though an exclusive ownership is most efficent use
  /// of space and time.
  bool is_exclusive;
  VirtMem* parent;

  ArenaStats stats;

  struct Block* root;

  void* last_alloc;

  // NOTE: We use these last fields as our inital size allocation,
  // then there is less pointer indirection, as i dont like the idea
  // of allocation this [ArenaHeap] struct if its only used for a couple pointers and some stat tracking

  i64 mem_used;
  i64 mem_cap;

  u8 mem[];
};

METHOD
static inline void* ah_try_inner_allocate(Arena* self, MemLayout layout) {
  const isize size = layout.size;
  const isize align = layout.align;
  const isize end = self->mem_used + size;
  if (end < self->mem_cap) {
    u8* start = &self->mem[self->mem_used];
    u8* aligned_start = ptr_alignup(start, align);

    const u8* alloc_end = aligned_start + size;

    const u8* mem_end = &self->mem[self->mem_cap - 1];
    if LIKELY (alloc_end < mem_end) {
      const isize alloc_size = (size + (aligned_start - start));
      self->mem_used += alloc_size;
      self->stats.total_used += alloc_size;
      return aligned_start;
    }
  }

  return nullptr;
}

METHOD
PURE_FUNC
static inline bool ah_alloc_in_block_ok(Arena* self, isize size, isize align) {
  const Block* root = self->root;
  if LIKELY (self->root) {
    const isize size_end = self->root->used + size;

    if (size_end >= self->root->capacity) {
      return false;
    }

    const u8* alloc_start = &root->mem[root->used];
    const u8* aligned_start = ptr_alignup((void*)alloc_start, align);

    const u8* alloc_end = aligned_start + size;

    const u8* mem_end = &root->mem[root->capacity - 1];

    if UNLIKELY (alloc_end >= mem_end) {
      return false;
    }

    return true;
  }
  return false;
}

/// This function checks if self->root is not null, however it does
/// not verify that the requested allocation will fit in this block, so be sure / to check that this block can fit an
/// allocation of @param (size) in bytes
METHOD
static inline void* ah_block_allocate(Arena* self, isize size, isize align) {
  if UNLIKELY (is_null(self->root)) {
    assert(false);
    return nullptr;
  }

  Block* root = self->root;
  u8* start = &root->mem[root->used];
  u8* aligned = ptr_alignup(start, align);

  const isize alloc_size = (size + (aligned - start));
  root->used += alloc_size;
  self->stats.total_used += alloc_size;
  return aligned;
}

METHOD
static inline void* ah_expand(Arena* self, void* ptr, MemLayout old_layout, MemLayout new_layout) {
  if (new_layout.size < old_layout.size) {
    return nullptr;
  }
  if (new_layout.size == old_layout.size) {
    return ptr;
  }

  if (self->last_alloc == ptr) {
    const u8* pend = pcast(const u8, ptr) + new_layout.size;
    const i32 delta = new_layout.size - old_layout.size;
    if (pend >= &self->mem[0] && pend <= &self->mem[self->mem_cap - 1]) {
     self->mem_used += delta; 
     return ptr;
    }
    if (self->root && pend >= &self->root->mem[0] && pend <= &self->root->mem[self->root->capacity - 1] ) {
      self->root->used += delta;
      return ptr;
    }

    // we cant expand in place. ): most likely because there is not enough room in the buffer this ptr lives at to expand any further
    return nullptr;
  }

  /// we can only expand when the requested memory range to expand was the last thing allocated by this allocator
  return nullptr;
}

METHOD
static inline void* ah_allocate(Arena* self, MemLayout layout) {
  // try to allocate from ArenaHeap's inner memory buffer first
  // this function returns a nullptr if its out of memory
  // or it is unable to fit this allocation
  if (self->mem_used + layout.size < self->mem_cap) {
    void* ptr = ah_try_inner_allocate(self, layout);
    if (is_not_null(ptr)) {
      self->last_alloc = ptr;
      return ptr;
    }
  }
  const isize size = layout.size;
  const isize align = layout.align;

  if (is_null(self->root) || !ah_alloc_in_block_ok(self, size, align)) {
    const isize block_size = size * 2;
    self->root = palloc_block_new(self->parent, self->root, block_size);
  }

  void* ptr = ah_block_allocate(self, size, align);
  if (is_not_null(ptr)) {
    self->last_alloc = ptr;
    return ptr;
  }
  return nullptr;
}

static void* arena_vtalloc_impl(void* ctx, MemLayout layout) {
  Arena* self = pcast(Arena, ctx);
  return arena_alloc(self, layout);
}

static void* arena_vtzalloc_impl(void* ctx, MemLayout layout) {
  Arena* self = pcast(Arena, ctx);
  return arena_zalloc(self, layout);
}

static void* arena_vtexpand_impl(void* ctx, void* ptr, MemLayout old_layout, MemLayout new_layout) {
  Arena* self = pcast(Arena, ctx);
  return ah_expand(self, ptr,  old_layout,  new_layout);

}

static inline Arena* ah_new_ex(VirtMem* vm, isize vmem_mb, isize capacity, bool exclusive) {
  if (is_null(vm)) {
    exclusive = true;

    if (vmem_init(&vm, vmem_mb) != OK) {
      LOG_DBG(
          "Call to vmem_init failed! Could not allocate virtual memory when attempting to create a new [ArenaHeap]!");
      return nullptr;
    }
  }

  const isize size = sizeof(Arena) + capacity;
  Arena* self = vmem_allocate(vm, mlayout_bytes(size));

  if UNLIKELY (is_null(self)) {
    assert(false);
    return nullptr;
  }

  *self = make(Arena, .is_exclusive = exclusive, .parent = vm,
               .stats = make(ArenaStats, .total_used = 0, .total_allocated = capacity), .root = nullptr, .mem_used = 0,
               .mem_cap = capacity);

  return self;
}

static void* arena_vtrealloc_impl(void* ctx, void* ptr, MemLayout old_layout, MemLayout new_layout) {
  if (old_layout.size == new_layout.size) {
   return ptr; 
  }

  Arena* self = pcast(Arena, ctx);
  if (new_layout.size < old_layout.size) {
    const u8* p = pcast(const u8, ptr);
    const i32 delta = old_layout.size - new_layout.size;
    if (p >= &self->mem[0] && p < &self->mem[self->mem_cap -1]) {
      self->mem_used -= delta;
      return ptr;
    }

    if (self->root && p >= &self->root->mem[0] && p <= &self->root->mem[self->root->capacity - 1]) {
      self->root->used -= delta;
      return ptr; 
    }

    // If we got to here, the pointer probably exists somewhere deeper in the linked list of blocks, and therefore most likley
    // not eligible for a resize, as "resizing" an Arena is the same as the inverse of expand (shrink) and if caller truely desires to "shrink" their allocated memory, then they can
    // simply decrement their capacity/length field that tracks the size of its span in memory
    return ptr;

  }
  if (self->last_alloc == ptr) {
    return ah_expand(self, ptr,  old_layout,  new_layout);
  }
  // otherwise we just make more space for new allocation and memcpy contents over to new memory location. This is an arena, so leaking
  // old data like this is not really undesireable, as all memory associated with this arena will be freed upon its destruction
  void* res =  ah_allocate(self, new_layout);
  if (is_not_null(res)) {
    memcpy(res, ptr, old_layout.size);
  }
  // we out of space/memory!
  return nullptr;
}

void arena_vtfree_impl(void*, void*) {}

static const AllocVTable HEAP_VTABLE =
    alloc_vtable_new(.allocate = arena_vtalloc_impl, .zallocate = arena_vtzalloc_impl, .free = arena_vtfree_impl,
                     .reallocate = arena_vtrealloc_impl, .expand = arena_vtexpand_impl, .mask = VT__Allocate | VT__Zallocate | VT__Free | VT__Expand);

Arena* arena_new(isize vmem_size_in_mb, isize init_capacity) {
  return ah_new_ex(nullptr, vmem_size_in_mb, init_capacity, true);
}

Arena* arena_in_vmem(VirtMem* vm, isize capacity, bool exclusive) {
  assert(vm);
  return ah_new_ex(vm, 0 /*not used since vm is non-null */, capacity, exclusive);
}

void* arena_alloc(Arena* self, MemLayout layout) { return ah_allocate(self, layout); }

void* arena_zalloc(Arena* self, MemLayout layout) {
  u8* mem = arena_alloc(self, layout);
  if UNLIKELY (is_null(mem)) {
    return nullptr;
  }
  memset(mem, 0, layout.size);
  return mem;
}

void arena_clear(Arena* self) {
  // fast path
  if (self->is_exclusive) {
    vmem_clear(self->parent);
    self->mem_used = 0;
    self->root = nullptr;
    return;
  }

  Block* current = self->root;

  while (current && current->prev) {
    Block* tmp = current;
    current = current->prev;
    // the backing VirtMem that this ArenaHeap allocates out of does not currently support
    // freeing beyond zeroing out memory, so we do that here,
    memset(tmp, 0, sizeof(Block) + tmp->capacity);
  }
  self->root = nullptr;
}

bool arena_destroy(Arena* self) {
  if (self->is_exclusive) {
    // cleans up everything
    vmem_destroy(self->parent);
    return true;
  }

  arena_clear(self);
  return false;
}

const AllocVTable* arena_alloc_vtable(void) { return &HEAP_VTABLE; }

Allocator arena_allocator(Arena* self) { return make(Allocator, .ctx = self, .vtable = &HEAP_VTABLE); }

ArenaStats arena_stats(Arena* self) { return self->stats; }
