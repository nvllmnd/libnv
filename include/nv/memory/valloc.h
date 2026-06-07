#pragma once

#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/virt.h"

typedef enum VallocatorType : bool {
  Vallocator__VirtMem = false,
  Vallocator__Interface = true,
} VallocatorType;

#include "nv/core_types.h"
struct Vallocator {
  VallocatorType type;
  union {
    VirtMem* vm;
    Allocator interf;
  };
};
alias(Vallocator);

PURE_FUNC
PARAMS_NONNULL(1)
static inline Vallocator valloc_vmem(struct VirtMem* vm) { return (Vallocator){.type = Vallocator__VirtMem, .vm = vm}; }

#define valloc_default valloc_vmem

PURE_FUNC
static inline Vallocator valloc_interf(Allocator alloc) {
  return (Vallocator){.type = Vallocator__Interface, .interf = alloc};
}

static inline Allocator vallocator(Vallocator self) {
  if (self.type == Vallocator__VirtMem) {
    return vmem_allocator(self.vm);
  } else if (self.type == Vallocator__Interface) {
    return self.interf;
  }
  LOG_FATAL("Invalid Vallocator type: %d", self.type);
}

static inline void* valloc_allocate(Vallocator self, MemLayout layout) {
  if (self.type == Vallocator__VirtMem) {
    return vmem_allocate(self.vm, layout);
  } else if (self.type == Vallocator__Interface) {
    return allocator_allocate(self.interf, layout);
  }

  LOG_FATAL("Invalid Vallocator type: %d", self.type);
}

static inline void* valloc_zallocate(Vallocator self, MemLayout layout) {
  if (self.type == Vallocator__VirtMem) {
    return vmem_zallocate(self.vm, layout);
  } else if (self.type == Vallocator__Interface) {
    if (vtmask_has_zalloc(self.interf.vtable->mask)) {
      return allocator_zallocate(self.interf, layout);
    } else {
            LOG_DBG(
          "Tried calling Allocator::zallocate on an Allocator implementation that does not support zeroed allocation! "
          "(Mask does not have zalloc bit set!)");
        return nullptr;
    }
  }

  LOG_FATAL("Invalid Vallocator type: %d", self.type);
}

static inline void* valloc_reallocate(Vallocator self, void* ptr, MemLayout old, MemLayout nlayout) {
  if (self.type == Vallocator__VirtMem) {
    return vmem_reallocate(self.vm, ptr, old, nlayout);
  } else if (self.type == Vallocator__Interface) {
    if (vtmask_has_realloc(self.interf.vtable->mask)) {
      return allocator_reallocate(self.interf, ptr, old, nlayout);
    } else {
      LOG_DBG(
          "Tried calling Allocator::reallocate on an Allocator implementation that does not support Reallocation! "
          "(Mask does not have realloc bit set!)");
      return nullptr;
    }
  }

  LOG_FATAL("Invalid Vallocator type: %d", self.type);
}

static inline bool valloc_expand(Vallocator self, void* ptr, MemLayout old, MemLayout nlayout) {
  if (self.type == Vallocator__VirtMem) {
    return vmem_expand(self.vm, ptr, old, nlayout);
  } else if (self.type == Vallocator__Interface) {
    if (vtmask_has_expand(self.interf.vtable->mask)) {
      return allocator_expand(self.interf, ptr, old, nlayout);
    } else {
      LOG_DBG(
          "Tried calling Allocator::expand on an Allocator implementation that does not support In place expansion! "
          "(Mask does not have expand bit set!)");
      return false;
    }
  }

  LOG_FATAL("Invalid Vallocator type: %d", self.type);
}
