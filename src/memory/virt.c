
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
#include "nv/core/stb_sprintf.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#include "nv/memory/layout.h"
#include "nv/memory/virt.h"

#if defined(LIBNV_VMEM_WATERMARK_MAX) && LIBNV_VMEM_WATERMARK_MAX > 0
static constexpr const i32 VMEM_WATERMARK_MAX = LIBNV_VMEM_WATERMARK_MAX;
#else
static constexpr const i32 VMEM_WATERMARK_MAX = 8;
#endif

#if SYSTEM_POSIX

#endif

#if SYSTEM_WINDOWS
#include <windows.h>
//  TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)
#error "Windows not currently supported. TODO: Support windows VirtualAlloc(Ex)/VirtualFree(Ex)"
#endif

struct Watermark {
  i32 i;
  VMarker data[VMEM_WATERMARK_MAX];
};

alias(Watermark);

CONST_FUNC
static inline Watermark wm_new(void) { return (Watermark){.i = 0, .data = {}}; }

METHOD
static inline void wm_push(Watermark* self, VMarker val) {
  assert(self);
  if (self->i >= VMEM_WATERMARK_MAX || self->i < 0) {
    LOG_DBG("Watermark limit: %d reached! Wrapping around!", VMEM_WATERMARK_MAX);
    self->i = 0;
  }

  self->data[self->i] = val;
  self->i += 1;
}

static inline VMarker wm_pop(Watermark* self) {
  if (self->i == 0) {
    return -1;
  } else if (self->i < 0) {
    self->i = 0;
    return -1;
  }

  if UNLIKELY (self->i > VMEM_WATERMARK_MAX) {
    self->i = VMEM_WATERMARK_MAX;
  }
  const VMarker res = self->data[self->i - 1];
  self->i -= 1;
  return res;
}

static inline VMarker wm_peekn(const Watermark* self, i32 offset) {
  const i32 i = self->i + offset;

  if (i >= VMEM_WATERMARK_MAX) {
    return self->data[i % VMEM_WATERMARK_MAX];
  } else if (i < 0) {
    return -1;
  }
  return self->data[i];
}

static inline VMarker wm_peek(const Watermark* self) {
  assert(self);

  return wm_peekn(self, 0);
}

static inline VMarker wm_pop_peek(Watermark* self) {
  if (wm_pop(self) >= 0) {
    return wm_peek(self);
  }
  return -1;
}

struct VirtMem {
  // VModeFlags flags;

  /// @brief lock size in bytes of this mappings [mlock]ed region (spanning from 0 - lsize)
  // i32 lsize;

  /// total size of virtual memory block, in bytes.
  i64 size;

  /// aligned pointer to the next free space to use for allocation
  u8* top;
  /// pointer to the absolute end of this region of virtual memory
  u8* end;

  /// @brief highest (most recent) value returned by [vmem_mark]
  /// if [vmem_mark] has not yet been called, this value is -1
  Watermark wm;

  void* last_alloc;

  ATTR_COUNTED_BY(size)
  u8 storage[];
};

i64 os_page_size(void) {
  static i64 size = -1;
  if UNLIKELY (size < 0) {
    size = sysconf(_SC_PAGESIZE);
  }
  return size;
}

static inline void vmem_resize_in_place(VirtHndl self, void* ptr, MemLayout old, MemLayout new_layout) {
  assert(self);
  assert(ptr);

  if UNLIKELY (ptr != self->last_alloc) {
    LOG_FATAL("Cannot call vmem_expand_in_place with pointer not equal to last allocated pointer!");
  }

  const i32 delta = new_layout.size - old.size;

#if LIBNV_DEBUG
  if (delta < 0) {
    LOG_DBG("Shrinking memory of size %li bytes to %li bytes! decrementing top pointer by %d bytes!", old.size,
            new_layout.size, delta);
  } else if (delta > 0) {
    LOG_DBG("Expanding memory in place from size %li bytes to %li bytes! incrementing top pointer by %d bytes!", old.size,
            new_layout.size, delta);
  } else {
    LOG_DBG("Old size: %li and new size: %li are equal! no need to shrink or resize!", old.size, new_layout.size);
  }
#endif

  self->top += delta;
}

NvError vmem_init(VirtMem** self, i64 size_bytes) {
  assert(self);
  assert(size_bytes > 0);

  const i64 storage_size = max(size_bytes, os_page_size());
  const i64 full_size = sizeof(VirtMem) + storage_size;

  VirtMem* ptr = mmap(0, full_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if UNLIKELY (ptr == MAP_FAILED) {
    LOG_DBG("Failed to map virtual memory of size: %li from requested size: %li ERRNO(%d) :: %s", storage_size,
            size_bytes, errno, strerror(errno));

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

  ptr->last_alloc = nullptr;
  ptr->wm = wm_new();
  ptr->size = storage_size;
  ptr->top = &ptr->storage[0];
  ptr->end = ptr->top + storage_size;
  *self = ptr;

  return Error__Ok;
}

void* vmem_reallocate(VirtMem* self, void* ptr, MemLayout old, MemLayout nlayout) {
  assert(self);
  assert(ptr);

  if (self->last_alloc == ptr) {
    vmem_resize_in_place(self, ptr, old, nlayout);
    return ptr;
  }

  void* dst = vmem_allocate(self, nlayout);
  if (dst) {
    memcpy(dst, ptr, old.size);
    return dst;
  }

  return nullptr;
}

bool vmem_expand(VirtMem* self, void* ptr, MemLayout old, MemLayout nlayout) {
  assert(self);
  assert(ptr);

  if (nlayout.size <= old.size) {
    LOG_DBG(
        "Tried to expand a pointer, but new size: %li is smaller than (or equal to) old size: %li Allocator::expand is "
        "ment for "
        "in-place growing of memory!, use Allocator::reallocate to shrink!",
        nlayout.size, old.size);
    return false;
  }

  if (self->last_alloc == ptr) {
    vmem_resize_in_place(self, ptr, old, nlayout);
    return true;
  }

  LOG_DBG(
      "Cannot expand pointer of size: %li bytes to size %li bytes, as it was not the most recent allocation in this "
      "particular VirtMem! expansion currently not possible!",
      old.size, nlayout.size);

  return false;
}

void* vmem_allocate(VirtMem* self, MemLayout layout) {
  assert(self);
  const i32 size = layout.size;
  const i32 align = layout.align;

  if UNLIKELY (size > vmem_available(self)) {
    LOG_FATAL("Failed to allocate memory block of size: %d bytes. Virtual Memory only has %li bytes available", size,
              vmem_available(self));

    return nullptr;
  }

  u8* ptr = ptr_alignup(self->top, align);

  // sanity check
  assert(ptr <= self->end);

  self->top = ptr + layout.size;

  assert(self->top <= self->end);

  self->last_alloc = ptr;

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

NvError vmem_destroy(VirtHndl self) {
  assert(self);

  const isize size = self->size;

  const error err = munmap(self, size);

  if UNLIKELY (err == -1) {
    LOG_DBG("Call to %s Failed! Unable to unmap virtual memory of size: %li at address %p. ERRNO(%d) :: %s", __func__,
            size, pcast(void, self), errno, strerror(errno));

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
    LOG_DBG("Call to %s Failed! Index requested is out of range! index: %li, bounded at: %li", __func__, index,
            self->size);
    return ApiError__ValueOutOfRange;
  }

  memset(&self->storage[0], 0, index);
  return Error__Ok;
}

i64 vmem_size(const VirtMem* self) {
  assert(self);
  return self->size;
}

i64 vmem_full_size(const VirtMem* self) {
  assert(self);
  return sizeof(VirtMem) + self->size;
}

i64 vmem_used_bytes(const VirtMem* self) {
  assert(self);
  return self->top - &self->storage[0];
}

i64 vmem_available(const VirtMem* self) {
  assert(self);
  return self->end - self->top;
}

void vmem_clear_zeroed(VirtMem* self) { vmem_zero_range(self, vmem_size(self)); }

static void vmem_vtable_free(void*, void*) {}

static void* vmem_vtable_alloc(void* self, MemLayout layout) { return vmem_allocate(self, layout); }

static void* vmem_vtable_zalloc(void* self, MemLayout layout) { return vmem_zallocate(self, layout); }

static bool vmem_vtable_expand(void* ctx, void* ptr, MemLayout old, MemLayout nlayout) {
  return vmem_expand(ctx, ptr, old, nlayout);
}

static void* vmem_vtable_realloc(void* ctx, void* ptr, MemLayout old, MemLayout newlayout) {
  return vmem_reallocate(ctx, ptr,  old,  newlayout);
}

const AllocVTable* vmem_vtable(void) {
  static constexpr const AllocVTable VT = (AllocVTable){.allocate = vmem_vtable_alloc,
                                                        .reallocate = vmem_vtable_realloc,
                                                        .zallocate = vmem_vtable_zalloc,
                                                        .free = vmem_vtable_free,
                                                        .expand = vmem_vtable_expand,
                                                        .mask = VT__All};
  return &VT;
}

Allocator vmem_allocator(VirtMem* self) { return (Allocator){.ctx = self, .vtable = vmem_vtable()}; }

bool vmem_contains(const VirtMem* self, const void* p) {
  assert(self);
  assert(self->end);
  assert(p);

  const u8* ptr = p;

  return ptr >= &self->storage[0] && ptr < self->end;
}

VirtMemView vmem_view(const VirtMem* self) {
  return (VirtMemView){.start = &self->storage[0],
                       .end = self->end,
                       .avail_bytes = vmem_available(self),
                       .used_bytes = vmem_used_bytes(self),
                       .size_bytes = self->size};
}

VMarker vmem_watermark(const VirtMem* self) {
  assert(self);

  return wm_peek(&self->wm);
}

VMarker vmem_pop_delete(VirtSelf self, VMarker marker) {
  assert(self);

  const VMarker wm = wm_peek(&self->wm);
  if (marker != wm) {
    LOG_DBG("Tried to pop delete marker: %li when most recent Watermark is (%li)", marker, wm);
    return wm;
  }

  vmem_reset_to(self, marker);
  return wm_pop_peek(&self->wm);
}

VMarker vmem_pop_zeroed(VirtSelf self, VMarker marker) {
  assert(self);
  const VMarker top = wm_peek(&self->wm);

  if (top != VMARKER_NONE && marker != top) {
    LOG_DBG("Tried to pop delete (zeroed) marker: %li when most recent Watermark is (%li)", marker, top);
    return top;
  }

  vmem_reset_zeroed(self, marker);
  return wm_pop_peek(&self->wm);
}

VMarker vmem_mark(VirtMem* self) {
  assert(self);
  assert(self->top);
  const VMarker m = self->top - &self->storage[0];

  assert(m >= 0 && m < self->size);
  wm_push(&self->wm, m);

  return m;
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
  const i64 delta_size = self->top - ntop;

  assert(ntop <= self->top);
  memset(ntop, 0, delta_size);

  self->top = ntop;
}

NvError vmem_lock(VirtMem* self, i64 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = mlock(self, nbytes);

  LOG_DBG("Called mlock with %li bytes", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to mlock with %li bytes Failed!", nbytes);
    return Error__VMapCannotLockToRAM;
  }
  return Error__Ok;
}

NvError vmem_unlock(VirtMem* self, i64 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = munlock(self, nbytes);

  LOG_DBG("Called munlock with %li bytes", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to munlock with %li bytes Failed!", nbytes);
    return Error__CannotUnlockRAM;
  }
  return Error__Ok;
}

NvError vmem_will_need(VirtMem* self, i64 nbytes) {
  assert(self);
  assert(nbytes > 0);
  const i32 err = madvise(self, nbytes, MADV_WILLNEED);
  LOG_DBG("Called madvise with %li bytes and flag MADV_WILLNEED", nbytes);

  if (err != 0) {
    ELOG_DBG("Called to madvise with %li bytes and flag MADV_WILLNEED Failed!", nbytes);
    return Error__MAdviseWillNeedFailed;
  }
  return Error__Ok;
}

NvError vmem_init_ex(VirtMem** self, VirtMemOpts opts) {
  assert(self);
  assert(opts.size_bytes > 0);

  const i64 size = max(opts.size_bytes, os_page_size());

  if (opts.mode == VMap__PrivAnon) {
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
    LOG_DBG("Failed to map virtual memory of size: %li bytes. ERRNO(%d) :: %s", size, errno, strerror(errno));

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
    const i64 bytes = opts.commit_bytes < 0 ? size : opts.commit_bytes;
    return vmem_will_need(*self, bytes);
  }

  if (bithas(opts.mode, VMap__LockPages) && opts.lock_bytes != 0) {
    const i64 bytes = opts.lock_bytes < 0 ? size : opts.lock_bytes;
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
    LOG_FATAL("rel_address: %li is out of range! VirtMem size is only %li bytes!", rel_address, self->size);
  }

  return pcast(void, &self->storage[rel_address]);
}

NvError vmem_remap(VirtMem** s, i64 size_bytes, VRemapMode mode) {
  assert(s);

  VirtMem* self = *s;

  i32 flags = 0;

  if (mode == VRemap__AllowRelocate) {
    bitset(flags, MREMAP_MAYMOVE);
  }
  const i64 size = size_bytes + sizeof(VirtMem);

  VirtMem* new_self = mremap(self, self->size, size, flags);
  // NOTE: No UNLIKELY here as a common operation could be to try to expand in place, if that fails caller may call
  // again allowing relocation
  if (new_self == MAP_FAILED) {
    if (mode == VRemap__AllowRelocate) {
      ELOG_DBG("Virtual Mapping allows reloction, but failed to remap from size: %li bytes to %li bytes", self->size,
               size);
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

  i64 len = 0;
  const char* begin = vmem_vfstring(self, &len, fmt, args);

  return sslice_new(.begin = begin, .len = len);
}

char* vmem_fstring(VirtMem* self, i64* len_out, const char* fmt, ...) {
  va_list args;
  va_start(args);

  char* str = vmem_vfstring(self, len_out, fmt, args);

  va_end(args);
  return str;
}

char* vmem_vfstring(VirtMem* self, i64* len_out, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  const i64 avail = vmem_available(self);

  if UNLIKELY (avail <= 0) {
    return nullptr;
  }

  const i32 len = min(avail, vfstring_length(fmt, args) + 1);  // + 1 for null-terminator

  char* str = punwrap(vmem_allocate(self, mlayout_bytes(len)));
  // NOTE: Since we limit this string to the available memory in this virtmem, the above [vmem_allocate] call should
  // never fail

  stbsp_vsnprintf(str, len, fmt, args);

  if (len_out) {
    *len_out = len - 1;  // dont include terminal null character in the length we report!
  }

  return str;
}

i64 vmem_delete_back(VirtMem* self, i64 nbytes) {
  assert(self);
  const i64 used = vmem_used_bytes(self);

  if (nbytes > used) {
    nbytes = used;
  }

  self->top -= nbytes;
  return vmem_available(self);
}

i64 vmem_delzero_back(VirtMem* self, i64 nbytes) {
  assert(self);
  const i64 used = vmem_used_bytes(self);
  if (nbytes > used) {
    nbytes = used;
  }

  self->top -= nbytes;

  memset(self->top, 0, nbytes);
  return vmem_available(self);
}

char* vmem_strndup(VirtSelf self, const char* str, i32 len) {
  assert(self);
  assert(str);
  assert(len > 0);

  const i64 avail = vmem_available(self) - 1;  // -1 for null term!

  if UNLIKELY (avail <= 0) {
    LOG_DBG("String: %.*s of length: %d cannot be allocated by VirtMem with 0 free bytes available!", len, str, len);
    return nullptr;
  }

  if UNLIKELY (len > avail) {
    LOG_DBG("String: %.*s of length: %d will be truncated to: %.*s due to VirtMem only having %d bytes available!", len,
            str, len, (i32)avail, str, (i32)avail);
    len = avail;
  }

  // should always be non-null since we truncate string if its too large
  char* res = punwrap(vmem_allocate(self, mlayout_bytes(len + 1)));

  strncpy(res, str, len);
  res[len] = '\0';

  return res;
}

char* vmem_strdup(VirtSelf self, const char* str) {
  const i64 avail = vmem_available(self) - 1;  // -1 for null term

  if UNLIKELY (avail <= 0) {
    LOG_DBG("String: %s cannot be allocated by VirtMem with 0 free bytes available!", str);
    return nullptr;
  }

  i32 len = stringlen(str) + 1;  // +1 so we copy the null terminator!

  const bool truncate = len > avail;

  if UNLIKELY (truncate) {
    LOG_DBG("String: %.*s of length: %d will be truncated to: %.*s due to VirtMem only having %d bytes available!", len,
            str, len, (i32)avail, str, (i32)avail);
    len = avail;
  }

  // should always be non-null since we truncate string if its too large
  char* res = punwrap(vmem_allocate(self, mlayout_bytes(len)));

  strncpy(res, str, len);
  // only need to append null term if we truncated
  if UNLIKELY (truncate) {
    res[len] = '\0';
  }

  return res;
}
