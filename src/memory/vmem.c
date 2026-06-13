// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/ext.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nv/common.h"
#include "nv/core/algo.h"
#include "nv/memory/alloc.h"
#include "nv/core/core_types.h"
#include "nv/core/log.h"
#include "nv/core/stb_sprintf.h"
#include "nv/memory/error.h"
#include "nv/memory/vmem.h"

PURE_FUNC
METHOD
i64 vmem_size(const VMem* self) {
  assert(self);
  return self->size;
}

i64 os_page_size(void) {
  static i64 size = -1;
  if UNLIKELY (size < 0) {
    size = sysconf(_SC_PAGESIZE);
  }
  return size;
}

VMem* vmem_new(const i64 size_bytes, const bool noreserve) {
  VMem* self = {};

  bailerr_with(vmem_init(&self, size_bytes, noreserve), nullptr);

  assert(self);
  return self;
}

NvError vmem_init(VMem** s, const i64 size_bytes, const bool noreserve) {
  assert(s);
  if UNLIKELY (size_bytes <= 0) {
    LOG_ERROR("Tried to initialize VMem with a negative or 0 size: %li, noreserve: %s", size_bytes,
              noreserve ? "true" : "false");
    return Error__InvalidAllocationSize;
  }

  const i64 size = max(size_bytes, os_page_size()) + VMEM_HEADER_SIZE;

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

  const i64 size = new_size + VMEM_HEADER_SIZE;

  VMem* new = mremap(self, old_size, size, flags);

  if (new == MAP_FAILED) {
    if (mode == VRemap__AllowRelocate) {
      LOG_ERROR(
          "Failed to remap VMem from size %li bytes to size %li bytes, even though MREMAP_MAYMOVE flag was enabled! "
          "ERRNO => %s",
          old_size - VMEM_HEADER_SIZE, new_size, strerror(errno));
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
  if UNLIKELY (is_null(self) || is_null(ptr)) {
    return false;
  }
  const byte* p = ptr;
  const byte* begin = vmem_cbegin(self);
  const byte* end = vmem_cend(self);

  return (p >= begin && p < end);
}

VMark va_checkpoint(const Vallocator* self) {
  assert(self);
  const VMark m = self->cursor - vmem_cbegin(self->mem);
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
  if (top < self->cursor) {
    delta = self->cursor - top;
  }
  return delta;
}

i64 va_available(const Vallocator* self) {
  assert(self);
  return self->mem->size - va_used_bytes(self);
}

i64 va_used_bytes(const Vallocator* self) {
  assert(self);
  return self->cursor - vmem_cbegin(self->mem);
}

void* va_allocate(Vallocator* self, Layout layout) {
  assert(self);
  assert(self->cursor);

  byte* ptr = ptr_alignup(self->cursor, layout.align);
  byte* next = ptr + layout.size;
  byte* end = vmem_end(self->mem);

  if UNLIKELY (next >= end) {
    LOG_ERROR("Vallocator of size: %li bytes does not have room for object of size: %li, only %li bytes available!",
              self->mem->size, layout.size, va_available(self));
    return nullptr;
  }

  self->cursor = next;

  return ptr;
}

METHOD
void* va_zallocate(Vallocator* self, Layout layout) {
  void* ptr = va_allocate(self, layout);
  if UNLIKELY (is_null(ptr)) {
    return nullptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}

#define va_make(_self, T) ((__typeof(T)*)va_allocate((_self), mlayout_new(T)))
#define va_alloc_array(_self, T, N) ((__typeof(T)*)va_allocate((_self), mlayout_array(T, N)))
#define va_alloc_vec(_self, T, _n) ((__typeof(T)*)va_allocate((_self), mlayout_vec(T, _n)))

PARAMS_NONNULL(1, 2)
bool va_resize(Vallocator* self, void* ptr, Layout old, Layout new) {
  assert(self);
  assert(ptr);
  if (old.size == new.size) {
    return false;
  }
  if ((self->cursor - old.size) == (byte*)ptr) {
    // Shrink if newsize is greater oldsize, otherwise grow
    const i64 delta = new.size - old.size;
    self->cursor += delta;

    return true;
  }

  return false;
}

PARAMS_NONNULL(1, 2)
void* va_reallocate(Vallocator* self, void* ptr, Layout old, Layout new) {
  if UNLIKELY (old.size == new.size) {
    return ptr;
  }

  if (va_resize(self, ptr, old, new)) {
    return ptr;
  }

  if (new.size < old.size) {
    return ptr;
  }

  void* res = va_allocate(self, new);
  if UNLIKELY (is_null(res)) {
    LOG_ERROR("Failed to Reallocate Memory of size %li bytes to size %li bytes", old.size, new.size);
    return nullptr;
  }

  memcpy(res, ptr, old.size);

  return res;
}

PARAMS_NONNULL(1, 2)
char* va_strdup(Vallocator* self, const char* str) {
  const i64 len = stringlen(str);
  return va_strndup(self, str, len);
}

PARAMS_NONNULL(1, 2)
char* va_strndup(Vallocator* self, const char* str, i32 len) {
  assert(self);
  assert(str);

  const i64 avail = va_available(self) - 1;
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Cannot dup string: %.*s of length %d in Vallocator with only %li bytes available!", len, str, len,
              avail);
    return nullptr;
  }
  if UNLIKELY (len > avail) {
    LOG_INFO("String: %.*s of length: %d will be truncated to %.*s to fit inside vallocator with %li bytes available!",
             len, str, len, (i32)avail, str, avail);
    len = avail;
  }

  char* ptr = va_allocate(self, mlayout_bytes(len + 1));

  strncpy(ptr, str, len);
  ptr[len + 1] = 0;
  return ptr;
}

PARAMS_NONNULL(1, 2)
sslice va_sslice_dup(Vallocator* self, const char* str, i32 len) {
  const char* ptr = va_strndup(self, str, len);
  if UNLIKELY (is_null(ptr)) {
    return sslice_empty();
  }
  return sslice_new(.begin = ptr, .len = len);
}

sslice va_fslice(Vallocator* self, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;
  va_start(args);

  const sslice str = va_vfslice(self, fmt, args);

  va_end(args);

  return str;
}

sslice va_vfslice(Vallocator* self, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  i64 len = 0;
  const char* begin = va_vfstring(self, &len, fmt, args);

  if UNLIKELY (is_null(begin)) {
    return sslice_new(.begin = begin, .len = len);
  }

  return sslice_new(.begin = begin, .len = len);
}

char* va_fstring(Vallocator* self, i64* len_out, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;

  va_start(args);

  char* str = va_vfstring(self, len_out, fmt, args);
  va_end(args);
  return str;
}

METHOD
char* va_vfstring(Vallocator* self, i64* len_out, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  const i64 avail = va_available(self);
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Vallocator of size: %li bytes has no free memory!", self->mem->size);
    return nullptr;
  }

  const i32 len = min(avail, vfstring_length(fmt, args) + 1);  // +1 for null terminator!

  char* str = punwrap(va_allocate(self, mlayout_bytes(len)));

  stbsp_vsnprintf(str, len, fmt, args);

  if (len_out) {
    *len_out = len - 1;  // dont include null terminal in length calc
  }
  return str;
}

void va_destroy(Vallocator* self) {
  if (self && is_not_null(self->mem)) {
    vmem_destroy(self->mem);
    self->cursor = nullptr;
    self->mem = nullptr;
  }
}

// char* va_realpath(Vallocator* self, sslice path) {
//   static char PATH[PATH_MAX] = {};

//   const char* p = realpath(path.begin, char *restrict resolved)

// }

void va_clear(Vallocator* self) {
  assert(self);
  self->cursor = vmem_begin(self->mem);
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
                                                         .free = VT_NAME(FREE), .mask = VT__All};


Allocator vallocator(Vallocator* self) {
  return (Allocator){.ctx = self, .vtable = &VA_VT};
}
