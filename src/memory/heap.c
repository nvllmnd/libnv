

#include "nv/memory/heap.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/iter/iterators.h"
#include "nv/memory/alloc.h"

struct Block {
  // NOTE: Im not making this a discriminated union, as which field to read and write to is apparent from its
  // useage/location. (if you are reading a block in the free-list, read next, otherwise size lol)
  // this is to save allocation header
  union {
    /// @brief used to maintain position in size class free list
    struct Block* next;

    /// @brief used while active
    struct {
      i32 val;
      i32 class_slot;
    } size;
  };

  /// @brief Size Class index that this allocation belongs to,
  /// indicies larger than [SIZE+CLASSES_LEN] all fall into the same 'spillover' bucket for big allocations
  // u8 size_class_slot;

  byte data[];
};
alias(Block);

#define asblock(bl) (&((Block*)(bl))[-1])

#define bl_begin(b) (&((b)->data[0]))

struct FreeList {
  i32 count;
  Block* first;
  Block* last;
};
alias(FreeList);

struct SizeClass {
  /// @brief Size in bytes of this size class.
  /// @details Blocks of size <= to this size class are put into this size classes' free list
  /// when this value is negative, it indicates any size (all allocation sizes can be put into this bucket!)
  /// as such you should traverse the whole  list to find a block of appropriate size
  i32 class_size;

  FreeList list;
};
alias(SizeClass);

static constexpr const u32 SIZE_CLASSES_LEN = 18U;

static constexpr const i32 SIZE_CLASS_MIN_SIZE = sizeof(Block);

static constexpr const u32 SIZE_CLASS_SIZES[SIZE_CLASSES_LEN] = {
    SIZE_CLASS_MIN_SIZE,
    1U << 4,
    24U,
    1U << 5,
    1U << 6,
    1U << 7,
    1U << 8,
    1U << 9,
    1U << 10,
    1U << 11,
    1U << 12,
    1U << 13,
    1U << 15,
    1U << 16,
    1U << 18,
    1U << 20,
    1U << 23,
    1U << 28,
};

static constexpr u32 MAX_ALLOCATION_SIZE = SIZE_CLASS_SIZES[SIZE_CLASSES_LEN - 1];

struct Heap {
  VMem* vm;
  IterByte iter;

  /// @brief total count of blocks in free list
  /// @details this is used for tracking, also so we dont search for blocks in free list if its empty! :)
  i32 free_list_count;

  /// @brief Each size class maintians its own free list
  SizeClass buckets[SIZE_CLASSES_LEN];

  // FreeList freelist;
};

/// @brief returns size class index of given block size

static isize size_class_of(isize block_size) CONST_FUNC;
[[maybe_unused]]
static isize size_class_size_of(isize block_size) CONST_FUNC;

METHOD
static inline Block* pop_block(FreeList* self) {
  if UNLIKELY (is_null(self->first) || self->count <= 0) {
    return nullptr;
  }

  if (self->count == 1) {
    Block* res = self->first;
    self->first = nullptr;
    self->last = nullptr;
    self->count = 0;
    return res;
  }

  Block* res = self->first;
  self->first = res->next;
  self->count -= 1;
  return res;
}

PARAMS_NONNULL(1, 2)
static inline void push_block(FreeList* self, Block* freed) {
  if (is_null(self->last) || self->count <= 0) {
    freed->next = nullptr;
    self->first = freed;
    self->last = self->first;
  } else {
    self->last->next = freed;
    self->last = freed;
    self->last->next = nullptr;
  }

  self->count += 1;
}

static Block* next_free_block(Heap* self, isize block_size) METHOD;

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
  // // validate this poitner belongs to some heap and its metadata has not been corrupted
  // expect(block->size.class_slot < SIZE_CLASSES_LEN && block->size.val);

  const isize size = block->size.val;
  const isize block_size = SIZE_CLASS_SIZES[block->size.class_slot];
  return block_size - size - sizeof(Block);
}

usize heap_size_of(const Heap* self, const void* ptr) {
  assert(self);
  assert(ptr);

  Block* b = asblock(ptr);
  return b->size.val;
}

usize heap_full_size_of(const Heap* self, const void* ptr) { return heap_size_of(self, ptr) + sizeof(Block); }

isize heap_min_size(void) { return sizeof(Heap); }

Heap* heap_new(const isize vmem_size) {
  const isize size = vmem_size + sizeof(Heap);

  VMem* vm = vmem_new(size);
  if UNLIKELY (is_null(vm)) {
    DERROR("Failed to create VMem of size %li bytes!", size);
    return nullptr;
  }

  return heap_from_vmem(vm);
}

Heap* heap_from_vmem(VMem* vm) {
  IterByte iter = {.begin = vmem_begin(vm), .cursor = vmem_begin(vm), .end = vmem_end(vm)};

  // FIXME: We dont have to bail if provided VMem is not large enough, we could just call [vmem_remap]
  // and remap it to a better size that we can work with!
  Heap* self = allocate_raw(&iter, mlayout_new(Heap));
  if (is_null(self)) {
    DERROR("Could not allocate inner Heap type with vmem of size: %li bytes! minimum size is %li bytes ", vmem_size(vm),
           heap_min_size());
    return nullptr;
  }

  self->vm = vm;
  self->iter = iter;
  self->free_list_count = 0;

  for (i32 i = 0; i < (i32)SIZE_CLASSES_LEN; i++) {
    SizeClass* sc = &self->buckets[i];
    sc->class_size = SIZE_CLASS_SIZES[i];
    sc->list = (FreeList){};
  }

  return self;
}

void* heap_alloc(Heap* self, isize size, isize align) {
  if (size > MAX_ALLOCATION_SIZE) {
    DERROR(
        "Requested allocation size: %li is larger than supported maximum: %li. (%liMB) If you need to allocate more "
        "than that, create your own VMem and allocate out of that!",
        size, MAX_ALLOCATION_SIZE, OF_MB(MAX_ALLOCATION_SIZE));
    return nullptr;
  }

  Block* block = next_free_block(self, size);
  if (is_not_null(block)) {
    // next_free_block fills out metadata for new block for us so we can just return here
    return &block->data[0];
  }

  const i32 size_class_slot = size_class_of(size);
  const i32 alloc_size = SIZE_CLASS_SIZES[size_class_slot] + sizeof(Block);

  assert(alloc_size >= size);

  block = allocate_raw(&self->iter, (Layout){.size = alloc_size, .align = align});
  if UNLIKELY (is_null(block)) {
    DERROR(
        "Failed to allocate object of size %li bytes. heap only has %li bytes available! (does not include bytes of "
        "any freed blocks",
        size, iter_tail(self->iter));
    return nullptr;
  }

  // NOTE: We have to scan size class list again here, but since its literally 10 items (and i dont plan on ever making
  // it larger than 10, unless i go the dynamic size class list route) this is not something we should worry about
  block->size.class_slot = size_class_slot;
  block->size.val = size;

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
    b->size.val = new.size;
    return ptr;
  }

  const isize avail = heap_avail_ptr_size(self, ptr);

  const isize delta = new.size - old.size;
  // we can resize in place! this avoids having to search for free blocks
  if (delta <= avail) {
    b->size.val = new.size;
    return ptr;
  }

  Block* new_block = heap_alloc(self, new.size, new.align);
  if UNLIKELY (is_null(new_block)) {
    DERROR("Heap has no free space and no freed blocks are large enough for allocation of size: %li!", new.size);
    return nullptr;
  }

  mempcpy(&new_block->data[0], &b->data[0], b->size.val);
  new_block->size.val = new.size;
  new_block->size.class_slot = size_class_of(new.size);

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
  const i32 slot = b->size.class_slot;

  FreeList* fl = &self->buckets[slot].list;
  // SizeClass* sc = self->buckets.sclasses[freed->size_class_slot];
  push_block(fl, b);

  self->free_list_count += 1;
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

static Block* next_free_block(Heap* self, isize block_size) {
  // NOTE: I considered only searching for free blocks when there is memory pressure or load factor passes a certain
  // threshold, but looking for free blocks is a pretty quick process, each size class list can pop blocks in constant
  // time, so time complexity of this function is literally O(SIZE_CLASSES_LEN) or: O(10)
  if (self->free_list_count <= 0) {
    // we dont have any free blocks
    return nullptr;
  }

  for (i32 i = 0; i < (i32)SIZE_CLASSES_LEN; i++) {
    // here we index into nur buckets instead of the static constant size class array in th hopes
    // that doing so here like we are will increase the likelyhood that things are in cache
    SizeClass* sc = &self->buckets[i];
    const isize class_size = sc->class_size;
    if (class_size >= block_size) {
      // NOTE:: we dont have any blocks to provide in this size class, so we bail, if we were to continue,
      // the loop woudl select any freed block in a larger size class, this would probanly not be very good for memory
      // fragmentation lol. so we return nullptr  so that we end up allocating normally from the bumped VMem memory
      if (sc->list.count <= 0) {
        return nullptr;
      }
      Block* block = pop_block(&sc->list);
      block->size.class_slot = i;
      block->size.val = block_size;
      self->free_list_count -= 1;
      return block;
    }
  }

  return nullptr;
}

static isize size_class_size_of(isize block_size) {
  const isize slot = size_class_of(block_size);
  if UNLIKELY (slot < 0) {
    return -1;
  }
  return SIZE_CLASS_SIZES[slot];
}

static isize size_class_of(const isize block_size) {
  if (block_size <= 0 || block_size > MAX_ALLOCATION_SIZE) {
    return -1;
  }

#pragma unroll SIZE_CLASSES_LEN
  for (i32 i = 0; i < (i32)SIZE_CLASSES_LEN; i++) {
    if (block_size <= SIZE_CLASS_SIZES[i]) {
      return i;
    }
  }

  LOG_ERROR("block size: %li did not fit into any size class! This is a bug!!!", block_size);
  return -1;
}
