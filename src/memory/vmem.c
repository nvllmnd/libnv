// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "nv/core/ext.h"

#include "nv/memory/vmem.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nv/common.h"
#include "nv/core/algo.h"
#include "nv/core/core_types.h"
#include "nv/core/log.h"
#include "nv/core/stb_sprintf.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

 

#define vmem_lock_prefault_check(_self, _from, _size)                                \
  {                                                                                  \
    NvError err = OK;                                                                \
    if UNLIKELY (is_null(_from)) {                                                   \
      bitset(err, Error__ParamInvalidNull);                                          \
    }                                                                                \
    if (_size <= 0) {                                                                \
      return err | Error__ParamUnexpectedNegOrZeroInt;                               \
    }                                                                                \
    if (!vmem_contains(_self, _from)) {                                              \
      LOG_ERROR("VMem does not contain/own memory span of size: %li bytes!", _size); \
      return err | Error__PtrNotOwnedByVMem;                                         \
    }                                                                                \
  }

Vallocator va_new_ex(i64 vmem_size, bool noreserve) {
  // Vallocator self = {};
  // bailerr_with(vmem_init(&self.mem, vmem_size, noreserve), VALLOC_NONE);

  // self.cursor = vmem_begin(self.mem);

  VMem* mem = vmem_new_ex(vmem_size, noreserve);
  if UNLIKELY (is_null(mem)) {
    return VALLOC_NONE;
  }

  byte* begin = vmem_begin(mem);
  byte* cursor = begin;
  byte* end = vmem_end(mem);
  return (Vallocator){.mem = mem, .iter = (IterByte){.begin = begin, .cursor = cursor, .end = end}};
}

NvError vmem_ram_lock(VMem* self, void* from, i64 size) {
  assert(self);

  vmem_lock_prefault_check(self, from, size);

  // NOTE: if from is nullptr, it will get caught in below call to vmem_contains, which does a couple lt/gt operations
  // on it without dereferencing it at all, so itll return false (correctly) which then gets caught and this func will
  // return early

  const i32 err = mlock(from, size);

  if (err != 0) {
    LOG_ERROR("Failed to lock memory span of size: %li bytes! ERRNO[%d] = => %s", size, errno, strerror(errno));
    return Error__VMemFailedToLockRangeToRAM;
  }

  return OK;
}

NvError vmem_ram_release(VMem* self, void* from, i64 size) {
  assert(self);

  vmem_lock_prefault_check(self, from, size);

  const i32 err = munlock(from, size);

  if (err != 0) {
    LOG_ERROR("Failed to unlock memory span of size: %li bytes! ERRNO[%d] = => %s", size, errno, strerror(errno));
    return Error__VMemFailedToUnlockRangeFromRAM;
  }

  return OK;
}

NvError vmem_prefault_range(VMem* self, void* from, i64 size) {
  assert(self);

  static i64 page_size = 0;

  if UNLIKELY (page_size == 0) {
    page_size = os_page_size();
  }

  // NOTE: users deal with this range and shouldnt have to worry about decrementing vmem_begin by the size of VMem's
  // header data, so we check here if that is the case and adjust as we need
  if (from == vmem_begin(self)) {
    from = self;
  }

  if (!ptr_is_aligned(from, page_size)) {
    LOG_ERROR("Given pointer is not page aligned! Must provide a page aligned pointer to pass to madvise!");
    return Error__UnexpectedMisAlignedPtr;
  }

  const i32 err = madvise(from, size, MADV_WILLNEED);

  if (err != 0) {
    LOG_ERROR("Failed to prefault memory span of size: %li bytes! (madvise call failed) ERRNO[%d] = => %s", size, errno,
              strerror(errno));
    return Error__VMemFailedToLockRangeToRAM;
  }

  return OK;
}

i64 os_page_size(void) {
  static i64 size = -1;
  if UNLIKELY (size < 0) {
    size = sysconf(_SC_PAGESIZE);
  }
  return size;
}

VMem* vmem_new_ex(const i64 size_bytes, const bool noreserve) {
  VMem* self = {};

  bailerr_with(vmem_init(&self, size_bytes, noreserve), nullptr);

  assert(self);
  return self;
}

VMem* vmem_new(const i64 size_bytes) { return vmem_new_ex(size_bytes, VMEM_NORESERVE_DEFAULT); }

NvError vmem_init(VMem** s, const i64 size_bytes, const bool noreserve) {
  assert(s);
  if UNLIKELY (size_bytes <= 0) {
    LOG_ERROR("Tried to initialize VMem with a negative or 0 size: %li, noreserve: %s", size_bytes,
              noreserve ? "true" : "false");
    return Error__InvalidAllocationSize;
  }

  const i64 size = max(size_bytes, os_page_size()) + VMEM_PREFIX_SIZE;

  const i32 flags = noreserve ? (MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE) : (MAP_PRIVATE | MAP_ANONYMOUS);

  VMem* ptr = mmap(0, size, PROT_READ | PROT_WRITE, flags, -1, 0);
  if UNLIKELY (ptr == MAP_FAILED) {
    LOG_ERROR("Failed to map virtual memory of size: %li from requested size: %li ERRNO(%d) :: %s", size, size_bytes,
              errno, strerror(errno));

    return Error__FailedMemMap;
  }

  ptr->size = size_bytes;

  *s = ptr;
  return OK;
}

typedef enum VRemapMode {
  VRemap__ExpandInPlace = 0,
  VRemap__AllowRelocate,
} VRemapMode;

VMem* vmem_remap(VMem* self, i64 new_size, VRemapMode mode) {
  assert(self);

  i32 flags = 0;
  if (mode == VRemap__AllowRelocate) {
    bitset(flags, MREMAP_MAYMOVE);
  }
  const i64 old_size = vmem_size_full(self);

  const i64 size = new_size + VMEM_PREFIX_SIZE;

  VMem* new = mremap(self, old_size, size, flags);

  if (new == MAP_FAILED) {
    if (mode == VRemap__AllowRelocate) {
      LOG_ERROR(
          "Failed to remap VMem from size %li bytes to size %li bytes, even though MREMAP_MAYMOVE flag was enabled! "
          "ERRNO => %s",
          old_size - VMEM_PREFIX_SIZE, new_size, strerror(errno));
    }

    return nullptr;
  }
  new->size = new_size;
  return new;
}

METHOD
void vmem_destroy(VMem* self) {
  if (is_not_null(self)) {
    const i64 size = self->size;
    const error err = munmap(self, size);
    if UNLIKELY (err == -1) {
      // if we failed to unmap, then there is something wrong with our data or logic, or memory got corrupted, or some
      // other catastrophic error condition
      LOG_FATAL(
          "Unable to unmap VMem of size: %li at address: %p. ERRNO => %s. This is a bug, please report to "
          "nvllmnd@pm.me",
          size, (void*)self, strerror(errno));
    }
    return;
  }
  LOG_DBG("Tried to call vmem_destroy on nullptr!");
}

VOffset vmem_offset(const VMem* self, const void* ptr) {
  assert(self);
  assert(ptr);
  if (vmem_contains(self, ptr)) {
    return pcast(byte, ptr) - vmem_cbegin(self);
  }
  return -1;
}

bool vmem_contains(const VMem* self, const void* ptr) {
  assert(self);

  const byte* p = ptr;
  const byte* begin = vmem_cbegin(self);
  const byte* end = vmem_cend(self);

  return (p >= begin && p < end);
}

VMark va_checkpoint(const Vallocator* self) {
  assert(self);
  const VMark m = iter_head(self->iter);
  assert(m >= 0 && m < self->mem->size);
  return m;
}

i64 va_reset_to(Vallocator* self, VMark mark) {
  assert(self);
  if UNLIKELY (mark < 0 || mark >= vmem_size(self->mem)) {
    LOG_ERROR("Cannot reset Vallocator to out of range VMark: %li", mark);
    return -1;
  }

  byte* top = &self->mem->data[mark];
  i64 delta = 0;
  if (top < self->iter.cursor) {
    delta = self->iter.cursor - top;
  }
  return delta;
}


char* va_fstring(Vallocator* self, i64* len_out, const char* fmt, ...) {

  va_list args = {};
  va_start(args);

  char* ptr = va_vfstring(self, len_out, fmt, args);

  va_end(args);
  return ptr;
}


sslice va_fslice(Vallocator* self, const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const sslice sl = va_vfslice(self, fmt, args);

  va_end(args);
  return sl;
  
}


void va_destroy(Vallocator* self) {
  if (self && is_not_null(self->mem)) {
    vmem_destroy(self->mem);
    memset(self, 0, sizeof(IterByte));
  }
}

// char* va_realpath(Vallocator* self, sslice path) {
//   static char PATH[PATH_MAX] = {};

//   const char* p = realpath(path.begin, char *restrict resolved)

// }

void va_clear(Vallocator* self) {
  assert(self);

  iter_reset(&self->iter);
}

void va_clear_zeroed(Vallocator* self) {
  const i64 used = va_used_bytes(self);
  va_clear(self);
  memset(vmem_begin(self->mem), 0, used);
}

METHOD
static void va_free(VArena*, void*) {}

#define VT_DEFINE(_rest, _name) static VT_DEFINE_AS(VArena, _rest, _name)

VT_DEFINE(ALLOC, va_allocate)
VT_DEFINE(ZALLOC, va_zallocate)
VT_DEFINE(REALLOC, va_reallocate)
VT_DEFINE(RESIZE, va_resize)
VT_DEFINE(FREE, va_free)

#define VT_NAME(_ty) VT_NAMEOF(VArena, _ty)

static constexpr const AllocVTable VA_VT = (AllocVTable){.allocate = VT_NAME(ALLOC),
                                                         .zallocate = VT_NAME(ZALLOC),
                                                         .resize = VT_NAME(RESIZE),
                                                         .reallocate = VT_NAME(REALLOC),
                                                         .free = VT_NAME(FREE),
                                                         .mask = VT__All};

Allocator va_allocator(Vallocator* self) { return (Allocator){.ctx = self, .vtable = &VA_VT}; }
