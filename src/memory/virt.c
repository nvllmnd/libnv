#include "memory/virt.h"

#include <assert.h>
#include <string.h>

#include "core/log.h"
#include "memory/alloc.h"
#include "memory/error.h"

#if SYSTEM_POSIX
#include <errno.h>
#include <sys/mman.h>
#endif

#if SYSTEM_WINDOWS
#include <windows.h>
//  TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)
#error "Windows not currently supported. TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)"
#endif

struct VirtMem {
#if SYSTEM_WINDOWS
  /// NOTE: This will not compile for windows currently as of 04/05/2026.
  ///       I cant be tiffed. Micro$hit Windoze is bloatware anway :)
  /// number of bytes available for allocation
  MemSize committed;
#endif
  /// total size of virtual memory block, in bytes.
  MemSize size;

  /// aligned pointer to the next free space to use for allocation
  u8* top;
  /// pointer to the absolute end of this region of virtual memory
  u8* end;

  u8 storage[];
};

MemError vmem_init(VirtMem** self, isize size_in_mb) {
  assert(self);
  size_in_mb = clamp(size_in_mb, MEMSIZE_MIN_MB, MEMSIZE_MAX_MB);

  const isize size = sizeof(VirtMem) + MEGABYTES(size_in_mb);

  VirtMem* ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if UNLIKELY (ptr == MAP_FAILED) {
    LOG_DBG("Failed to map virtual memory of size: %li (%liMB). ERRNO(%d) :: %s", size, size_in_mb, errno,
            strerror(errno));

    switch (errno) {
      case ENOMEM: {
        return MemError__OOM;
      } break;
      case EOVERFLOW: {
        return MemError__ValTooLargeFoDataType;
      } break;
      case EAGAIN: {
        return MemError__ResourceTempUnavail;
        default: {
          return MemError__FailedMemMap;
        } break;
      } break;
    }

    return MemError__FailedMemMap;
  }

  ptr->size = size;
  ptr->top = &ptr->storage[0];
  ptr->end = &ptr->storage[ptr->size - 1];
  *self = ptr;

  return MemError__Ok;
}

void* vmem_allocate(VirtMem* self, MemLayout layout) {
  assert(self);
  const isize size = layout.size;
  const isize align = layout.align;

  if UNLIKELY (size > vmem_available(self)) {
    LOG_DBG(
        "Call to %s Unsuccessful! Virtual Memory has been exhausted! Clear/Reset this VirtMem, or release its memory "
        "back to the system and reinitialize to allocate more memory from this VirtMem!",
        __func__);
    return nullptr;
  }

  u8* ptr = align_ptr(self->top, align);

  // sanity check
  assert(ptr <= self->end);

  self->top = ptr + layout.size;

  assert(self->top <= self->end);

  return ptr;
}

void* vmem_zallocate(VirtMem* self, MemLayout layout) {
  void* ptr = vmem_allocate(self, layout);
  if LIKELY (ptr) {
    memset(ptr, 0, layout.size);
    return ptr;
  }
  return nullptr;
}

MemError vmem_destroy(VirtMem* self) {
  assert(self);

  const isize size = self->size;

  const error err = munmap(self, size);

  if UNLIKELY (err == -1) {
    LOG_DBG("Call to %s Failed! Unable to unmap virtual memory of size: %li at address %p. ERRNO(%d) :: %s", __func__,
            size, pcast(void, self), errno, strerror(errno));

    switch (errno) {
      case ENOMEM: {
        return MemError__OOM;
      } break;
      case EOVERFLOW: {
        return MemError__ValTooLargeFoDataType;
      } break;
      case EAGAIN: {
        return MemError__ResourceTempUnavail;
        default: {
          return MemError__FailedMemMap;
        } break;
      } break;
    }

    return MemError__FailedMemMap;
  }
  return MemError__Ok;
}

void vmem_clear(VirtMem* self) {
  assert(self);
  self->top = &self->storage[0];
}

error vmem_zero_range(VirtMem* self, isize index) {
  assert(self);
  vmem_clear(self);

  if UNLIKELY (index >= self->size) {
    LOG_DBG("Call to %s Failed! Index requested is out of range! index: %li, bounded at: %d", __func__, index,
            self->size);
    return ApiError__ValueOutOfRange;
  }

  memset(&self->storage[0], 0, index);
  return MemError__Ok;
}

isize vmem_size(const VirtMem* self) {
  assert(self);
  return self->size;
}

isize vmem_full_size(const VirtMem* self) {
  assert(self);
  return sizeof(VirtMem) + self->size;
}

isize vmem_used_bytes(const VirtMem* self) {
  assert(self);
  return self->top - &self->storage[0];
}

isize vmem_available(const VirtMem* self) {
  assert(self);
  return self->end - self->top;
}

void vmem_clear_zeroed(VirtMem* self) { vmem_zero_range(self, vmem_size(self)); }

static void* vmem_vtable_alloc(void* self, MemLayout layout) { return vmem_allocate(self, layout); }

static void* vmem_vtable_zalloc(void* self, MemLayout layout) { return vmem_zallocate(self, layout); }

static void* vmem_vtable_realloc(void* self, void* ptr, MemLayout old, MemLayout newlayout) {
  assert(self);
  assert(ptr);

  VirtMem* s = self;

  void* dst = vmem_allocate(s, newlayout);
  if (dst) {
    memcpy(dst, ptr, old.size);
    return dst;
  }

  return nullptr;
}

const AllocVTable* vmem_vtable(void) {
  static constexpr const AllocVTable VT =
      make(AllocVTable, .allocate = vmem_vtable_alloc, .reallocate = vmem_vtable_realloc,
           .zallocate = vmem_vtable_zalloc, .free = NO_IMPL_FREE);
  return &VT;
}

Allocator vmem_allocator(VirtMem* self) { return make(Allocator, .ctx = self, .vtable = vmem_vtable()); }

bool vmem_contains(const VirtMem* self, const void* p) {
  assert(self);
  assert(self->end);
  assert(p);
  assert(self->storage);

  const u8* ptr = p;

  return ptr >= &self->storage[0] && ptr <= self->end;
}
