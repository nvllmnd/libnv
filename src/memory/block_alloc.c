#include "memory/block_alloc.h"

#include <assert.h>

#include "core_types.h"
#include "memory/virt.h"



struct Block {
  i32 size;
   
};
alias(Block);

struct BlockAllocator {
  VirtMem* vm;
  bool exclusive;

  // union {
  //   VirtMem* vm;

  //   struct {
  //     addr p : 63;
  //     bool exclusive : 1;
  //   };
  // };
};
alias(BlockAllocator);

BlockAllocator* ba_owned_new(isize vm_mb) {
  VirtMem* vm = nullptr;
  if (vmem_init(&vm, vm_mb) != OK) {
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
}

BlockAllocator* ba_new(VirtMem* backing, bool exclusive) {
  BlockAllocator* self = nullptr;
  if (ba_init(&self, backing, exclusive) != OK) {
    return nullptr;
  }
  assert(self);
  return self;
}

void* ba_allocate(BlockAllocator* self, MemLayout layout);

void* ba_zallocate(BlockAllocator* self, MemLayout layout);

void* ba_reallocate(BlockAllocator* self, void* ptr, MemLayout old_layout, MemLayout new_layout);

void* ba_free(BlockAllocator* self, void* ptr);

MemError ba_destroy(BlockAllocator* self);

const AllocVTable* ba_vtable(void);

Allocator ba_allocator(BlockAllocator* self);
