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
  bool is_avail;
  /// index of where this block resides in the free list array, only used for blocks that are free (is_avail == true),
  /// otherwise this value is a negative number
  i32 fl_id;
  struct Block* next;

  ///  points to the end of this block in memory
  u8* end;

  u8 storage[];
};
alias(Block);

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
  i64 gen;
  /// Index of element to overwrite in FreeList if FreeList is too full to add another free block.
  /// We just take the element nearest to the end of FreeList, as i figure those elements have the highest chance of
  /// being the oldest in the list, plus it really doesnt matter THAT much which free block gets overwritten, speedy allocation and deallocation is more desired
  i32 overwrite_index;
  i32 len;
  Block* list[BA_FREE_LIST_SIZE];
};
alias(FreeList);


PURE_FUNC
METHOD
[[maybe_unused]]
static inline isize bl_size(const Block* self) {
  return self->end - self->storage;
}

PURE_FUNC
METHOD
static inline isize bl_full_size(const Block* self) { return sizeof(Block) + bl_size(self); }

/// Adds given Block to FreeList.
/// Returns true if block was successfully added to FreeList, otherwise false
METHOD
static bool fl_push(FreeList* self, Block* val);

[[maybe_unused]]
/// Same as [fl_push], but ensures userdata section of pushed block is zeroed
METHOD static inline bool fl_push_zeroed(FreeList* self, Block* val) {
  if (fl_push(self, val)) {
    memset(val->storage, 0, bl_size(val));
    return true;
  }
  return false;
}

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
[[maybe_unused]]
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

  static constexpr const isize SIZE = sizeof(BlockAllocator);

  BlockAllocator* self = vmem_allocate(backing, mlayout_bytes(SIZE));
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

  {
    Block* next_free =
        ba_next_free_block(self, layout.size + sizeof(void*));  // + 4/8 bytes in case alignment needs to be adusted
    if (next_free) {
      return next_free->storage;
    }
  }
  const isize alloc_size = sizeof(Block) + layout.size;
  Block* next = vmem_allocate(self->vm, mlayout_bytes(alloc_size));
  if UNLIKELY (is_null(next)) {
    LOG_DBG(
        "Call to %s Failed! Inner call to vmem_allocate returned nullptr! Virtual Memory region only has %li bytes of "
        "available memory and cannot accomidate an allocation of size %li bytes!",
        __func__, vmem_available(self->vm), alloc_size);

    return nullptr;
  }
  next->is_avail = false;
  next->next = nullptr;
  next->end = next->storage + layout.size;
  if UNLIKELY (is_null(self->head)) {
    self->head = next;
  }
  if LIKELY (is_not_null(self->tail)) {
    self->tail->next = next;
  }
  self->tail = next;
  return next->storage;
}

void* ba_zallocate(BlockAllocator* self, MemLayout layout) {
  assert(self);
  assert(layout.size > 0 && IS_POWER_OF_2(layout.align));

  void* ptr = ba_allocate(self,  layout);
  if (is_null(ptr)) {
    LOG_DBG("%s[%s::%s]:%d => Inner call to ba_allocate returned nullptr!", __FILE__, STRINGIFY(BlockAllocator), __func__,  __LINE__);
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
    bptr->end = bptr->storage + new_layout.size;
    return bptr->storage;
  }

  if (new_layout.size > old_layout.size) {
    u8* new_end = bptr->storage + new_layout.size;
    // we can grow in place!
    if UNLIKELY (new_end <= bptr->end) {
      bptr->end = new_end;
      return bptr->storage;
    }

    Block* next = vmem_allocate(self->vm, new_layout);
    if UNLIKELY (is_null(next)) {
      LOG_DBG(
          "%s[%s::%s]:%d Inner call to vmem_allocate returned nullptr! Virtual Memory region only has %li bytes of "
          "available memory and cannot accomadate an allocation of size %d bytes!",
          __FILE__, STRINGIFY(BlockAllocator), __func__, __LINE__, vmem_size(self->vm), new_layout.size);
      return nullptr;
    }

    memcpy(next, bptr, bl_full_size(bptr));
    ba_free(self, bptr);
    return next->storage;
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
  Block* bl = &pcast(Block, ptr)[-1];

  assert(fl_push(&self->free_list, bl));
  // if (!fl_push(&self->free_list, bl)) {
  //   self->free_list.list[self->free_list.oldest_index] = bl;

  // }
}

MemError ba_destroy(BlockAllocator* self);

const AllocVTable* ba_vtable(void);

Allocator ba_allocator(BlockAllocator* self);

Block* ba_next_free_block(BlockAllocator* self, isize size) {
  assert(self);

  FreeList* fl = &self->free_list;

  for (i32 i = 0; i < fl->len; i++) {
    Block* b = fl->list[i];
    if (b->is_avail && bl_size(b) >= size) {
      fl_delete(fl, b);
      return b;
    }
  }

  return nullptr;
}

// static inline void fl_rescan_oldest(FreeList* self) {
//   if (self->len == 1) {
//     self->oldest_genid = self->gen++;
//     self->oldest_index = 0;
//     return;
//   }
//   i32 oldest_index = -1;
//   i32 oldest_genid = INT32_MAX;
//   for (i32 i = 0; i < self->len; i++) {
//     const Block* b = self->list[i];
//     const i32 gid = b->genid;
//     // if this gen is less than the previous genid, we forsure have our oldest element
//     // TODO: Need to check that this branch is actually possible to be taken, not sure if its possible to come across
//     a
//     // gid that is older than oldest_genid like this
//     if (gid < self->oldest_genid) {
//       oldest_index = i;
//       oldest_genid = gid;
//       break;
//       // self->oldest_index = i;
//       // self->oldest_genid = gid;
//     }
//     // otherwise we keep scanning through the entirety of freelist to find oldest
//     if (gid < oldest_genid) {
//       oldest_index = i;
//       oldest_genid = gid;
//     }
//   }

//   assert(oldest_index >= 0);
//   assert(oldest_genid != INT32_MAX);

//   self->oldest_genid = oldest_genid;
//   self->oldest_index = oldest_index;
// }

bool fl_push(FreeList* self, Block* val) {
  assert(self);
  assert(val);

  val->is_avail = true;

  if LIKELY (self->len < BA_FREE_LIST_SIZE) {
    const isize index = self->len;
    // store index inside Block header for O(1) frees
    val->fl_id = index;
    // Mark this block as available since it has been freed
    self->list[index] = val;
    self->len += 1;
    return true;
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
  return false;
}

void fl_delete(FreeList* self, Block* val) {
  assert(self);
  assert(val);

  const isize index = val->fl_id;
  assert(index >= 0 && index < BA_FREE_LIST_SIZE);

  /// Mark this block as no longer available (freed)
  val->is_avail = false;
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
    // fl_rescan_oldest(self);
  }
}

bool ba_contains(const BlockAllocator* self, const void* ptr) { return vmem_contains(self->vm, ptr); }
