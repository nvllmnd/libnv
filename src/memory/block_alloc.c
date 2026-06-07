#include "nv/memory/block_alloc.h"

#include <assert.h>

#include "nv/core/attributes.h"
#include "nv/core/debug.h"
#include "nv/core/log.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/virt.h"

#define asblock(bl) (&((Block*)(bl))[-1])

/// A Block of memory used and chained together by [BlockAllocator].
struct Block {
  struct Block* next;

  ///  points to the end of this block in memory
  u8* end;

  u8 storage[];
};
alias(Block);

/// Min allocation size for requested allocations. The rational for this is that allocations smaller than the size of
/// each block's header is wasteful, so we either can enforce callers to only request allocations larger than
/// sizeof(Block), or we simple round up to 24 for allocation requests smaller than that, then at least those blocks can
/// be later reused by more allocations (smaller allocations have a less chance of being reused after free.)
static constexpr const isize BA_MIN_ALLOC_SIZE = sizeof(Block);

#define bl_begin(b) (&((b)->storage[0]))

static constexpr const i32 SIZE_CLASS_LIST_LEN = 255;

struct SizeClass {
  /// Size in bytes of this size class. Blocks of size <= to this size class are put into this size classes' free list
  /// ring buffer
  i32 class_size;
  /// Length of Ring Buffer Free List.
  i32 len;
  /// pointer to start of this Size Classes' Free List Ring Buffer
  Block** start;
  /// Pointer to end of this Size Classes' Free List Ring Buffer
  Block** end;
  /// A Ring buffer of pointers to freed blocks, segregated by size. Sizes >= [BA_FREE_LIST_MAX_SIZE] are not added to
  /// any Size Class lists and are allocated and freed separately, as blocks of that size are likley to hang around for
  /// a while
  Block* list[SIZE_CLASS_LIST_LEN];
};
alias(SizeClass);

static constexpr const i32 SIZE_CLASSES_LEN = 10;

static constexpr const i32 SIZE_CLASS_MIN_SIZE = 24;

static constexpr const i32 SIZE_CLASS_SIZES[SIZE_CLASSES_LEN] = {
    SIZE_CLASS_MIN_SIZE, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13};

static constexpr const i32 BA_FREE_LIST_MAX_SIZE = SIZE_CLASS_SIZES[SIZE_CLASSES_LEN - 1];

struct FreeList {
  i32 blocks_free;
  // TODO: We can implement this as a flat red/black tree (or a non-sorted linked list), sorted by size (or age,
  // whichever is most efficient) to help reduce the time searching through free list
  // Block* list[BA_FREE_LIST_SIZE];
  SizeClass sclasses[SIZE_CLASSES_LEN];
};
alias(FreeList);

PURE_FUNC
METHOD
static inline isize bl_size(const Block* self) { return self->end - bl_begin(self); }

PURE_FUNC
METHOD
static inline isize bl_full_size(const Block* self) { return sizeof(Block) + bl_size(self); }

/// Adds given Block to FreeList.
/// Returns true if block was successfully added to FreeList, otherwise false
METHOD
static bool fl_push(FreeList* self, Block* val);

struct BlockAllocator {
  VirtMem* vm;
  bool exclusive;

  Block* head;
  Block* tail;

  FreeList free_list;
};
alias(BlockAllocator);
METHOD
static Block* ba_next_free_block(BlockAllocator* self, isize size);

BlockAllocator* ba_owned_new(i32 vm_bytes) {
  VirtMem* vm = nullptr;
  if UNLIKELY (vmem_init(&vm, vm_bytes) != OK) {
    return nullptr;
  }
  return ba_new(vm, true);
}

NvError ba_init(BlockAllocator** out, VirtMem* backing, bool exclusive) {
  assert(out);
  if (is_null(backing)) {
    exclusive = true;
    tryerr(vmem_init(&backing, MEGABYTES(BA_BACKING_VIRTMEM_SIZE_MB)));
  }

  const MemLayout layout = mlayout_new(BlockAllocator);
  BlockAllocator* self = vmem_allocate(backing, layout);
  if UNLIKELY (is_null(self)) {
    LOG_DBG(
        "Call to %s Failed! Inner call to function vmem_allocate returned nullptr! Virtual Memory region only has %li "
        "bytes of available memory and cannot acommidate an allocation of size: %d",
        __func__, vmem_available(backing), mlayout_new(BlockAllocator).size);
    return Error__VirtMemOutOfMemory;
  }

  self->exclusive = exclusive;
  self->vm = backing;
  self->head = nullptr;
  self->tail = nullptr;
  self->free_list = make(FreeList, .blocks_free = 0, .sclasses = {});

  for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
    SizeClass* sc = &self->free_list.sclasses[i];
    sc->class_size = SIZE_CLASS_SIZES[i];
    sc->start = &sc->list[0];
    sc->end = sc->start;
  }

  *out = self;

  return OK;
}

BlockAllocator* ba_new(VirtMem* backing, bool exclusive) {
  BlockAllocator* self = nullptr;
  if UNLIKELY (ba_init(&self, backing, exclusive) != OK) {
    return nullptr;
  }
  assert(self);
  return self;
}

void* ba_allocate(BlockAllocator* self, MemLayout layout) {
  assert(self);
  assert(layout.size > 0 && IS_POWER_OF_2(layout.align));

  layout.size = layout.size < BA_MIN_ALLOC_SIZE ? BA_MIN_ALLOC_SIZE : layout.size;

  if (self->free_list.blocks_free > 0) {
    Block* next_free = ba_next_free_block(self, layout.size);

    if (next_free) {
      assert(ptr_is_aligned(next_free, alignof(Block)));
      u8* s = bl_begin(next_free);  //&next_free->storage[0];
      assert(asblock(s) == next_free);
      Block* back = asblock(s);
      assert(back);
      return s;
    }
  }
  const MemLayout block_layout = mlayout_fma(Block, layout.size);
  Block* next = vmem_allocate(self->vm, block_layout);
  if UNLIKELY (is_null(next)) {
    LOG_DBG(
        "Call to %s Failed! Inner call to vmem_allocate returned nullptr! Virtual Memory region only has %li bytes of "
        "available memory and cannot accomidate an allocation of size %d bytes!",
        __func__, vmem_available(self->vm), block_layout.size);

    return nullptr;
  }
  *next = make_zeroed(Block);
  next->next = nullptr;
  next->end = &next->storage[layout.size - 1];
  if UNLIKELY (is_null(self->head)) {
    self->head = next;
  }
  if LIKELY (is_not_null(self->tail)) {
    self->tail->next = next;
  }
  self->tail = next;
  return bl_begin(next);  //&next->storage[0];
}

void* ba_zallocate(BlockAllocator* self, MemLayout layout) {
  assert(self);
  assert(layout.size > 0 && IS_POWER_OF_2(layout.align));

  void* ptr = ba_allocate(self, layout);
  if UNLIKELY (is_null(ptr)) {
    LOG_DBG("%s[%s::%s]:%d => Inner call to ba_allocate returned nullptr!", __FILE__, STRINGIFY(BlockAllocator),
            __func__, __LINE__);
    return nullptr;
  }

  memset(ptr, 0, layout.size);

  return ptr;
}

void* ba_reallocate(BlockAllocator* self, void* ptr, MemLayout old_layout, MemLayout new_layout) {
  assert(self);
  if (is_null(ptr)) {
    LOG_DBG(
        "%s[%s::%s]:%d => Reallocation Function does not support nullptr as pointer to relocate! Please pass a pointer "
        "that was allocated by this %s",
        __FILE__, STRINGIFY(BlockAllocator), __func__, __LINE__, STRINGIFY(BlockAllocator*));
    return nullptr;
  }

  Block* bptr = asblock(ptr);

  const isize old_size = bl_size(bptr);

  // Fall back to size of block if old_size layout does not match with expected size.
  // Doing this makes the old_layout parameter entirely optional as it will be ignored
  // for the most part
  if (old_layout.size != old_size) {
    old_layout.size = old_size;
  }

  if UNLIKELY (new_layout.size == old_layout.size) {
    return ptr;
  }

  if (new_layout.size < old_layout.size) {
    bptr->end = &bptr->storage[new_layout.size - 1];
    return bl_begin(bptr);
  }

  u8* new_end = &bptr->storage[new_layout.size - 1];
  // we can grow in place!
  if UNLIKELY (new_end <= bptr->end) {
    bptr->end = new_end;

    return bl_begin(bptr);
  }

  const MemLayout block_layout = mlayout_fma(Block, new_layout.size);

  Block* next = vmem_allocate(self->vm, block_layout);
  if UNLIKELY (is_null(next)) {
    LOG_DBG(
        "%s[%s::%s]:%d Inner call to vmem_allocate returned nullptr! Virtual Memory region only has %li bytes of "
        "available memory and cannot accomadate an allocation of size %d bytes!",
        __FILE__, STRINGIFY(BlockAllocator), __func__, __LINE__, vmem_size(self->vm), new_layout.size);
    return nullptr;
  }

  memcpy(next, bptr, bl_full_size(bptr));
  ba_free(self, bptr);
  return bl_begin(next);  //&next->storage[0];
}

void ba_free(BlockAllocator* self, void* ptr) {
  assert(self);

  if UNLIKELY (is_null(ptr)) {
    LOG_DBG("%s[%s::%s]:%d => Attempted to free null pointer!", __FILE__, STRINGIFY(BlockAllocator), __func__,
            __LINE__);
    return;
  }
  assert(vmem_contains(self->vm, ptr));

  u8* p = ptr;
  u8* ps = p - sizeof(Block);
  Block* b = pcast(Block, ps);

  if (fl_push(&self->free_list, b)) {
    self->free_list.blocks_free += 1;
  }
}

NvError ba_destroy(BlockAllocator* self) {
  // TODO: Might want to add the ability to zero out memory used by this BlockAllocator entirely if
  // it does not exclusively own its backing VirtMem. For now im just going to zero out the BlockAllocator header to
  // prevent it from being used to allocate after this function returns
  if (self->exclusive) {
    tryerr(vmem_destroy(self->vm));
    return OK;
  }

  memset(self, 0, sizeof(BlockAllocator));

  return OK;
}

const AllocVTable* ba_vtable(void);

Allocator ba_allocator(BlockAllocator* self);

Block* ba_next_free_block(BlockAllocator* self, isize size) {
  assert(self);

  if (size > BA_FREE_LIST_MAX_SIZE) {
    return nullptr;
  }

  FreeList* const fl = &self->free_list;

  SizeClass* klass = nullptr;

  if (size <= BA_MIN_ALLOC_SIZE) {
    size = SIZE_CLASS_MIN_SIZE;
    klass = &fl->sclasses[0];
  } else {
    for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
      SizeClass* const sc = &fl->sclasses[i];
      if (size <= sc->class_size) {
        klass = sc;
        break;
      }
    }
  }
  if LIKELY (klass) {
    assert(klass->start);

    assert(klass->end);
    if (klass->len <= 0) {
      return nullptr;
    }

    // If start has gotten to end of list, then end has forsure already wrapped around
    // we check start != end, because it could be that start >= &list[SIZE_CLASS_LIST_LEN], but that means we are full
    if (klass->start != klass->end && klass->start >= &klass->list[SIZE_CLASS_LIST_LEN]) {
      klass->start = &klass->list[0];
      // sanity check to make sure wrapping is working properly
      assert(*klass->start);
    }

    // pop (unshift) the beginning of our free list ring buffer, selecting the
    // block that has been in this queue the longest

    Block* b = *klass->start;
    klass->start++;

    if UNLIKELY (is_null(b)) {
      ELOG_DBG("Tried to pop a block off of FreeList SizeClass of size: %d of len 1, but popped element was null!",
               klass->class_size);
      EXIT_FATAL();
    }

    klass->len -= 1;
    self->free_list.blocks_free -= 1;

    return b;
  }

  LOG_DBG(
      "Unreachable section reached! tried to find next free block for allocation of size: %li, which should fit into a "
      "size class, but did not.",
      size);

  EXIT_FATAL();
}

bool fl_push(FreeList* self, Block* val) {
  assert(self);
  assert(val);

  const i32 size = bl_size(val);

  if (size > BA_FREE_LIST_MAX_SIZE) {
    LOG_DBG(
        "Attempted to push block of size: %d bytes into FreeList, which does not support blocks of size greater than "
        "%d bytes!  Check size before attempting to add block to FreeList!",
        size, BA_FREE_LIST_MAX_SIZE);

    assert(size <= BA_FREE_LIST_MAX_SIZE);
    return false;
  }

  SizeClass* klass = nullptr;

  for (i32 i = 0; i < SIZE_CLASSES_LEN; i++) {
    SizeClass* const sc = &self->sclasses[i];
    if (size <= sc->class_size) {
      klass = sc;
      break;
    }
  }

  if UNLIKELY (is_null(klass)) {
    LOG_DBG(
        "Unrecoverable Error! Block of size %d bytes is less than max size of %d bytes, but for some reason was not "
        "able to find a SizeClass of appropriate size. Definitely a logic error. check initialization is properly "
        "done!!",
        size, BA_FREE_LIST_MAX_SIZE);

    assert(klass);
    UNREACHABLE();
  }

  if (klass->len >= SIZE_CLASS_LIST_LEN) {
    LOG_DBG(
        "Size class of size: %d bytes has no more space available to add freed block of size: %d. Memory will be "
        "silently leaked, which is not entirely undesireable in this allocation model...",
        klass->class_size, size);
    return false;
  }

  if (klass->len <= 0 || klass->start == klass->end) {
    klass->start = &klass->list[0];
    klass->end = klass->start;  // will insert val into first index slot below, then gets properly incremented as well
    klass->len = 0;  // will be incremented to 1 below, this is just to ensure that len is never a negative value

  } else if (klass->end >= &klass->list[SIZE_CLASS_LIST_LEN]) {
    klass->end = &klass->list[0];
  }

  *klass->end = val;
  klass->end++;
  klass->len += 1;
  return true;
}

bool ba_contains(const BlockAllocator* self, const void* ptr) { return vmem_contains(self->vm, ptr); }
