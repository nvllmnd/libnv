#include "nv/memory/heap.h"

#include <assert.h>
#include <stddef.h>
#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/debug.h"
#include "nv/core/log.h"
#include "nv/iter/iterators.h"
#include "nv/memory/alloc.h"
#include "nv/memory/vmem.h"

struct Block {
  // alignas(4) union {
  struct {
    u16 size;
    /// @brief Size Class index that this allocation belongs to
    u16 size_class_slot;
  };
  // u32 _pad;
  // };

  ATTR_COUNTED_BY(size)
  byte data[];
};
alias(Block);

#define asblock(bl) (&((Block*)(bl))[-1])
/// Min allocation size for requested allocations. The rational for this is that allocations smaller than the size
// of
/// each block's header is wasteful, so we either can enforce callers to only request allocations larger than
/// sizeof(Block), or we simple round up to 24 for allocation requests smaller than that, then at least those blocks
// can
/// be later reused by more allocations (smaller allocations have a less chance of being reused after free.)
// static constexpr const isize BA_MIN_ALLOC_SIZE = sizeof(Block);

#define bl_begin(b) (&((b)->data[0]))

static constexpr const u32 SIZE_CLASS_LIST_LEN = 200;

/// @brief we use u16s to save space, plus the largest size class is only 8k, which fits nicely inside a u16
struct SizeClass {
  u16 len;
  /// Size in bytes of this size class. Blocks of size <= to this size class are put into this size classes' free list
  /// ring buffer
  u16 class_size;

  /// @brief begin and end  pointers for ringbuffer list so that we can
  /// pop from the front of it, so that we always pop off the block that has been sitting in this free list the longest
  u32* first;
  u32* last;

  /// @brief byte offsets to where freed block sits in this heap
  /// @details offset if from the start of heap
  u32 list[SIZE_CLASS_LIST_LEN];
};
alias(SizeClass);

static constexpr const i32 SIZE_CLASSES_LEN = 10;

static constexpr const i32 SIZE_CLASS_MIN_SIZE = 24;

[[maybe_unused]]
static constexpr const i32 SIZE_CLASS_SIZES[SIZE_CLASSES_LEN] = {
    SIZE_CLASS_MIN_SIZE, 1 << 5, 1 << 6, 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14};

static constexpr const i32 FREE_LIST_MAX_SIZE = SIZE_CLASS_SIZES[SIZE_CLASSES_LEN - 1];

struct FreeList {
  i32 blocks_free;
  SizeClass sclasses[SIZE_CLASSES_LEN];
};
alias(FreeList);

struct Heap {
  VMem* vm;
  IterByte iter;

  FreeList freelist;
};

/// @brief returns size class index of given block size
static isize size_class_of(const FreeList* self, usize block_size);

METHOD
RETURNS_NON_NULL
PURE_FUNC
static inline const u32* size_class_end(SizeClass* self) { return &self->list[SIZE_CLASS_LIST_LEN]; }

PARAMS_NONNULL(1, 2)
RETURNS_NON_NULL
static inline Block* pop_size_class(Heap* self, SizeClass* sc) {
  // we are full!
  if UNLIKELY (sc->first == sc->last) {
    // we are just going to reset to beginnng regardless of where these 2 poitners are pointing
    sc->first = &sc->list[0];
    sc->last = sc->first + 1;
  }
  const u32 offset = *sc->first;
  sc->first++;

  Block* block = (Block*)(vmem_begin(self->vm) + sizeof(Heap)) + offset;
  if UNLIKELY ((byte*)block >= heap_end(self)) {
    LOG_FATAL("Offset: %d caused block calculation to be out of bounds! this is a bug!", offset);
  }
  return block;
}

PARAMS_NONNULL(1, 2)
static inline void push_size_class(Heap* self, Block* freed) {
  SizeClass* sc = &self->freelist.sclasses[freed->size_class_slot];

  const intptr_t offset = (byte*)freed - heap_begin(self);
  expectm(offset >= 0, "calculation of freed block offset resulted in a negative number. This is a bug!!");
  const u32 slot = offset;

  // we have hit the end, so we wrap around
  if UNLIKELY (sc->last >= size_class_end(sc)) {
    sc->last = &sc->list[0];
  }
  // we are completely full, warn caller and advance last +2 beyond first (wrapping if at end) to attempt to leak as
  // little memory as possible
  if UNLIKELY (sc->last == sc->first) {
    // wrap around to beginning if we pass end
    sc->last = clamp(sc->last + 2, &sc->list[0], &sc->list[SIZE_CLASS_LIST_LEN]);
  }

  if (*sc->last != 0) {
    LOG_WARN("pushing into size class list that is full! Freed block at offset: %li will leak!", *sc->last);
  }

  *sc->last = slot;
  sc->last++;
}

static Block* next_free_block(Heap* self, usize block_size) METHOD;

isize heap_avail_ptr_size(const Heap* self, const void* ptr) {
  assert(self);
  if UNLIKELY (is_null(ptr)) {
    DERROR("Attempted to find available block memory from a nullptr!");
    return -1;
  }

  if (!heap_contains(self, ptr)) {
    DERROR(
        "Attempted to check avail block size with pointer that does not belong in this heap! call this function with "
        "the correct heap!");
    return -1;
  }

  const Block* block = asblock(ptr);
  // validate this poitner belongs to some heap and its metadata has not been corrupted
  expect(block->size_class_slot < SIZE_CLASSES_LEN && block->size <= FREE_LIST_MAX_SIZE);

  const isize size = block->size;
  const isize block_size = SIZE_CLASS_SIZES[block->size_class_slot];
  return block_size - size - sizeof(Block);
}

usize heap_size_of(const Heap* self, const void* ptr) {
  assert(self);
  assert(ptr);

  Block* b = asblock(ptr);
  return b->size;
}

usize heap_full_size_of(const Heap* self, const void* ptr) { return heap_size_of(self, ptr) + sizeof(Block); }

isize heap_min_size(void) { return sizeof(Heap); }

isize heap_good_size(void) { return heap_min_size() * 3; }

Heap* heap_new(const isize vmem_size) {
  const isize size = max(heap_good_size(), vmem_size);
  VMem* vm = vmem_new(size);
  if UNLIKELY (is_null(vm)) {
    DERROR("Failed to create VMem of size %li bytes!", size);
    return nullptr;
  }

  return heap_from_vmem(vm);
}

Heap* heap_from_vmem(VMem* vm) {
  IterByte iter = {.begin = vmem_begin(vm), .cursor = vmem_begin(vm), .end = vmem_end(vm)};

  Heap* self = allocate_raw(&iter, mlayout_new(Heap));
  if (is_null(self)) {
    DERROR(
        "Could not allocate inner Heap type with vmem of size: %li bytes! minimum size is %li bytes and suggested "
        "minimum size is: %li",
        vmem_size(vm), heap_min_size(), heap_good_size());
    return nullptr;
  }

  self->vm = vm;
  self->iter = iter;
  self->freelist.blocks_free = 0;

  SizeClass* sc = &self->freelist.sclasses[0];

  for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
    sc[i].class_size = SIZE_CLASS_SIZES[i];
    sc[i].len = 0;
  }

  return self;
}

void* heap_alloc(Heap* self, isize size, isize align) {
  if (size > FREE_LIST_MAX_SIZE) {
    LOG_WARN("Requested allocation size: %li is larger than max allocation size of %li (%dKB). Rejected", size,
             FREE_LIST_MAX_SIZE, OF_KB(FREE_LIST_MAX_SIZE));
    return nullptr;
  }

  Block* block = next_free_block(self, size);
  if (is_not_null(block)) {
    // next_free_block fills out metadata for new block for us so we can just return here
    return &block->data[0];
  }

  block = allocate_raw(&self->iter, (Layout){.size = size, .align = align});
  if UNLIKELY (is_null(block)) {
    DERROR("Failed to allocate object of size %li bytes. heap only has %li bytes available!", size,
           iter_tail(self->iter));
    return nullptr;
  }

  // NOTE: We have to scan size class list again here, but since its literally 10 items (and i dont plan on ever making
  // it larger than 10, unless i go the dynamic size class list route) this is not something we should worry about
  block->size_class_slot = size_class_of(&self->freelist, size);
  block->size = size;

  return &block->data[0];
}

void* heap_zalloc(Heap* self, isize size, isize align) {
  void* ptr = heap_alloc(self, size, align);
  if (is_null(ptr)) {
    return ptr;
  }
  memset(ptr, 0, size);
  return ptr;
}

void* heap_realloc(Heap* self, void* ptr, const isize new_size, const isize align) {
  assert(self);
  assert(IS_POWER_OF_2(align));

  if UNLIKELY (is_null(ptr)) {
    DERROR("Attempted reallocation of nullptr! if you want to allcoate memory in this heap use heap_alloc!");
    return nullptr;
  }

  const usize old_size = heap_size_of(self, ptr);

  const Layout old_layout = (Layout){.size = old_size, .align = align};
  const Layout new_layout = (Layout){.size = new_size, .align = align};

  return heap_reallocate(self, ptr, old_layout, new_layout);
}

void* heap_reallocate(Heap* self, void* ptr, Layout old, Layout new) {
  assert(self);

  if (!heap_contains(self, ptr)) {
    DERROR(
        "Attempted to reallocate memory not owned by this heap! (or given ptr is null!) call with appropriate heap!");
    return nullptr;
  }

  if (new.size == old.size) {
    return ptr;
  }
  Block* b = asblock(ptr);
  if (new.size < old.size) {
    b->size = new.size;
    return ptr;
  }

  const isize avail = heap_avail_ptr_size(self, ptr);

  const isize delta = new.size - old.size;
  // we can resize in place! this avoids having to search for free blocks
  if (delta <= avail) {
    b->size = new.size;
    return ptr;
  }

  Block* new_block = heap_alloc(self, new.size, new.align);
  if UNLIKELY (is_null(new_block)) {
    DERROR("Heap has no free space and no freed blocks are large enough for allocation of size: %li!", new.size);
    return nullptr;
  }

  mempcpy(&new_block->data[0], &b->data[0], b->size);

  heap_free(self, b);
  return &new_block->data[0];
}

bool heap_resize(Heap* self, void* ptr, Layout old, Layout new) METHOD;

void heap_free(Heap* self, void* ptr) {
  assert(self);

  if (is_null(ptr)) {
    return;
  }

  if (!heap_contains(self, ptr)) {
    LOG_WARN("Attempted to free pointer not owned by given Heap!");
    return;
  }

  Block* b = asblock(ptr);

  push_size_class(self, b);
  self->freelist.blocks_free += 1;
}

void heap_destroy(Heap* self) {
  if (is_not_null(self) && is_not_null(self->vm)) {
    // NOTE: Heap header is allocated at the start of its owning VMem, so
    // everything in Heap is cleaned up after vmem_destroy returns. So dont access any of its fields! doing so will
    // trigger Segfault!
    vmem_destroy(self->vm);
  }
}

const byte* heap_begin(const Heap* self) { return vmem_begin(self->vm) + sizeof(Heap); }

const byte* heap_end(const Heap* self) { return vmem_end(self->vm); }

bool heap_contains(const Heap* self, const void* ptr) {
  assert(self);

  const byte* p = ptr;

  return p >= heap_begin(self) && p < heap_end(self);
}

usize heap_max_alloc_size(void) { return FREE_LIST_MAX_SIZE; }

static Block* next_free_block(Heap* self, usize block_size) {
  // TODO: Try out delaying searching through free blocks only when load factor is above a pre-determined threshold
  if (self->freelist.blocks_free <= 0 || block_size > FREE_LIST_MAX_SIZE) {
    // we dont have any free blocks, or block size is not supported
    return nullptr;
  }

  for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
    SizeClass* sc = &self->freelist.sclasses[i];
    const usize class_size = sc->class_size;
    if (class_size >= block_size) {
      Block* block = pop_size_class(self, sc);
      block->size_class_slot = i;
      block->size = block_size;
      self->freelist.blocks_free -= 1;
      return block;
    }
  }

  // NOTE: We should never get to this point.
  // TODO: Check that this actually is the case, and if so add an UNREACHABLE()
  return nullptr;
}

static isize size_class_of(const FreeList* self, const usize block_size) {
  for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
    const SizeClass* sc = &self->sclasses[i];
    if (sc->class_size >= block_size) {
      return i;
    }
  }
  return -1;
}
