#include "memory/block_alloc.h"

#include <assert.h>

#include "attributes.h"
#include "core_types.h"
#include "log.h"
#include "memory/alloc.h"
#include "memory/error.h"
#include "memory/virt.h"

#define asblock(bl) (&((Block*)(bl))[-1])

/// A Block of memory used and chained together by [BlockAllocator].
struct Block {
  /// index of where this block resides in the free list array, only used for blocks that are free (is_avail == true),
  /// otherwise this value is a negative number
  i32 fl_id;
  struct Block* next;

  ///  points to the end of this block in memory
  u8* end;

  u8 storage[];
};
alias(Block);

/// Min allocation size for requested allocations. The rational for this is that allocations smaller than the size of each block's header is wasteful,
/// so we either can enforce callers to only request allocations larger than sizeof(Block), or we simple round up to 24 for allocation requests smaller than that,
/// then at least those blocks can be later reused by more allocations (smaller allocations have a less chance of being reused after free.)
static constexpr const isize BA_MIN_ALLOC_SIZE = sizeof(Block);

#define bl_begin(b) (&((b)->storage[0]))

#if defined(BA_FL_SIZE)

#if BA_FL_SIZE >= 16
static constexpr const isize BA_FREE_LIST_SIZE = BA_FL_SIZE
#else
#error "Preprocessor macro define: BA_FL_SIZE must be >= 16"
#endif
#else
static constexpr const isize BA_FREE_LIST_SIZE = 1024;
#endif

    struct FreeList {
  /// Index of element to overwrite in FreeList if FreeList is too full to add another free block.
  /// We just take the element nearest to the end of FreeList, as i figure those elements have the highest chance of
  /// being the oldest in the list, plus it really doesnt matter THAT much which free block gets overwritten, speedy
  /// allocation and deallocation is more desired
  i32 overwrite_index;
  i32 len;
  // TODO: We can implement this as a flat red/black tree (or a non-sorted linked list), sorted by size (or age, whichever is most efficient) to
  // help reduce the time searching through free list
  Block* list[BA_FREE_LIST_SIZE];
};
alias(FreeList);

PURE_FUNC
METHOD
static inline isize bl_size(const Block* self) {
  return self->end - bl_begin(self);
}

PURE_FUNC
METHOD
static inline isize bl_full_size(const Block* self) { return sizeof(Block) + bl_size(self); }

/// Adds given Block to FreeList.
/// Returns true if block was successfully added to FreeList, otherwise false
METHOD
static void fl_push(FreeList* self, Block* val);

METHOD
static void fl_delete(FreeList* self, Block* val);

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

BlockAllocator* ba_owned_new(isize vm_mb) {
  VirtMem* vm = nullptr;
  if UNLIKELY (vmem_init(&vm, vm_mb) != OK) {
    return nullptr;
  }
  return ba_new(vm, true);
}

MemError ba_init(BlockAllocator** out, VirtMem* backing, bool exclusive) {
  assert(out);
  if (is_null(backing)) {
    exclusive = true;
    tryerr(vmem_init(&backing, BA_BACKING_VIRTMEM_SIZE_MB));
  }

  const MemLayout layout = mlayout_new(BlockAllocator);
  BlockAllocator* self = vmem_allocate(backing, layout);
  if UNLIKELY (is_null(self)) {
    LOG_DBG(
        "Call to %s Failed! Inner call to function vmem_allocate returned nullptr! Virtual Memory region only has %li "
        "bytes of available memory and cannot acommidate an allocation of size: %d",
        __func__, vmem_available(backing), mlayout_new(BlockAllocator).size);
    return MemError__VirtMemOutOfMemory;
  }

  self->exclusive = exclusive;
  self->vm = backing;
  self->head = nullptr;
  self->tail = nullptr;
  self->free_list = make(FreeList,  .overwrite_index = 0, .len = 0, .list = {});

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

  {
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

  if (new_layout.size > old_layout.size) {
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

  // should never reach this point. as we have check if new.size == old.size, new.size < old.size and finally new.size <
  // old.size
  HEDLEY_UNREACHABLE();
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


  fl_push(&self->free_list, b);
}

MemError ba_destroy(BlockAllocator* self) {
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

  FreeList* fl = &self->free_list;

  for (i32 i = 0; i < fl->len; i++) {
    Block* b = fl->list[i];
    if (bl_size(b) >= size) {
      fl_delete(fl, b);
      return b;
    }
  }

  return nullptr;
}

void fl_push(FreeList* self, Block* val) {
  assert(self);
  assert(val);

  if LIKELY (self->len < BA_FREE_LIST_SIZE) {
    const isize index = self->len;

    // store index inside Block header for O(1) frees
    val->fl_id = index;
    // Mark this block as available since it has been freed
    self->list[index] = val;
    self->len += 1;
  } else {
    // If we are out of space to add a new Block poitner to free list,
    // then we overwrite the oldest element in free list
    const isize index = self->overwrite_index;

    val->fl_id = index;


    // overwrite oldest element
    self->list[index] = val;

    const isize back_index = self->len - 1;

    // if we already overwrote the back element,
    if (index == back_index) {
      // set next overwrite/oldest to element before that one
      self->overwrite_index = back_index - 1;
    }
  }
}

void fl_delete(FreeList* self, Block* val) {
  assert(self);
  assert(val);

  const isize index = val->fl_id;
  assert(index >= 0 && index < BA_FREE_LIST_SIZE);


  /// Mark this block as no longer available (freed);
  /// We no longer need this value, so mark it as negative number
  /// to signal this block is not in free list
  val->fl_id = -1;

  const isize back_index = self->len - 1;
  Block* back = self->list[back_index];
  self->list[index] = back;
  self->len -= 1;

  if UNLIKELY (index == self->overwrite_index) {
    // just set oldest index to the last element of our free list, so we dont have
    // to spend cpu cycles scanning for the oldest element, its really not that important, speedy allocations are more
    // desired.
    self->overwrite_index = self->len - 1;
  }
}

bool ba_contains(const BlockAllocator* self, const void* ptr) { return vmem_contains(self->vm, ptr); }
