// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// #ifdef __cplusplus

// #include <concepts>
// #include <type_traits>

// struct Layout {};

// namespace nv {}  // namespace nv

// #else

#include <assert.h>

#include "nv/core/algo.h"
#include "nv/common.h"
#include "nv/core/attributes.h"
#include "nv/iter/iterators.h"
#include "nv/memory/alloc.h"

#ifndef __cplusplus
#ifndef MAKE
#define MAKE make
#endif
#ifndef MAKE_ZEROED
#define MAKE_ZEROED make_zeroed
#endif
#endif

BEGIN_C_DECLS
// #include "nv/memory/vmem.h"

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
    (Layout){.size = _s, .align = _a};                \
  })

#define mlayout_new(T) (mlayout_static(sizeof(T), alignof(T)))
#define mlayout_array(T, N) (mlayout_static(sizeof(T) * N, alignof(T)))
#define mlayout_vec(T, _n) ((Layout){.size = sizeof(T) * (_n), .align = alignof(T)})
#define mlayout_fma(THeader, flex_member_size) \
  ((Layout){.size = sizeof(THeader) + (flex_member_size), .align = alignof(THeader)})

/// @brief Creates a new [Layout] appropriate for allocating a buffer of bytes of size `nbytes`
/// @param(i32 nbytes) size in bytes of allocation request. Must be > 0
CONST_FUNC
static inline Layout mlayout_bytes(i32 nbytes) {
  assert(nbytes > 0);
  return (Layout){.size = nbytes, .align = 1};
}

/// @brief exteneds Layout by count. (if MemLayout represents a single element of a typed array, then MemLayout * count
/// is the MemLayout of that typed array)
CONST_FUNC
static inline Layout mlayout_extend(Layout self, i32 count) {
  assert(count > 0);
  return (Layout){.size = self.size * count, .align = self.align};
}

#ifndef __cplusplus
#define MAX max
#endif

/// @brief Creates a new MemLayout calculated as such: multiplies self.size * count and adds the rhs.size to the result,
/// takes max of self and rhs alignment
CONST_FUNC
static inline Layout mlayout_extend_with(Layout self, i32 count, Layout rhs) {
  assert(count > 0);
  return (Layout){.size = (self.size * count) + rhs.size, .align = MAX(self.align, rhs.align)};
}

/// @brief lhs.size + rhs.size, align = max(lhs.align, rhs.align)
CONST_FUNC
static inline Layout mlayout_add(Layout lhs, Layout rhs) {
  return (Layout){.size = lhs.size + rhs.size, .align = MAX(lhs.align, rhs.align)};
}

PARAMS_NONNULL(1)
isize ptr_align_offset(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

/// Checks if a given pointer is aligned to given alignment.
/// @param (align) MUST BE A POWER OF 2. If it is not this funciton returns
/// false
PURE_FUNC
PARAMS_NONNULL(1)
bool ptr_is_aligned(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

/// Aligns pointer up to given alignment, or returns the same pointer if it
/// already is aligned
/// @param (align) MUST BE A POWER OF 2.  If it is not, then this function
/// returns the exact same pointer, doing no calulations and possiby causing
/// confusion if given pointer is misaligned
PARAMS_NONNULL(1)
RETURNS_NON_NULL
void* ptr_alignup(void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

PARAMS_NONNULL(1, 2)
PURE_FUNC
u8* ptr_alignto(u8* ptr, u8* end, Layout layout) WHERE(IS_POWER_OF_2(layout.align) && end >= ptr);

/// behaves similarly to C++'s std::align
PARAMS_NONNULL(1, 2)
u8* ptr_alignin(u8* ptr, i32* space, Layout layout);

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

#ifndef __cplusplus
#define BITHAS bithas
#define BITHASALL bithasall
#endif

CONST_FUNC
static inline bool vtmask_has_alloc(AllocVTableMask mask) { return BITHAS(mask, VT__Allocate); }

CONST_FUNC
static inline bool vtmask_has_realloc(AllocVTableMask mask) { return BITHAS(mask, VT__Reallocate); }

CONST_FUNC
static inline bool vtmask_has_zalloc(AllocVTableMask mask) { return BITHAS(mask, VT__Zallocate); }

CONST_FUNC
static inline bool vtmask_has_expand(AllocVTableMask mask) { return BITHAS(mask, VT__Expand); }

CONST_FUNC
static inline bool vtmask_has_free(AllocVTableMask mask) { return BITHAS(mask, VT__Free); }

/// Tests that a mask has the the needed allocation methods implemented (only allocate and free are mandatory)
CONST_FUNC
static inline bool vtmask_is_ok(AllocVTableMask mask) { return BITHASALL(mask, VT__Allocate | VT__Free); }

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

static constexpr const Allocator ALLOCATOR_NONE = MAKE_ZEROED(Allocator);
// static constexpr const Allocator ALLOCATOR_NOOP = make(Allocator, .ctx = nullptr, .vtable = )

// CONST_FUNC
/// Gets a [Allocator] interface struct for the
// Allocator global_allocator(void);

/// Invokes a given [Allocator] interface struct's inner vtable to call [allocate]!
/// Do note that if given [Allocator] interface struct's do not always implement all function on the [AllocVTable]
/// vtable. as such, if any particular Allocator Vtable call returns ((void*)-1)
[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_allocate(Allocator self, Layout layout) {
  assert(vtmask_has_alloc(self.vtable->mask) && self.vtable->allocate);
  return self.vtable->allocate(self.ctx, layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_reallocate(Allocator self, void* ptr, Layout old_layout, Layout new_layout) {
  assert(vtmask_has_realloc(self.vtable->mask) && self.vtable->reallocate);
  return self.vtable->reallocate(self.ctx, ptr, old_layout, new_layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_zallocate(Allocator self, Layout layout) {
  assert(vtmask_has_zalloc(self.vtable->mask) && self.vtable->zallocate);
  return self.vtable->zallocate(self.ctx, layout);
}

static inline bool allocator_expand(Allocator self, void* ptr, Layout old_layout, Layout new_layout) {
  assert(vtmask_has_expand(self.vtable->mask) && self.vtable->resize);
  return self.vtable->resize(self.ctx, ptr, old_layout, new_layout);
}

static inline void allocator_free(Allocator self, void* ptr) {
  assert(vtmask_has_free(self.vtable->mask) && self.vtable->free);
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
// iterbyte pointing to any memory you want to allocate into//
bool resize_raw(IterByte* self, void* ptr, Layout old, Layout new_layout) PARAMS_NONNULL(1, 2);

// @basic Fundamental memory move /
// @details
//
// NOTE: These raw allocation functions assume the range pointed to by IterByte parameter is contiguous,
// and as such, has the behavior of a bump-style Arena Allocator. I figure the Arena is the most fundamental (simple)
// Allocator, so it makes sense to use this allocation style for the most basic allocations You can create your own
// IterByte pointing to any memory you want to allocate into//
void* reallocate_raw(IterByte* self, void* ptr, Layout old, Layout new_layout) PARAMS_NONNULL(1, 2);

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

/// @brief a non-owning Arena-style allocator
/// @details Callers are responsible for managing buffer used by this Arena
typedef struct Arena {
  IterData(byte);
} Arena;

typedef Arena ScopedArena;

static constexpr const Arena ARENA_NONE = {};

PARAMS_NONNULL(1)
Arena arena_new(byte* begin, isize size);

static inline void arena_init(Arena* self, byte* begin, isize size) {
#ifdef __cplusplus
  using namespace nv;
#endif
  if LIKELY (is_not_null(self) && is_not_null(begin) && size > 0) {
    *self = arena_new(begin, size);
  }
}

PARAMS_NONNULL(1, 2)
static inline Arena arena_range_new(byte* begin, byte* end) {
  assert(begin);
  assert(end);
  return (Arena){
      .begin = begin,
      .end = end,
      .cursor = begin,
  };
}

Allocator arena_allocator(Arena* self);

METHOD
PURE_FUNC
static inline isize arena_avail(const Arena* self) { return self->end - self->cursor; }

METHOD
PURE_FUNC
static inline isize arena_used_bytes(const Arena* self) { return self->cursor - self->begin; }

METHOD
PURE_FUNC
static inline isize arena_size(const Arena* self) { return self->end - self->begin; }

METHOD
PURE_FUNC
static inline CIterByte arena_citer(const Arena* self) {
  return (CIterByte){
      .begin = self->begin,
      .end = self->end,
      .cursor = self->cursor,
  };
}

METHOD
static inline IterByte arena_iter(Arena* self) {
  return (IterByte){
      .begin = self->begin,
      .end = self->end,
      .cursor = self->cursor,
  };
}

struct VMem;
struct Vallocator;

/// @brief Creates an Arena that will allocate into given [VMem] at given byte offset of given size bytes
Arena arena_vmem_at_new(struct VMem* vm, isize size, isize offset) PARAMS_NONNULL(1);

/// @brief same as [arena_vmem_at_new], but uses all memory in given VMem
Arena arena_vmem_new(struct VMem* vm) PARAMS_NONNULL(1);

/// @brief uses [Vallocator] to request a block of given size and creates new Arena to allocate into it
Arena arena_va_new(struct Vallocator* va, isize size) PARAMS_NONNULL(1);

/// @brief Same as [arena_va_new], but polymorphic over [Allocator]
Arena arena_new_in(Allocator alloc, isize size);

void arena_clear(Arena* self) METHOD;

void arena_clear_zeroed(Arena* self) METHOD;

/// @brief creates a copy of this Arena, but with its begin pointer set to the current value of this Arena's cursor.
/// @details You should not use the original Arena while this scoped arena is actively being used, doing so will cause
/// parent arena to overwrite memory used by copy created by this function
/// After the retured 'ScopedArena' has gone out of scope, you can continue using the original arena, and we will use
/// the same space the scoped child arena used
/// @remarks this is a clean way of doing a watermark system, where offsets are saved. I like this way better tbh,
/// little more room for error but its a lot cleaner, and child allocator cannot touch memory of its parent
METHOD
static inline ScopedArena arena_scoped(const Arena* self) {
  byte* const cursor = self->cursor;
  return (ScopedArena){
      .begin = cursor,
      .end = self->end,
      .cursor = cursor,
  };
}

/// @brief resets this Arena back to state before most recetn allocation.
/// @details pointer and layout parameters are for verifying that this poitner and layout was indeed the last allocation
/// made by this Arena If given pointer and layout where are not the same as the most recent allocation made by this
/// Arena, nothing about this arena is changed and false is returned
/// @returns true on success, false otherwise.
/// @remarks you can use this to undo a recent allocation. For example, you use this arena to allocate some memory, but
/// after allocation, caller encounters some error and has to abort the allocation. Without this function such scenarios
/// would cause uneccessary memory leaks. Leaks are not as detrimental to runtimes since we are using an Arena, but
/// still it certainly does not help to avoid it if possible!
bool arena_alloc_undo(Arena* self, void* ptr, Layout layout) PARAMS_NONNULL(1, 2);

/// @brief same as @see [arena_alloc_undo] but zeroes the freed memory upon success
bool arena_alloc_undo_zeroed(Arena* self, void* ptr, Layout layout) PARAMS_NONNULL(1, 2);

PURE_FUNC
METHOD
static inline bool arena_contains(const Arena* self, void* ptr) {
  assert(self);

  byte* p = CAST(byte*, ptr);
  return p >= self->begin && p < self->end;
}

void* arena_alloc(Arena* self, Layout layout) METHOD;

void* arena_zalloc(Arena* self, Layout layout) METHOD;

bool arena_resize(Arena* self, void* ptr, Layout old, Layout new_layout) METHOD;

void* arena_realloc(Arena* self, void* ptr, Layout old, Layout new_layout) METHOD;

char* arena_fstring(Arena* self, isize* slen_out, const char* fmt, ...) METHOD HEDLEY_PRINTF_FORMAT(3, 4);

char* arena_vfstring(Arena* self, isize* slen_out, const char* fmt, va_list args) PARAMS_NONNULL(1, 3);

char* arena_strndup(Arena* self, const char* str, isize len) PARAMS_NONNULL(1, 2);

sslice arena_strdup(Arena* self, sslice str) METHOD;

/// @brief reads file at given path into this Arena as a readonly null-terminated string
const char* arena_fread_string(Arena* self, const char* path, isize* file_size_out) PARAMS_NONNULL(1, 2);

PARAMS_NONNULL(1, 2)
static inline sslice arena_fread_slice(Arena* self, const char* path) {
  isize len = 0;
  const char* str = arena_fread_string(self, path, &len);
  return sslice_new(.begin = str, .len = CAST(i32, len));
}

/// @brief performs a deep copy of all bytes in the iterator range of this Arena.
/// @details this operation is O(n), where n is the difference in bytes between this Arena's end and begin iterator
/// pointers
void arena_clone(const Arena* src, Arena dest) METHOD;

#define arena_make(_self, T) ((__typeof(T)*)arena_allocate((_self), mlayout_new(T)))
#define arena_array_alloc(_self, T, N) ((__typeof(T)*)arena_allocate((_self), mlayout_array(T, N)))
#define arena_array_allocn(_self, T, _n) ((__typeof(T)*)arena_allocate((_self), mlayout_vec(T, (_n))))

END_C_DECLS

// #endif
