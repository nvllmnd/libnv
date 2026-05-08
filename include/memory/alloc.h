#pragma once

#include "attributes.h"
#include "core_types.h"
#include "intdefs.h"

typedef enum AllocationResult : isize {
  /// The Allocator VTable Method is not implemented in the implementing/super Allocator!
  ///
  /// For implementing custom Allocators:
  /// If your allocator does not wish to implement or support one of the
  /// [AllocVTable] fields/methods, for the ones that return a void*,
  /// then you MUST return This value instead of nullptr! otherwise callers
  /// will assume that your allocator DOES support that method, its just that your allocator has ran into an error or
  /// is out of memory!
  ///
  AllocatorVTableMethodNotImplemented = -1,
  /// alias for nullptr/NULL
  ///
  /// For Implementing allocator interface,
  /// for whose implementation functions run into an error, either due
  /// to invalid parameters, inner error, or the allocator is simply out of available memory,
  /// and cannot grab more/resize, then return nulltr.
  ///
  /// For VTable methods you choose NOT to implement that need to return a nullptr,
  /// See/Return [AllocatorVTableMethodNotImplemented] NOT NULL
  ///
  AllocatorFailedAllocation = 0,
  /// values over this one are also valid and are considered [AllocatorOk]
  /// Any non-null, non-negative value returned from the casted pointer symbolizes a successfull
  /// allocation (> [AllocatorOk])
  AllocationOk,
} AllocationResult;

/// Memory Layout, used for determining size and alignment of Allocator allocations
struct MemLayout {
  /// Size of requested allocation in bytes. must be a multiple of alignment
  i32 size;
  /// Alignment of requested allocation. must be a multiple of 2
  i32 align;
};
typedef struct MemLayout MemLayout;

#define mlayout_static(s, a)                          \
  ({                                                  \
    constexpr const __typeof(s) _s = (s);             \
    constexpr const __typeof(a) _a = (a);             \
    static_assert(IS_POWER_OF_2(_a) && _s % _a == 0); \
    make(MemLayout, .size = _s, .align = _a);       \
  })

#define mlayout_new(T) (mlayout_static(sizeof(T), alignof(T)))
#define mlayout_array(T, N) (mlayout_static(sizeof(T) * N, alignof(T[N])))

#define mlayout_fma(THeader, flex_member_size) (make(MemLayout, .size = sizeof(THeader) + (flex_member_size), .align = alignof(Block)))

CONST_FUNC
static inline MemLayout mlayout_bytes(isize nbytes) {
  return make(MemLayout, .size = nbytes, .align = alignof(u8[nbytes]));
}
// #define mlayout_bytes()

#define NO_IMPL_METHOD_RESULT ((void*)AllocatorVTableMethodNotImplemented)

/// Returns the Error state of the pointer returned by an [Allocator] interface struct
static inline AllocationResult alloc_result(void* ptr) { return (AllocationResult)ptr; }

/// Checks if pointer returned by an [Allocator] interface struct
/// is from a method that the [Allocator] does not implement/support
static inline bool alloc_is_not_impl(void* ptr) { return alloc_result(ptr) == AllocatorVTableMethodNotImplemented; }

/// Checks if a pointer returned by an [Allocator] interface struct
/// is nullptr, therefore symbolizing the [Allocator] raising an Allocation Error.
/// This means that the [Allocator] method failed to allocate any memory due
/// to either an inner system error or because the [Allocator] is simply out of
/// available space/memory to accomadate the size of the requested allocation!
static inline bool alloc_is_failed_allocation(void* ptr) { return alloc_result(ptr) == AllocatorFailedAllocation; }

/// Checks that a pointer returned by an [Allocator] interface struct
/// is valid and points to valid, read/writeable memory
static inline bool alloc_is_ok(void* ptr) { return alloc_result(ptr) >= AllocationOk; }

/// Function pointer typedef for [Allocator] [AllocVTable] allocate method.
///
/// void* self  - Pointer to self (may be null if Allocator has no state!)
///              Typically you will cast this to your derived Allocator type in this method implementation
/// MemLayout layout - layout of requested allocation. see [MemLayout]
///
typedef void* (*const VTableAllocate)(void* self, MemLayout layout);
/// Function pointer typedef for [Allocator] [AllocVTable] reallocate method
///
/// void* self     - Pointer to self (may be null if Allocator has no state!)
///                  Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr      - Pointer to begging of block of memory to be reallocated
/// isize new_size - Size of requested reallocation in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
typedef void* (*const VTableReallocate)(void* self, void* ptr, MemLayout old_layout, MemLayout new_layout);

/// Function pointer typedef for [Allocator] [AllocVTable] zallocate method
/// This is the same as [VTableAllocate], but ensures allocated memory is zeroed
///
/// void* self  - Pointer to self (may be null if Allocator has no state!)
///               Typically you will cast this to your derived Allocator type in this method implementation
/// usize size  - Size of allocation requested in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
typedef void* (*const VTableZallocate)(void* self, MemLayout layout);

/// Function pointer typedef for [Allocator] [AllocVTable] free method.
///
/// void* self - Pointer to self (may be null if Allocator has no state!)
///              Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr  - Pointer to block of memory to be freed by this allocator
typedef void (*const VTableFree)(void* self, void* ptr);

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
};
typedef struct AllocVTable AllocVTable;

// CONST_FUNC
// const AllocVTable* global_allocator_vtable(void);

#define alloc_vtable_new(...) ((AllocVTable){__VA_ARGS__})

void* vtable_alloc_no_impl(void*, MemLayout);
void* vtable_realloc_no_impl(void*, void*, MemLayout, MemLayout);
void* vtable_zalloc_no_impl(void*, MemLayout);
void* vtable_expand_no_impl(void*, void*, MemLayout, MemLayout);
void vtable_free_no_impl(void*, void*);

#define NO_IMPL_ALLOCATE (&vtable_alloc_no_impl)
#define NO_IMPL_REALLOCATE (&vtable_realloc_no_impl)
#define NO_IMPL_ZALLOCATE (&vtable_zalloc_no_impl)
#define NO_IMPL_FREE (&vtable_free_no_impl)

/// C-Style Allocator Interface
/// Inspired by Zig <3
struct Allocator {
  void* ctx;
  const AllocVTable* vtable;
};
typedef struct Allocator Allocator;

// CONST_FUNC
/// Gets a [Allocator] interface struct for the
// Allocator global_allocator(void);

/// Invokes a given [Allocator] interface struct's inner vtable to call [allocate]!
/// Do note that if given [Allocator] interface struct's do not always implement all function on the [AllocVTable]
/// vtable. as such, if any particular Allocator Vtable call returns ((void*)-1)
[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_allocate(Allocator self, MemLayout layout) {
  return self.vtable->allocate(self.ctx, layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_reallocate(Allocator self, void* ptr, MemLayout old_layout, MemLayout new_layout) {
  return self.vtable->reallocate(self.ctx, ptr, old_layout, new_layout);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_zallocate(Allocator self, MemLayout layout) {
  return self.vtable->zallocate(self.ctx, layout);
}

static inline void allocator_free(Allocator self, void* ptr) { self.vtable->free(self.ctx, ptr); }

/// A simple Arena Allocator
///
/// If this Arena is not really meant to resize the buffer
/// it owns, as that would cause a nightmare where pointers allocated up to that point
/// are going to be invalidated after arena resize.
///
struct FixedBuffAlloc {
  u8* mem;
  isize capacity;
  isize used;
};
typedef struct FixedBuffAlloc FixedBuffAlloc;


/// Creates a new [Arena] struct.
/// If this function fails to allocate with the global allocator,
/// or runs into an unexpected error during its execution at runtime,
/// then this function will return a zeroed/null [Arena] instance.
///
/// You can use [arena_is_ok] function to check that the returned [Arena] instance
/// is valid and ready to be used.
[[nodiscard("Must check returned Arena is not zeroed, in which case it must be freed before going out of scope")]]
FixedBuffAlloc fba_new(isize capacity);

/// Creates a new [Arena], using given @param (alloc) to allocate
/// the initial memory for it. Use [arena_destroy_in] after done with this arena, NOT [arena_destroy], which uses the
/// global allcoator, which may be different from the allocator used to create the Allocator

[[nodiscard("Must check returned Arena is not zeroed, in which case it must be freed before going out of scope")]]
FixedBuffAlloc fba_new_in(isize capacity, Allocator alloc);

/// Checks that a newly created/initialized Arena non-null/non-zeroed
/// and has a valid pointer to memory and a valid capacity
///
/// This is necessary as ZII (Zero Is Initialization), or rather, zero as error.
/// so if [arena_new] cannot allocate for some reason, or runs into an unexpected error,
/// it will return a zeroed [Arena] struct instead of one that has valid fields and is
/// ready to be used
///
PURE_FUNC
static inline bool fba_is_ok(const FixedBuffAlloc* self) { return self && self->mem && self->capacity > 0; }

METHOD
void* fba_allocate(FixedBuffAlloc* self, MemLayout layout);

/// Same as [arena_allocate], but ensure memory is zeroed.
/// [Arena] initially use ?? allocate the memory buffer, so
/// memory is zeroed already initially, but if [arena_clear] was called instead of [arena_clear_zeroed],
/// then there is a possiblity that memory might not be zeroed
METHOD
void* fba_zallocate(FixedBuffAlloc* self, MemLayout layout);




/// Cleans up memory used by this [Arena]
METHOD
void fba_destroy(FixedBuffAlloc* self);

/// cleans up memory used by [Arena]. Must use the same [Allocator] that was used to create this [Arena]!
METHOD
void fba_destroy_in(FixedBuffAlloc* self, Allocator alloc);

METHOD
void fba_clear(FixedBuffAlloc* self);

METHOD
void fba_clear_zeroed(FixedBuffAlloc* self);
