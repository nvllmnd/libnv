// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <assert.h>

#include "nv/common.h"
#include "nv/iter/iterators.h"

/// Simple struct used for sizing memory allocations, inspired from Rust's Layout type
struct Layout {
  /// Size of requested allocation in bytes. Must be non-negative and greater than 0
  isize size;
  /// Alignment of requested allocation. must be a multiple of 2 (or the value 1)
  isize align;
};
typedef struct Layout Layout;

#define mlayout_static(s, a)                          \
  ({                                                  \
    constexpr const __typeof(s) _s = (s);             \
    constexpr const __typeof(a) _a = (a);             \
    static_assert(IS_POWER_OF_2(_a) && _s % _a == 0); \
    make(Layout, .size = _s, .align = _a);            \
  })

#define mlayout_new(T) (mlayout_static(sizeof(T), alignof(T)))
#define mlayout_array(T, N) (mlayout_static(sizeof(T) * N, alignof(T)))
#define mlayout_vec(T, _n) (make(Layout, .size = sizeof(T) * (_n), .align = alignof(T)))
#define mlayout_fma(THeader, flex_member_size) \
  (make(Layout, .size = sizeof(THeader) + (flex_member_size), .align = alignof(THeader)))

/// @brief Creates a new [Layout] appropriate for allocating a buffer of bytes of size `nbytes`
/// @param(i32 nbytes) size in bytes of allocation request. Must be > 0
CONST_FUNC
static inline Layout mlayout_bytes(i32 nbytes) {
  assert(nbytes > 0);
  return make(Layout, .size = nbytes, .align = 1);
}

/// @brief exteneds Layout by count. (if MemLayout represents a single element of a typed array, then MemLayout * count
/// is the MemLayout of that typed array)
CONST_FUNC
static inline Layout mlayout_extend(Layout self, i32 count) {
  assert(count > 0);
  return (Layout){.size = self.size * count, .align = self.align};
}

/// @brief Creates a new MemLayout calculated as such: multiplies self.size * count and adds the rhs.size to the result,
/// takes max of self and rhs alignment
CONST_FUNC
static inline Layout mlayout_extend_with(Layout self, i32 count, Layout rhs) {
  assert(count > 0);
  return (Layout){.size = (self.size * count) + rhs.size, .align = max(self.align, rhs.align)};
}

/// @brief lhs.size + rhs.size, align = max(lhs.align, rhs.align)
CONST_FUNC
static inline Layout mlayout_add(Layout lhs, Layout rhs) {
  return (Layout){.size = lhs.size + rhs.size, .align = max(lhs.align, rhs.align)};
}

/// Function pointer typedef for [Allocator] [AllocVTable] allocate method.
///
/// void* self  - Pointer to self (may be null if Allocator has no state!)
///              Typically you will cast this to your derived Allocator type in this method implementation
/// MemLayout layout - layout of requested allocation. see [MemLayout]
///
typedef void* (*const VTableAllocate)(void* self, Layout layout);
/// Function pointer typedef for [Allocator] [AllocVTable] reallocate method
///
/// void* self     - Pointer to self (may be null if Allocator has no state!)
///                  Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr      - Pointer to begging of block of memory to be reallocated
/// isize new_size - Size of requested reallocation in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
/// @returns Implementations should return nullptr on failure, otherewise a poitner to the start of reallocated memory
typedef void* (*const VTableReallocate)(void* self, void* ptr, Layout old_layout, Layout new_layout);

/// Function pointer typedef for [Allocator] [AllocVTable] zallocate method
/// This is the same as [VTableAllocate], but ensures allocated memory is zeroed
///
/// void* self  - Pointer to self (may be null if Allocator has no state!)
///               Typically you will cast this to your derived Allocator type in this method implementation
/// usize size  - Size of allocation requested in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
typedef void* (*const VTableZallocate)(void* self, Layout layout);

/// Function pointer typedef for [Allocator] [AllocVTable] free method.
///
/// void* self - Pointer to self (may be null if Allocator has no state!)
///              Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr  - Pointer to block of memory to be freed by this allocator
typedef void (*const VTableFree)(void* self, void* ptr);

/// Returns true if @param (void* ptr) was successfully expanded in place
typedef bool (*const VTableResize)(void* self, void* ptr, Layout old_layout, Layout new_layout);

typedef enum HEDLEY_FLAGS AllocVTableMask : u8 {
  VT__Allocate = 1,
  VT__Reallocate = 1 << 1,
  VT__Zallocate = 1 << 2,
  VT__Expand = 1 << 3,
  VT__Free = 1 << 4,

  VT__Required = VT__Allocate | VT__Free,
  VT__AllocZallocFree = VT__Required | VT__Zallocate,
  VT__AllocReallocExpandFree = VT__Required | VT__Reallocate | VT__Expand,
  VT__AllocReallocZallocFree = VT__AllocZallocFree | VT__Reallocate,
  VT__All = VT__Allocate | VT__Reallocate | VT__Zallocate | VT__Expand | VT__Free,
} HEDLEY_FLAGS AllocVTableMask;

CONST_FUNC
static inline bool vtmask_has_alloc(AllocVTableMask mask) { return bithas(mask, VT__Allocate); }

CONST_FUNC
static inline bool vtmask_has_realloc(AllocVTableMask mask) { return bithas(mask, VT__Reallocate); }

CONST_FUNC
static inline bool vtmask_has_zalloc(AllocVTableMask mask) { return bithas(mask, VT__Zallocate); }

CONST_FUNC
static inline bool vtmask_has_expand(AllocVTableMask mask) { return bithas(mask, VT__Expand); }

CONST_FUNC
static inline bool vtmask_has_free(AllocVTableMask mask) { return bithas(mask, VT__Free); }

/// Tests that a mask has the the needed allocation methods implemented (only allocate and free are mandatory)
CONST_FUNC
static inline bool vtmask_is_ok(AllocVTableMask mask) { return bithasall(mask, VT__Allocate | VT__Free); }

/// [Allocator] VTable struct that contains function pointers
/// to Allocator implementations
struct AllocVTable {
  /// See [VTableAllocate]
  VTableAllocate allocate;
  /// See [VTableReallocate]
  VTableReallocate reallocate;
  /// See [VTableZallocate]
  VTableZallocate zallocate;
  /// See [VTableFree]
  VTableFree free;

  VTableResize resize;

  /// VTable Mask, Allocators can set bits related to the allocation methods that they support, to avoid having to
  /// make  a funciton call. This also is more clear to the caller which functions they can use.
  AllocVTableMask mask;
};
typedef struct AllocVTable AllocVTable;

// CONST_FUNC
// const AllocVTable* global_allocator_vtable(void);

// void* vtable_alloc_no_impl(void*, MemLayout);
void* vtable_realloc_no_impl(void*, void*, Layout, Layout);
void* vtable_zalloc_no_impl(void*, Layout);
void* vtable_expand_no_impl(void*, void*, Layout, Layout);
// void vtable_free_no_impl(void*, void*);

// #define NO_IMPL_ALLOCATE (&vtable_alloc_no_impl)
#define NO_IMPL_REALLOCATE (&vtable_realloc_no_impl)
#define NO_IMPL_ZALLOCATE (&vtable_zalloc_no_impl)
// #define NO_IMPL_FREE (&vtable_free_no_impl)

#define VTABLE_ADAPTER_ALLOC_NAME(T) T##_vtable_adapter_alloc

#define VTABLE_ADAPTER_DEF_ALLOC(T, _impl)                       \
  void* VTABLE_ADAPTER_ALLOC_NAME(T)(void* ctx, Layout layout) { \
    __typeof(T)* self = ctx;                                     \
    return (_impl)(self, layout);                                \
  }

#define VTABLE_ADAPTER_REALLOC_NAME(T) T##_vtable_adapter_realloc

#define VTABLE_ADAPTER_DEF_REALLOC(T, _impl)                                                         \
  void* VTABLE_ADAPTER_REALLOC_NAME(T)(void* ctx, void* ptr, Layout old_layout, Layout new_layout) { \
    __typeof(T)* self = ctx;                                                                         \
    return (_impl)(self, ptr, old_layout, new_layout);                                               \
  }

#define VTABLE_ADAPTER_ZALLOC_NAME(T) T##_vtable_adapter_zalloc

#define VTABLE_ADAPTER_DEF_ZALLOC(T, _impl)                       \
  void* VTABLE_ADAPTER_ZALLOC_NAME(T)(void* ctx, Layout layout) { \
    __typeof(T)* self = ctx;                                      \
    return (_impl)(self, layout);                                 \
  }

#define VTABLE_ADAPTER_RESIZE_NAME(T) T##_vtable_adapter_resize

#define VTABLE_ADAPTER_DEF_RESIZE(T, _impl)                                                        \
  bool VTABLE_ADAPTER_RESIZE_NAME(T)(void* ctx, void* ptr, Layout old_layout, Layout new_layout) { \
    __typeof(T)* self = ctx;                                                                       \
    return (_impl)(self, ptr, old_layout, new_layout);                                             \
  }

#define VTABLE_ADAPTER_FREE_NAME(T) T##_vtable_adapter_free

#define VTABLE_ADAPTER_DEF_FREE(T, _impl)                  \
  void VTABLE_ADAPTER_FREE_NAME(T)(void* ctx, void* ptr) { \
    __typeof(T)* self = ctx;                               \
    (_impl)(self, ptr);                                    \
  }

#define VT_DEFINE_AS(_type, _kind, _name) VTABLE_ADAPTER_DEF_##_kind(_type, _name)
#define VT_NAMEOF(_type, _kind) VTABLE_ADAPTER_##_kind##_NAME(_type)

/// C-Style Allocator Interface
/// Inspired by Zig <3
struct Allocator {
  void* ctx;
  const AllocVTable* vtable;
};
typedef struct Allocator Allocator;

static constexpr const Allocator ALLOCATOR_NONE = make_zeroed(Allocator);
// static constexpr const Allocator ALLOCATOR_NOOP = make(Allocator, .ctx = nullptr, .vtable = )

// CONST_FUNC
/// Gets a [Allocator] interface struct for the
// Allocator global_allocator(void);

/// Invokes a given [Allocator] interface struct's inner vtable to call [allocate]!
/// Do note that if given [Allocator] interface struct's do not always implement all function on the [AllocVTable]
/// vtable. as such, if any particular Allocator Vtable call returns ((void*)-1)
[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_allocate(Allocator self, Layout layout) {
  assert(vtmask_has_alloc(self.vtable->mask && self.vtable->allocate));
  return self.vtable->allocate(self.ctx, layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_reallocate(Allocator self, void* ptr, Layout old_layout, Layout new_layout) {
  assert(vtmask_has_realloc(self.vtable->mask && self.vtable->reallocate));
  return self.vtable->reallocate(self.ctx, ptr, old_layout, new_layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_zallocate(Allocator self, Layout layout) {
  assert(vtmask_has_zalloc(self.vtable->mask && self.vtable->zallocate));
  return self.vtable->zallocate(self.ctx, layout);
}

static inline bool allocator_expand(Allocator self, void* ptr, Layout old_layout, Layout new_layout) {
  assert(vtmask_has_expand(self.vtable->mask) && self.vtable->resize);
  return self.vtable->resize(self.ctx, ptr, old_layout, new_layout);
}

static inline void allocator_free(Allocator self, void* ptr) {
  assert(vtmask_has_free(self.vtable->mask && self.vtable->free));
  self.vtable->free(self.ctx, ptr);
}

PURE_FUNC
static inline bool allocator_is_none(Allocator self) { return nullptr == self.vtable; }

/// Returns true if given
PURE_FUNC
static inline bool allocator_is_ok(Allocator self) {
  return !allocator_is_none(self) && (self.vtable->allocate && self.vtable->free);
}

// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into


//
// @basic Fundamental malloc
//
// @details
//
// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into
void* allocate_raw(IterByte* self, Layout layout) METHOD;

//
// @basic Fundamental zeroed malloc
//
// @details
//
// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into
void* zallocate_raw(IterByte* self, Layout layout) METHOD;

// @basic Fundamental in-place expand/shrink
//
// @details
//
// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into//
bool resize_raw(IterByte* self, void* ptr, Layout old, Layout new) PARAMS_NONNULL(1, 2);

// @basic Fundamental memory move /
// @details
//
// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into//
void* reallocate_raw(IterByte* self, void* ptr, Layout old, Layout new) PARAMS_NONNULL(1, 2);





char* strdup_raw(IterByte* self, const char* str);

char* strndup_raw(IterByte* self, const char* str, i32 len);

sslice sslice_dup_raw(IterByte* self, const char* str, i32 len);

HEDLEY_PRINTF_FORMAT(2, 3)
sslice fslice_raw(IterByte* self, const char* fmt, ...);

sslice vfslice_raw(IterByte* self, const char* fmt, va_list args);

HEDLEY_PRINTF_FORMAT(3, 4)
char* fstring_raw(IterByte* self, i64* len_out, const char* fmt, ...);

char* vfstring_raw(IterByte* self, i64* len_out, const char* fmt, va_list args);





METHOD
/// @brief Zeroes memory at pointer with size layout
/// @details does not free any memory and pointers and memory are still valid for reads and writes after this function
/// returns.
/// @param (u64 pattern) :: Pattern used to set freed memory to. If you dont know or care about this, you can safely
/// just pass 0
void free_raw(IterByte* self, void* ptr, Layout layout, u64 pattern);

