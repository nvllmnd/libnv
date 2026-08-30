// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/memory/alloc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "nv/core/algo.h"
#include "nv/core/log.h"
#include "nv/core/stb_sprintf.h"
#include "nv/core/system.h"
#include "nv/iter/iterators.h"
#include "nv/memory/error.h"
#include "nv/memory/vmem.h"

void* vtable_realloc_no_impl(void*, void*, Layout, Layout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, Layout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, Layout, Layout) { return nullptr; }

void* ptr_expect_(const void* ptr, const char* msg) {
  if UNLIKELY (nullptr == ptr) {
    log_fatal("%s", msg);
  }
  // NOTE: We dont mutate this pointer at all, so its safe to cast this back to non-const, since
  // we cast it back to exactly the same type as the pointer was before being passed to this function though the
  // implementation macro
  return pcast(void, ptr);
}

isize ptr_align_offset(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const u64ptr mask = align - 1;
  return cast(isize, cast(u64ptr, ptr) & mask);
}

bool ptr_is_aligned(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

void* ptr_alignup(void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  LOG_DBG("Pointer: %p not currently aligned! aligning to: %p", ptr, (void*)aligned);

  return pcast(void, aligned);
}

u8* ptr_alignto(u8* ptr, u8* end, Layout layout) {
  assert(ptr);
  assert(end);
  assert(end >= ptr);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  u8* const top = ptr_alignup(ptr, layout.align);

  if (top + layout.size >= end) {
    return nullptr;
  }

  return top;
}

u8* ptr_alignin(u8* ptr, i32* space, Layout layout) {
  assert(ptr);
  assert(space);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  const i32 avail = *space;

  if (avail < layout.size) {
    return nullptr;
  }

  u8* const end = ptr + *space;

  u8* const aligned = ptr_alignto(ptr, end, layout);

  if (is_null(aligned)) {
    return nullptr;
  }

  const i32 delta = aligned - ptr;
  *space -= delta;

  return aligned;
}

void* ptr_nonnull_(const void* ptr) {
  return ptr_expect_(ptr, " Expected given pointer to be non-null, but was nullptr! Aborting program!");
}
void* allocate_raw(IterByte* self, const Layout layout) {
  assert(self->cursor);

  byte* ptr = (byte*)ptr_alignup(self->cursor, layout.align);
  byte* next = ptr + layout.size;

  if UNLIKELY (next >= self->end) {
    LOG_ERROR("Iterator Raw Allocation Failed! Not enough space left between cursor and end of byte iterator!");
    return nullptr;
  }

  self->cursor = next;
  return ptr;
}

void* zallocate_raw(IterByte* self, const Layout layout) {
  void* ptr = allocate_raw(self, layout);
  if UNLIKELY (is_null(ptr)) {
    return ptr;
  }

  memset(ptr, 0, layout.size);
  return ptr;
}

bool resize_raw(IterByte* self, void* ptr, Layout old, Layout new) {
  assert(self);
  assert(ptr);
  if (old.size == new.size) {
    return true;
  }
  byte* last = (self->cursor - old.size);
  if (last == (byte*)ptr) {
    // Shrink if newsize is greater oldsize, otherwise grow
    const i64 delta = new.size - old.size;
    self->cursor += delta;

    return true;
  }

  return false;
}

void* reallocate_raw(IterByte* self, void* ptr, Layout old, Layout new) {
  if (resize_raw(self, ptr, old, new)) {
    return ptr;
  }

  if (new.size < old.size) {
    return ptr;
  }

  void* res = allocate_raw(self, new);
  if UNLIKELY (is_null(res)) {
    LOG_ERROR("Failed to Reallocate Memory of size %li bytes to size %li bytes", old.size, new.size);
    return nullptr;
  }

  memcpy(res, ptr, old.size);

  return res;
}

void free_raw(IterByte* self, void* ptr, Layout layout, u64 pattern) {
  assert(self);

  if (ptr && span_contains(*self, ptr)) {
    memset(ptr, pattern, layout.size);
  }
}

char* strdup_raw(IterByte* self, const char* str) {
  const i64 len = stringlen(str);
  return strndup_raw(self, str, len);
}

char* strndup_raw(IterByte* self, const char* str, i32 len) {
  assert(self);
  assert(str);

  const i64 avail = iter_tail(*self) - 1;
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Cannot dup string: %.*s of length %d in Vallocator with only %li bytes available!", len, str, len,
              avail);
    return nullptr;
  }
  if UNLIKELY (len > avail) {
    DWARN("String: %.*s of length: %d will be truncated to %.*s to fit inside vallocator with %li bytes available!",
          len, str, len, (i32)avail, str, avail);
    len = avail;
  }

  char* ptr = allocate_raw(self, mlayout_bytes(len + 1));

  strncpy(ptr, str, len);
  ptr[len] = 0;
  return ptr;
}

sslice sslice_dup_raw(IterByte* self, const char* str, i32 len) {
  const char* ptr = strndup_raw(self, str, len);
  if UNLIKELY (is_null(ptr)) {
    return sslice_empty();
  }
  return sslice_new(.begin = ptr, .len = len);
}

sslice fslice_raw(IterByte* self, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;
  va_start(args);

  const sslice str = vfslice_raw(self, fmt, args);

  va_end(args);

  return str;
}

sslice vfslice_raw(IterByte* self, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  i64 len = 0;
  const char* begin = vfstring_raw(self, &len, fmt, args);

  if UNLIKELY (is_null(begin)) {
    return sslice_new(.begin = begin, .len = len);
  }

  return sslice_new(.begin = begin, .len = len);
}

char* fstring_raw(IterByte* self, i64* len_out, const char* fmt, ...) {
  assert(self);
  assert(fmt);

  va_list args;

  va_start(args);

  char* str = vfstring_raw(self, len_out, fmt, args);
  va_end(args);
  return str;
}

char* vfstring_raw(IterByte* self, i64* len_out, const char* fmt, va_list args) {
  assert(self);
  assert(fmt);

  const i64 avail = iter_tail(*self);
  if UNLIKELY (avail <= 0) {
    LOG_ERROR("Byte Iterator used for string formatting of size: %li bytes has no free memory!", itersize(*self));
    return nullptr;
  }

  const i32 len = vfstring_length(fmt, args) + 1;  // +1 for null terminator!

  char* str = allocate_raw(self, mlayout_bytes(len));
  if UNLIKELY (is_null(str)) {
    LOG_ERROR("Failed to allocate formatted string: %s, with expanded size: %li, only %li bytes left available!", fmt,
              len, avail);
    return nullptr;
  }

  const i64 n = stbsp_vsnprintf(str, len, fmt, args);
  if UNLIKELY (n == -1) {
    DERROR("stbsp_vsnprintf returned -1!");
  }

  if (len_out) {
    *len_out = len - 1;  // dont include null terminal in length calc
  }
  return str;
}

NvArena arena_new(byte* begin, isize size) {
  assert(begin);
  assert(size > 0);

  byte* end = begin + size;
  return arena_range_new(begin, end);
}

NvArena arena_vmem_new(struct VMem* vm) {
  assert(vm);
  LOG("Creating Arena with VMem of size: %li at offset 0!", vm->size);
  NvArena res = arena_vmem_at_new(vm, vm->size - 1, 0);

  return res;
}

NvArena arena_vmem_at_new(struct VMem* vm, isize size, isize offset) {
  assert(vm);
  assert(size > 0);
  assert(offset >= 0);
  if UNLIKELY (offset + size > vm->size) {
    LOG_ERROR("Offset :%li + size: %li overflows VMem size: %li!", offset, size, vm->size);
    return ARENA_NONE;
  }
  byte* vm_end = vmem_end(vm);
  byte* begin = vmem_begin(vm) + offset;

  if UNLIKELY (begin >= vm_end) {
    LOG_FATAL("Offset calculation overflow!");
  }
  // assert(begin < vm_end);

  byte* end = begin + size;
  assert(end < vm_end);
  return arena_range_new(begin, end);
}

NvArena arena_va_new(struct Vallocator* va, isize size) {
  const i64 avail = va_available(va);
  if UNLIKELY (size > avail) {
    return ARENA_NONE;
  }

  byte* begin = va_allocate(va, mlayout_bytes(size));
  byte* end = begin + size;
  return arena_range_new(begin, end);
}

NvArena arena_new_in(Allocator alloc, isize size) {
  byte* begin = allocator_allocate(alloc, mlayout_bytes(size));
  if UNLIKELY (is_null(begin)) {
    return ARENA_NONE;
  }
  byte* end = begin + size;
  return arena_range_new(begin, end);
}

void* arena_alloc(NvArena* self, Layout layout) {
  IterByte iter = arena_iter(self);
  void* ptr = allocate_raw(&iter, layout);
  if UNLIKELY (is_null(ptr)) {
    LOG_ERROR(
        "Arena of size: %li only has %li bytes available and cannot allocate object of size :%li and alignment: %li",
        arena_size(self), arena_avail(self), layout.size, layout.align);
    return nullptr;
  }
  self->cursor = iter.cursor;
  return ptr;
}

void* arena_zalloc(NvArena* self, Layout layout) {
  void* ptr = arena_alloc(self, layout);
  if UNLIKELY (is_null(ptr)) {
    return ptr;
  }
  memset(ptr, 0, layout.size);
  return ptr;
}

bool arena_resize(NvArena* self, void* ptr, Layout old, Layout new) {
  IterByte iter = arena_iter(self);
  const bool res = resize_raw(&iter, ptr, old, new);
  self->cursor = iter.cursor;
  return res;
}

void* arena_realloc(NvArena* self, void* ptr, Layout old, Layout new) {
  IterByte iter = arena_iter(self);
  void* res = reallocate_raw(&iter, ptr, old, new);
  self->cursor = iter.cursor;
  return res;
}

char* arena_fstring(NvArena* self, isize* slen_out, const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  char* ptr = arena_vfstring(self, slen_out, fmt, args);

  va_end(args);
  return ptr;
}

PARAMS_NONNULL(1, 3)
char* arena_vfstring(NvArena* self, isize* slen_out, const char* fmt, va_list args) {
  IterByte iter = arena_iter(self);
  char* ptr = vfstring_raw(&iter, slen_out, fmt, args);

  if UNLIKELY (is_null(ptr)) {
    return nullptr;
  }

  self->cursor = iter.cursor;

  return ptr;
}

PARAMS_NONNULL(1, 2)
char* arena_strndup(NvArena* self, const char* str, isize len) {
  return arena_fstring(self, nullptr, "%.*s", (i32)len, str);
}

METHOD
sslice arena_strdup(NvArena* self, sslice str) {
  const char* ptr = arena_strndup(self, str.begin, str.len);

  return sslice_new(.begin = ptr, .len = str.len);
}

void arena_clone(const NvArena* src, NvArena dest) {
  assert(src);
  assert(!is_none(&dest));
  memcpy(dest.begin, src->begin, arena_size(&dest));
}

void arena_clear(NvArena* self) { self->cursor = self->begin; }

void arena_clear_zeroed(NvArena* self) {
  const isize used = arena_used_bytes(self);
  memset(self->begin, 0L, used);
  arena_clear(self);
}

void arena_free(NvArena*, void*) {}

#define VT_DEFINE(_rest, _name) static VT_DEFINE_AS(NvArena, _rest, _name)

VT_DEFINE(ALLOC, arena_alloc)
VT_DEFINE(ZALLOC, arena_zalloc)
VT_DEFINE(REALLOC, arena_realloc)
VT_DEFINE(RESIZE, arena_resize)
VT_DEFINE(FREE, arena_free)

#define VT_NAME(_ty) VT_NAMEOF(NvArena, _ty)

static constexpr const AllocVTable ARENA_VT = (AllocVTable){.allocate = VT_NAME(ALLOC),
                                                            .zallocate = VT_NAME(ZALLOC),
                                                            .resize = VT_NAME(RESIZE),
                                                            .reallocate = VT_NAME(REALLOC),
                                                            .free = VT_NAME(FREE),
                                                            .mask = VT__All};

Allocator arena_allocator(NvArena* self) { return (Allocator){.ctx = self, .vtable = &ARENA_VT}; }

const char* arena_fread_string(NvArena* self, const char* path, isize* file_size_out) {
  FILE* file = fopen(path, "r");
  if (is_null(file)) {
    DERROR("Failed to open file at path: %li in readonly mode!");
    return nullptr;
  }
  fseek(file, 0, SEEK_END);
  const isize file_size = ftell(file);
  rewind(file);

  const Layout layout = mlayout_bytes(file_size + 1);
  char* buf = arena_alloc(self, layout);
  if UNLIKELY (is_null(buf)) {
    LOG_ERROR(
        "Failed to read file at path: %s into memory! File size requires allocation of %li bytes, but only %li bytes "
        "left available in this Arena!",
        path, file_size, arena_avail(self));
    fclose(file);
    return nullptr;
  }

  const isize n = fread(buf, 1, file_size, file);
  if (n != file_size) {
    DERROR(
        "Failed to read file at path: %s into memory! fread returned: %li but we expected it to return: %li! Possible "
        "truncation or error!",
        path, n, file_size);

    assert(arena_alloc_undo(self, buf, layout));
    fclose(file);
    return nullptr;
  }

  if (file_size_out) {
    *file_size_out = file_size;
  }

  fclose(file);
  return buf;
}

bool arena_alloc_undo(NvArena* self, void* ptr, Layout layout) {
  assert(self);
  // requested pointer layout is larger than the number of bytes we currently have allocated, this forsure isnt our
  // pointer
  if UNLIKELY (layout.size < arena_used_bytes(self)) {
    return false;
  }
  if (!arena_contains(self, ptr)) {
    return false;
  }
  byte* last = self->cursor -= layout.size;
  if (ptr == last) {
    self->cursor = last;
    return true;
  }
  return false;
}

bool arena_alloc_undo_zeroed(NvArena* self, void* ptr, Layout layout) {
  if (arena_alloc_undo(self, ptr, layout)) {
    memset(self->cursor, 0L, layout.size);
    return true;
  }
  return false;
}
