#pragma once

#include <assert.h>
#include <string.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"
#include "nv/core/core_types.h"
#include "nv/memory/layout.h"

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

#define VTABLE_ADAPTER_DEF_ALLOC(T, _impl)                              \
  void* VTABLE_ADAPTER_ALLOC_NAME(T)(void* ctx, Layout layout) { \
    __typeof(T)* self = ctx;                                        \
    return (_impl)(self, layout);                                   \
  }

#define VTABLE_ADAPTER_REALLOC_NAME(T) T##_vtable_adapter_realloc

#define VTABLE_ADAPTER_DEF_REALLOC(T, _impl)                                                                   \
  void* VTABLE_ADAPTER_REALLOC_NAME(T)(void* ctx, void* ptr, Layout old_layout, Layout new_layout) { \
    __typeof(T)* self = ctx;                                                                               \
    return (_impl)(self, ptr, old_layout, new_layout);                                                     \
  }

#define VTABLE_ADAPTER_ZALLOC_NAME(T) T##_vtable_adapter_zalloc

#define VTABLE_ADAPTER_DEF_ZALLOC(T, _impl)                              \
  void* VTABLE_ADAPTER_ZALLOC_NAME(T)(void* ctx, Layout layout) { \
    __typeof(T)* self = ctx;                                         \
    return (_impl)(self, layout);                                    \
  }

#define VTABLE_ADAPTER_RESIZE_NAME(T) T##_vtable_adapter_resize

#define VTABLE_ADAPTER_DEF_RESIZE(T, _impl)                                                                   \
  bool VTABLE_ADAPTER_RESIZE_NAME(T)(void* ctx, void* ptr, Layout old_layout, Layout new_layout) { \
    __typeof(T)* self = ctx;                                                                              \
    return (_impl)(self, ptr, old_layout, new_layout);                                                    \
  }

#define VTABLE_ADAPTER_FREE_NAME(T) T##_vtable_adapter_free

#define VTABLE_ADAPTER_DEF_FREE(T, _impl)                       \
  void VTABLE_ADAPTER_FREE_NAME(T)(void* ctx, void* ptr) { \
    __typeof(T)* self = ctx;                                \
    (_impl)(self, ptr);                                     \
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

// // TODO: Decide if i want to do all this ish...
// // for now it seems a little to over-abstracted for me..
// // /// @brief a non-owning span of bytes
// // struct Span {};
// // alias(Span);

// // /// @details '(Chunk::begin + ChCursor::i) == ChCursor::iter' should be true at all times
// // struct ChCursor {
// //   i64 i;
// //   byte* iter;
// // };
// // alias(ChCursor);

// // /// @brief a non-owning span of bytes in memory
// // struct Chunk {
// //   /// @brief pointer to the first byte of this chunk
// //   byte* begin;
// //   /// @brief should always be pointing to the byte immediately after
// //   /// the last byte in this chunk
// //   byte* end;

// //   ChCursor cursor;
// // };
// // alias(Chunk);

// // Chunk chunk_new(byte* begin, i64 size) PARAMS_NONNULL(1);

// // void chunk_write(Chunk* self, const Chunk* other) PARAMS_NONNULL(1, 2);

// // void chunk_write_bytes(Chunk* self, byte* begin, i64 size) PARAMS_NONNULL(1, 2);

// // /// @brief Chunk [P]ush [Alloc]ate
// // void* chunk_palloc(Chunk* self, MemLayout layout) METHOD;

// // void* chunk_pzalloc(Chunk* self, MemLayout layout) METHOD;

// // Chunk chunk_clone(const Chunk* self, Allocator alloc) METHOD;

// // void chunk_clone_bytes(const Chunk* self, char* dest, i64 dest_len) PARAMS_NONNULL(1,2);

// struct VirtMem;

// /// @brief a header-less version of [VArena]
// struct Arena {
//   /// @brief null if not created with [arena_new_in]
//   struct VirtMem* parent;
//   /// @brief for in-place expansion
//   void* last_alloc;
//   /// brief -1 if not created with [arena_new_in]
//   i64 marker;

//   i64 size;
//   byte* begin;
//   byte* end;
//   byte* cursor;
// };
// alias(Arena);

// /// @brief how was this arena created?
// typedef enum ArenaType {
//   /// @brief created with [arena_new]
//   Arena__Buffered = 0,
//   /// @brief created with [arena_new_in]
//   Arena__OwnedVirt,
// } ArenaType;

// PURE_FUNC
// static inline ArenaType arena_type(const Arena* self) {
//   return (self && self->parent && (self->marker >= 0)) ? Arena__OwnedVirt : Arena__Buffered;
// }

// static inline i64 arena_used_bytes(const Arena* self) {
//   assert(self);
//   return self->cursor - self->begin;
// }

// static inline i64 arena_available(const Arena* self) {
//   assert(self);
//   return self->size - arena_used_bytes(self);
// }

// static inline bool arena_is_owned(const Arena* self) { return arena_type(self) == Arena__OwnedVirt; }
// static inline bool arena_is_buffered(const Arena* self) { return arena_type(self) == Arena__Buffered; }

// static constexpr const Arena ARENA_EMPTY = {};

// static inline bool arena_is_empty(const Arena* self) { return memcmp(self, &ARENA_EMPTY, sizeof(Arena)) == 0; }

// Arena arena_new(byte* begin, i64 size_bytes) PARAMS_NONNULL(1);
// Arena arena_new_in(struct VirtMem* vm, i64 size_bytes);

// void* arena_allocate(Arena* self, Layout);
// void* arena_zallocate(Arena* self, Layout layout) METHOD;
// void* arena_reallocate(Arena* self, void* ptr, Layout old, Layout nlayout) METHOD;
// bool arena_expand(Arena* self, void* ptr, Layout old, Layout nlayout) METHOD;

// Allocator arena_allocator(Arena* self) METHOD;

// void arena_destroy(Arena* self);
