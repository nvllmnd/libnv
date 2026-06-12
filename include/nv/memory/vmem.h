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
struct VMem {
  /// @brief does not include the size of this header
  /// @remarks used for __counted_by__ compiler attribute flexible array member
  i64 size;

  ///
  ATTR_COUNTED_BY(size)
  byte data[];
};
alias(VMem);

static constexpr const i64 VMEM_HEADER_SIZE = sizeof(VMem);

i64 os_page_size(void);

PURE_FUNC
METHOD
i64 vmem_size(const VMem* self);

PURE_FUNC
METHOD
static inline i64 vmem_size_full(const VMem* self) { return vmem_size(self) + VMEM_HEADER_SIZE; }

RETURNS_RESOURCE
VMem* vmem_new(i64 size_bytes, bool noreserve);

RETURNS_ERROR
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
RETURNS_RESOURCE
METHOD
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

PARAMS_NONNULL(1,2)
PURE_FUNC
VOffset vmem_offset(const VMem* self, const void* ptr);

PARAMS_NONNULL(1, 2)
PURE_FUNC
bool vmem_contains(const VMem* self, const void* ptr);

METHOD
static inline VMem* vmem_remap_expand(VMem* self, i64 new_size) {
  return vmem_remap(self, new_size, VRemap__ExpandInPlace);
}

METHOD
static inline VMem* vmem_remap_move(VMem* self, i64 new_size) {
  return vmem_remap(self, new_size, VRemap__AllowRelocate);
}

METHOD
void vmem_destroy(VMem* self);

/// @brief Arena-style allocator fully backed by a single VMem virutal memory mapping
/// @details for an Arena-style allocator also backed by virtual memory, but grows as mappings are exhausted, @see
/// [Arena] in arena.h
struct Vallocator {
  VMem* mem;
  byte* cursor;
};
alias(Vallocator);

typedef Vallocator VArena;

static constexpr const Vallocator VALLOC_NONE = {};

METHOD
PURE_FUNC
static inline bool va_isnone(const Vallocator* self) {
  return memcmp(self, &VALLOC_NONE, sizeof(Vallocator)) == 0;
}

METHOD
PURE_FUNC
static inline bool va_isok(const Vallocator* self) {
  return !va_isnone(self);
}

typedef Vallocator VMalloc;

METHOD
Allocator vallocator(Vallocator* self);

static inline Vallocator va_new(i64 vmem_size, bool noreserve) {
  Vallocator self = {};
  bailerr_with(vmem_init(&self.mem, vmem_size, noreserve), VALLOC_NONE);

  self.cursor = vmem_begin(self.mem);

  return self;
}


/// @brief creates new [Vallocator] with existing [VMem].
/// Created Vallocator takes ownership of given VMem, and will destroy it when/if this instance is destroyed as well
static inline Vallocator va_take(VMem* mem) {
  assert(mem);
  return (Vallocator){.mem = mem, .cursor = vmem_begin(mem)};
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
i64 va_available(const Vallocator* self);

METHOD
PURE_FUNC
i64 va_used_bytes(const Vallocator* self);

METHOD
void* va_allocate(Vallocator* self, Layout layout);

METHOD
void* va_zallocate(Vallocator* self, Layout layout);

#define va_make(_self, T) ((__typeof(T)*)va_allocate((_self), mlayout_new(T)))
#define va_alloc_array(_self, T, N) ((__typeof(T)*)va_allocate((_self), mlayout_array(T, N)))
#define va_alloc_vec(_self, T, _n) ((__typeof(T)*)va_allocate((_self), mlayout_vec(T, _n)))

PARAMS_NONNULL(1, 2)
bool va_resize(Vallocator* self, void* ptr, Layout old, Layout new);

PARAMS_NONNULL(1, 2)
void* va_reallocate(Vallocator* self, void* ptr, Layout old, Layout new);

PARAMS_NONNULL(1, 2)
char* va_strdup(Vallocator* self, const char* str);

PARAMS_NONNULL(1, 2)
char* va_strndup(Vallocator* self, const char* str, i32 len);

PARAMS_NONNULL(1, 2)
sslice va_sslice_dup(Vallocator* self, const char* str, i32 len);

HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice va_fslice(Vallocator* self, const char* fmt, ...);

METHOD
sslice va_vfslice(Vallocator* self, const char* fmt, va_list args);

HEDLEY_PRINTF_FORMAT(3, 4)
METHOD
char* va_fstring(Vallocator* self, i64* len_out, const char* fmt, ...);

METHOD
char* va_vfstring(Vallocator* self, i64* len_out, const char* fmt, va_list args);

METHOD
void va_clear(Vallocator* self);
METHOD
void va_clear_zeroed(Vallocator* self);

void va_destroy(Vallocator* self);

// METHOD
// char* va_realpath(Vallocator* self, sslice path);
