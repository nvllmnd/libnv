// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <assert.h>
#include <string.h>

#include "nv/core/attributes.h"
#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"


/// @brief allocated virtual memory
/// @details Header is kept small so this type is easier to extend, its also harder to
/// accidently add redundant data as callers can see the full impl
///
///
struct nv_nodiscard_msg("Hey! Where did my memory go? ;P Seriously tho dont ignore this!") VMem {
  i64 size;
  /// @brief if negative or 0, this field is ignored,
  /// otherwise, it is used for begin/end calculations,
  /// so that callers can add their own metadata and not worry about it getting updated by allocations

  ATTR_COUNTED_BY(size)
  byte data[];
};
alias(VMem);



/// @brief magic number used to mark VMem created with custom header data
/// @details location: (&VMem::data[0]) is set to this magic value when 
static constexpr const i64 VMEM_PREFIX_SIZE = sizeof(VMem);

i64 os_page_size(void);


PURE_FUNC
METHOD
static inline i64 vmem_size(const VMem* self) {
  assert(self);
  return self->size;
}

PURE_FUNC
METHOD
static inline i64 vmem_size_full(const VMem* self) { return vmem_size(self) + VMEM_PREFIX_SIZE; }

#ifndef LIBNV_VMEM_NORESERVE_DEFAULT
#define LIBNV_VMEM_NORESERVE_DEFAULT 0
#endif

static constexpr const bool VMEM_NORESERVE_DEFAULT = cast(bool, LIBNV_VMEM_NORESERVE_DEFAULT);

/// @brief create new VMem with options
/// @details has noreserve flag instead of options tentatively
VMem* vmem_new_ex(i64 size_bytes, bool noreserve);

/// @brief Creates a new VMem of given size and default options
/// @details Currently vmem_new_ex only takes a single extra argument than
/// this function, but that may change in the future
///
/// @returns nullptr if any errors occur during creation
VMem* vmem_new(i64 size_bytes);

/// @brief initializes a nullptr to a new instance of VMem
/// @details NvError is a mask that may contain [ERROR_COUNT] error states
NvError vmem_init(VMem** self, i64 size_bytes, bool noreserve) METHOD;

typedef enum VRemapMode {
  VRemap__ExpandInPlace = 0,
  VRemap__AllowRelocate,
} VRemapMode;

/// @brief remaps virtual memory used by self to given new size
///
/// @returns pointer to new remapped [VirtMem]. If [VRemap__ExpandInPlace] is passed to @param (VRemapMode mode), then
/// a non-null pointer indicates the remaping was successfully done in place, nullptr indicates that
/// the attempt to expand/shrink VMem inplace failed, you can still call this function again with the
/// [VRemap__AllowRelocate] (which will most likely succeed), but that may not be your desired behavior
VMem* vmem_remap(VMem* self, i64 new_size, VRemapMode mode);

RETURNS_NON_NULL
METHOD
static inline byte* vmem_begin(VMem* self) {
  assert(self);
  return &self->data[0];
}

RETURNS_NON_NULL
METHOD
static inline const byte* vmem_cbegin(const VMem* self) {
  assert(self);
  return &self->data[0];
}

RETURNS_NON_NULL
METHOD
static inline byte* vmem_end(VMem* self) {
  assert(self);
  // dont index the end so we don't accidently cause a page fault at the end of virtual memory!
  return vmem_begin(self) + self->size;
}

RETURNS_NON_NULL
METHOD
static inline const byte* vmem_cend(const VMem* self) {
  assert(self);
  // dont index the end so we don't accidently cause a page fault at the end of virtual memory!
  return vmem_cbegin(self) + self->size;
}

typedef i64 VOffset;

VOffset vmem_offset(const VMem* self, const void* ptr) PARAMS_NONNULL(1, 2) METHOD PURE_FUNC;

bool vmem_contains(const VMem* self, const void* ptr) PARAMS_NONNULL(1, 2) METHOD PURE_FUNC;

METHOD
static inline VMem* vmem_remap_expand(VMem* self, i64 new_size) {
  return vmem_remap(self, new_size, VRemap__ExpandInPlace);
}

METHOD
static inline VMem* vmem_remap_move(VMem* self, i64 new_size) {
  return vmem_remap(self, new_size, VRemap__AllowRelocate);
}

NvError vmem_ram_lock(VMem* self, void* from, i64 size) METHOD;
NvError vmem_ram_release(VMem* self, void* from, i64 size) METHOD;

NvError vmem_prefault_range(VMem* self, void* from, i64 size) METHOD;

void vmem_destroy(VMem* self) METHOD;

/// @brief Arena-style allocator fully backed by a single VMem virutal memory mapping
/// @details for an Arena-style allocator also backed by virtual memory, but grows as mappings are exhausted, @see [Arena] in arena.h
struct Vallocator {
  VMem* mem;
  IterByte iter;
};
alias(Vallocator);

typedef Vallocator VArena;

static constexpr const Vallocator VALLOC_NONE = {};

METHOD
PURE_FUNC
static inline bool va_isnone(const Vallocator* self) { return memcmp(self, &VALLOC_NONE, sizeof(Vallocator)) == 0; }

METHOD
PURE_FUNC
static inline bool va_isok(const Vallocator* self) { return !va_isnone(self); }

typedef Vallocator VMalloc;

METHOD
Allocator va_allocator(Vallocator* self);

Vallocator va_new_ex(i64 vmem_size, bool noreserve);

static inline Vallocator va_new(i64 vmem_size) { return va_new_ex(vmem_size, VMEM_NORESERVE_DEFAULT); }

/// @brief creates new [Vallocator] with existing [VMem].
/// Created Vallocator takes ownership of given VMem, and will destroy it when/if this instance is destroyed as well
static inline Vallocator va_take(VMem* mem) {
  assert(mem);
  byte* begin = vmem_begin(mem);
  byte* end = vmem_end(mem);
  return (Vallocator){.mem = mem, .iter = (IterByte){.begin = begin, .cursor = begin, .end = end}};
}

/// @brief checkpoint marker used to reset memory back to an earlier point
typedef i64 VMark;

METHOD
PURE_FUNC
VMark va_checkpoint(const Vallocator* self);

METHOD
i64 va_reset_to(Vallocator* self, VMark mark);

METHOD
PURE_FUNC
static inline i64 va_available(const Vallocator* self) {
  assert(self);
  return iter_tail(self->iter);
}

METHOD
PURE_FUNC
static inline i64 va_used_bytes(const Vallocator* self) {
  assert(self);
  return iter_head(self->iter);
}

METHOD
static inline void* va_allocate(Vallocator* self, Layout layout) { return allocate_raw(&self->iter, layout); }

METHOD
static inline void* va_zallocate(Vallocator* self, Layout layout) { return zallocate_raw(&self->iter, layout); }

PARAMS_NONNULL(1, 2)
static inline bool va_resize(Vallocator* self, void* ptr, Layout old, Layout new) {
  return resize_raw(&self->iter, ptr, old, new);
}

PARAMS_NONNULL(1, 2)
static inline void* va_reallocate(Vallocator* self, void* ptr, Layout old, Layout new) {
  return reallocate_raw(&self->iter, ptr, old, new);
}

PARAMS_NONNULL(1, 2)
static inline char* va_strdup(Vallocator* self, const char* str) { return strdup_raw(&self->iter, str); }

PARAMS_NONNULL(1, 2)
static inline char* va_strndup(Vallocator* self, const char* str, i32 len) {
  return strndup_raw(&self->iter, str, len);
}

PARAMS_NONNULL(1, 2)
static inline sslice va_sslice_dup(Vallocator* self, const char* str, i32 len) {
  return sslice_dup_raw(&self->iter, str, len);
}

PARAMS_NONNULL(1, 2)
static inline sslice va_vfslice(Vallocator* self, const char* fmt, va_list args) {
  return vfslice_raw(&self->iter, fmt, args);
}

PARAMS_NONNULL(1, 3)
static inline char* va_vfstring(Vallocator* self, i64* len_out, const char* fmt, va_list args) {
  return vfstring_raw(&self->iter, len_out, fmt, args);
}

#define va_make(_self, T) ((__typeof(T)*)va_allocate((_self), mlayout_new(T)))
#define va_alloc_array(_self, T, N) ((__typeof(T)*)va_allocate((_self), mlayout_array(T, N)))
#define va_alloc_vec(_self, T, _n) ((__typeof(T)*)va_allocate((_self), mlayout_vec(T, _n)))

HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice va_fslice(Vallocator* self, const char* fmt, ...);

HEDLEY_PRINTF_FORMAT(3, 4)
METHOD
char* va_fstring(Vallocator* self, i64* len_out, const char* fmt, ...);

METHOD
void va_clear(Vallocator* self);

METHOD
void va_clear_zeroed(Vallocator* self);

void va_destroy(Vallocator* self);

#define vmem_meta_new(_size, _meta) ({\
  VMem* _self = vmem_new((_size));\
  if (is_not_null(_self)) {\
    \
  }\
})

