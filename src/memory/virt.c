
#include <stdarg.h>
#include <stdio.h>

#include "nv/core/sslice.h"
#define _GNU_SOURCE 1
#define __USE_GNU 1
#define __USE_MISC 1

#include <sys/mman.h>
#include <unistd.h>

// #include <assert.h>
// #include <errno.h>
// #include <string.h>

#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/layout.h"
#include "nv/memory/virt.h"

#if SYSTEM_POSIX

#endif

#if SYSTEM_WINDOWS
#include <windows.h>
//  TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)
#error "Windows not currently supported. TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)"
#endif

struct VirtMem {
  // VModeFlags flags;

  /// @brief lock size in bytes of this mappings [mlock]ed region (spanning from 0 - lsize)
  // i32 lsize;

  /// total size of virtual memory block, in bytes.
  i32 size;

  /// aligned pointer to the next free space to use for allocation
  u8* top;
  /// pointer to the absolute end of this region of virtual memory
  u8* end;

  ATTR_COUNTED_BY(size)
  u8 storage[];
};

i32 os_page_size(void) {
  static i32 size = -1;
  if UNLIKELY (size < 0) {
    size = sysconf(_SC_PAGESIZE);
  }
  return size;
}

NvError vmem_init(VirtMem** self, i32 size_bytes) {
  assert(self);
  assert(size_bytes > 0);

  const isize size = sizeof(VirtMem) + max(size_bytes, os_page_size());

  VirtMem* ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if UNLIKELY (ptr == MAP_FAILED) {
    LOG_DBG("Failed to map virtual memory of size: %li (%dMB). ERRNO(%d) :: %s", size, size_bytes, errno,
            strerror(errno));

    switch (errno) {
      case ENOMEM: {
        return Error__OOM;
      } break;
      case EOVERFLOW: {
        return Error__ValTooLargeFoDataType;
      } break;
      case EAGAIN: {
        return Error__ResourceTempUnavail;
        default: {
          return Error__FailedMemMap;
        } break;
      } break;
    }

    return Error__FailedMemMap;
  }

  ptr->size = size;
  ptr->top = &ptr->storage[0];
  ptr->end = ptr->top + size;
  *self = ptr;

  return Error__Ok;
}

void* vmem_allocate(VirtMem* self, MemLayout layout) {
  assert(self);
  const i32 size = layout.size;
  const i32 align = layout.align;

  if UNLIKELY (size > vmem_available(self)) {
    LOG_FATAL("Failed to allocate memory block of size: %d bytes. Virtual Memory only has %d bytes available", size,
              vmem_available(self));

    return nullptr;
  }

  u8* ptr = ptr_alignup(self->top, align);

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

NvError vmem_destroy(VirtMem* self) {
  assert(self);

  const isize size = self->size;

  const error err = munmap(self, size);

  if UNLIKELY (err == -1) {
    LOG_DBG("Call to %s Failed! Unable to unmap virtual memory of size: %li at address %p. ERRNO(%d) :: %s", __func__,
            size, pcast(void, self), errno, strerror(errno));

    switch (errno) {
      case ENOMEM: {
        return Error__OOM;
      } break;
      case EOVERFLOW: {
        return Error__ValTooLargeFoDataType;
      } break;
      case EAGAIN: {
        return Error__ResourceTempUnavail;
        default: {
          return Error__FailedMemMap;
        } break;
      } break;
    }

    return Error__FailedMemMap;
  }
  return Error__Ok;
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
  return Error__Ok;
}

i32 vmem_size(const VirtMem* self) {
  assert(self);
  return self->size;
}

i32 vmem_full_size(const VirtMem* self) {
  assert(self);
  return sizeof(VirtMem) + self->size;
}

i32 vmem_used_bytes(const VirtMem* self) {
  assert(self);
  return self->top - &self->storage[0];
}

i32 vmem_available(const VirtMem* self) {
  assert(self);
  return self->end - self->top;
}

void vmem_clear_zeroed(VirtMem* self) { vmem_zero_range(self, vmem_size(self)); }

static void vmem_vtable_free(void*, void*) {}

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
      (AllocVTable){.allocate = vmem_vtable_alloc,
                    .reallocate = vmem_vtable_realloc,
                    .zallocate = vmem_vtable_zalloc,
                    .free = vmem_vtable_free,
                    .mask = VT__Allocate | VT__Reallocate | VT__Zallocate | VT__Free};
  return &VT;
}

Allocator vmem_allocator(VirtMem* self) { return (Allocator){.ctx = self, .vtable = vmem_vtable()}; }

bool vmem_contains(const VirtMem* self, const void* p) {
  assert(self);
  assert(self->end);
  assert(p);

  const u8* ptr = p;

  return ptr >= &self->storage[0] && ptr <= self->end;
}

VirtMemView vmem_view(const VirtMem* self) {
  return (VirtMemView){.start = &self->storage[0],
                       .end = self->end,
                       .avail_bytes = vmem_available(self),
                       .used_bytes = vmem_used_bytes(self),
                       .size_bytes = self->size};
}

VMarker vmem_mark(const VirtMem* self) {
  assert(self);
  assert(self->top);
  return self->top - &self->storage[0];
}

void vmem_reset_to(VirtMem* self, VMarker marker) {
  assert(marker >= 0 && marker <= vmem_size(self));

  u8* const ntop = (&self->storage[marker]);

  assert(ntop <= self->top);

  self->top = ntop;
}

void vmem_reset_zeroed(VirtMem* self, VMarker marker) {
  assert(marker >= 0 && marker <= vmem_size(self));

  u8* const ntop = (&self->storage[marker]);
  const i32 delta_size = self->top - ntop;

  assert(ntop <= self->top);
  memset(ntop, 0, delta_size);

  self->top = ntop;
}

NvError vmem_lock(VirtMem* self, i32 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = mlock(self, nbytes);

  LOG_DBG("Called mlock with %d bytes", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to mlock with %d bytes Failed!", nbytes);
    return Error__VMapCannotLockToRAM;
  }
  return Error__Ok;
}

NvError vmem_unlock(VirtMem* self, i32 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = munlock(self, nbytes);

  LOG_DBG("Called munlock with %d bytes", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to munlock with %d bytes Failed!", nbytes);
    return Error__CannotUnlockRAM;
  }
  return Error__Ok;
}

NvError vmem_commit(VirtMem* self, i32 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = madvise(self, nbytes, MADV_WILLNEED);
  LOG_DBG("Called madvise with %d bytes and flag MADV_WILLNEED", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to madvise with %d bytes and flag MADV_WILLNEED Failed!", nbytes);
    return Error__MAdviseWillNeedFailed;
  }
  return Error__Ok;
}

NvError vmem_init_ex(VirtMem** self, VirtMemOpts opts) {
  assert(self);
  assert(opts.size_bytes > 0);

  const i32 size = max(opts.size_bytes, os_page_size());

  if (opts.mode == VMap__Default) {
    return vmem_init(self, size);
  }

  i32 flags = MAP_PRIVATE | MAP_ANONYMOUS;

  if (bithas(opts.mode, VMap__NoReserve)) {
    LOG_DBG("Adding MAP_NORESERVE flag to mmap call");
    bitset(flags, MAP_NORESERVE);
  }

  if (bithas(opts.mode, VMap__CommitAll)) {
    LOG_DBG("Adding MAP_POPULATE flag to mmap call");
    bitset(flags, MAP_POPULATE);
  }

  if (bithas(opts.mode, VMap__LockAll)) {
    LOG_DBG("Adding MAP_LOCKED flag to mmap call");
    bitset(flags, MAP_LOCKED);
  }

  const i32 access = cast(i32, opts.access);

  VirtMem* ptr = mmap(0, size, access, flags, -1, 0);
  if UNLIKELY (ptr == MAP_FAILED) {
    LOG_DBG("Failed to map virtual memory of size: %d bytes. ERRNO(%d) :: %s", size, errno, strerror(errno));

    switch (errno) {
      case ENOMEM: {
        return Error__OOM;
      } break;
      case EOVERFLOW: {
        return Error__ValTooLargeFoDataType;
      } break;
      case EAGAIN: {
        return Error__ResourceTempUnavail;
        default: {
          return Error__FailedMemMap;
        } break;
      } break;
    }

    return Error__FailedMemMap;
  }

  // NOTE: set size right away before we access storage to take advantage of the counted_by attribute
  ptr->size = size;
  ptr->top = &ptr->storage[0];
  ptr->end = ptr->top + size;
  *self = ptr;

  if (bithas(opts.mode, VMap__CommitPages) && opts.commit_bytes != 0) {
    const i32 bytes = opts.commit_bytes < 0 ? size : opts.commit_bytes;
    return vmem_commit(*self, bytes);
  }

  if (bithas(opts.mode, VMap__LockPages) && opts.lock_bytes != 0) {
    const i32 bytes = opts.lock_bytes < 0 ? size : opts.lock_bytes;
    return vmem_lock(*self, bytes);
  }

  return Error__Ok;
}

VAddrOffset vmem_offset(const VirtMem* self, const void* ptr) {
  assert(self);
  assert(ptr);
  if (vmem_contains(self, ptr)) {
    return pcast(u8, ptr) - (&self->storage[0]);
  }
  return -1;
}

void* vmem_alloc_offset(VirtMem* self, MemLayout layout, VAddrOffset* offset_out) {
  void* res = vmem_allocate(self, layout);
  if UNLIKELY (is_null(res)) {
    return nullptr;
  }

  if LIKELY (is_not_null(offset_out)) {
    *offset_out = pcast(u8, res) - (&self->storage[0]);
  }

  return res;
}

void* vmem_zalloc_offset(VirtMem* self, MemLayout layout, VAddrOffset* offset_out) {
  void* res = vmem_zallocate(self, layout);

  if UNLIKELY (is_null(res)) {
    return nullptr;
  }

  if LIKELY (is_not_null(offset_out)) {
    *offset_out = pcast(u8, res) - (&self->storage[0]);
  }

  return res;
}

void vmem_update_ptr(VirtMem* self, void** ptr, VAddrOffset rel_address) {
  assert(self);
  assert(ptr);
  assert(rel_address >= 0);
  assert(rel_address <= self->size);

  *ptr = vmem_lookup_offset(self, rel_address);
}

void* vmem_lookup_offset(VirtMem* self, VAddrOffset rel_address) {
  assert(self);
  assert(rel_address >= 0);
  assert(rel_address <= self->size);

  if UNLIKELY (rel_address > self->size) {
    LOG_FATAL("rel_address: %d is out of range! VirtMem size is only %d bytes!", rel_address, self->size);
  }

  return pcast(void, &self->storage[rel_address]);
}

NvError vmem_remap(VirtMem** s, i32 size_bytes, VRemapMode mode) {
  assert(s);

  VirtMem* self = *s;

  i32 flags = 0;

  if (mode == VRemap__AllowRelocate) {
    bitset(flags, MREMAP_MAYMOVE);
  }

  VirtMem* new_self = mremap(self, self->size, size_bytes, flags);
  // NOTE: No UNLIKELY here as a common operation could be to try to expand in place, if that fails caller may call
  // again allowing relocation
  if (new_self == MAP_FAILED) {
    if (mode == VRemap__AllowRelocate) {
      ELOG_DBG("Virtual Mapping allows reloction, but failed to remap from size: %d bytes to %d bytes", self->size,
               size_bytes);
      return Error__FailedRemap;
    }

    // NOTE: This isnt exactly an error, user may respond to this error by calling this function again, but with
    // VRemap__AllowRelocate
    ELOG_DBG("Virtual Mapping failed to expand in place!");
    return Error__CannotExpandInPlace;
  }

  *s = new_self;

  return Error__Ok;
}

sslice vmem_fslice(VirtMem* self, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;
  va_start(args);

  const sslice str = vmem_vfslice(self, fmt, args);

  va_end(args);

  return str;
}

sslice vmem_vfslice(VirtMem* self, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  i32 len = 0;
  const char* begin = vmem_vfstring(self, &len, fmt, args);

  return sslice_new(.begin = begin, .len = len);
}

char* vmem_fstring(VirtMem* self, i32* len_out, const char* fmt, ...) {
  va_list args;
  va_start(args);

  char* str = vmem_vfstring(self, len_out, fmt, args);

  va_end(args);
  return str;
}

char* vmem_vfstring(VirtMem* self, i32* len_out, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  const i32 avail = vmem_available(self);

  if UNLIKELY (avail <= 0) {
    return nullptr;
  }

  const i32 len = min(avail, vfstring_length(fmt, args) + 1);  // + 1 for null-terminator

  char* str = punwrap(vmem_allocate(self, mlayout_bytes(len)));
  // NOTE: Since we limit this string to the available memory in this virtmem, the above [vmem_allocate] call should
  // never fail

  vsnprintf(str, len, fmt, args);

  if (len_out) {
    *len_out = len - 1;  // dont include terminal null character in the length we report!
  }

  return str;
}

i32 vmem_delete_back(VirtMem* self, i32 nbytes) {
  assert(self);
  const i32 used = vmem_used_bytes(self);

  if (nbytes > used) {
    nbytes = used;
  }

  self->top -= nbytes;
  return vmem_available(self);
}

i32 vmem_delzero_back(VirtMem* self, i32 nbytes) {
  assert(self);
  const i32 used = vmem_used_bytes(self);
  i32 n = used - nbytes;

  if (n < 0) {
    n = used;
  }

  self->top -= n;

  memset(self->top, 0, n);
  return vmem_available(self);
}
