#pragma once

#include "attributes.h"
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
/// isize size  - Size of allocation requested in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
typedef void* (*const VTableAllocate)(void* self, isize size, isize align);
/// Function pointer typedef for [Allocator] [AllocVTable] reallocate method
///
/// void* self     - Pointer to self (may be null if Allocator has no state!)
///                  Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr      - Pointer to begging of block of memory to be reallocated
/// isize new_size - Size of requested reallocation in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2! 
typedef void* (*const VTableReallocate)(void* self, void* ptr, isize new_size, isize align);

/// Function pointer typedef for [Allocator] [AllocVTable] zallocate method
/// This is the same as [VTableAllocate], but ensures allocated memory is zeroed
///
/// void* self  - Pointer to self (may be null if Allocator has no state!)
///               Typically you will cast this to your derived Allocator type in this method implementation
/// usize size  - Size of allocation requested in bytes
/// isize align - Alignment of allocation requested. Must be a power of 2!
typedef void* (*const VTableZallocate)(void* self, isize size, isize align);

/// Function pointer typedef for [Allocator] [AllocVTable] expand method
/// This is the same as [VTableReallocate], but does nothing  if
/// memory cannot be expanded in place to new_size
///
/// void* self     - Pointer to self (may be null if Allocator has no state!)
///                  Typically you will cast this to your derived Allocator type in this method implementation
/// void* ptr      - Pointer to begging of block of memory to be expanded in place
/// usize new_size - Size of requested reallocation in bytes
typedef void* (*const VTableExpand)(void* self, void* ptr, isize new_size);

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
  /// See [VTableExpand]
  VTableExpand expand_in_place;
  /// See [VTableFree]
  VTableFree free;
};
typedef struct AllocVTable AllocVTable;

CONST_FUNC
const AllocVTable* global_allocator_vtable(void);

#define alloc_vtable_new(...) ((AllocVTable){__VA_ARGS__})

void* vtable_alloc_no_impl(void*, isize, isize);
void* vtable_realloc_no_impl(void*, void*, isize, isize);
void* vtable_zalloc_no_impl(void*, isize, isize);
void* vtable_expand_no_impl(void*, void*, isize);
void vtable_free_no_impl(void*, void*);

#define NO_IMPL_ALLOCATE (&vtable_alloc_no_impl)
#define NO_IMPL_REALLOCATE (&vtable_realloc_no_impl)
#define NO_IMPL_ZALLOCATE (&vtable_zalloc_no_impl)
#define NO_IMPL_EXPAND (&vtable_expand_no_impl)
#define NO_IMPL_FREE (&vtable_free_no_impl)

/// C-Style Allocator Interface
/// Inspired by Zig <3
struct Allocator {
  void* ctx;
  const AllocVTable* vtable;
};
typedef struct Allocator Allocator;


CONST_FUNC
/// Gets a [Allocator] interface struct for the
/// global allocator [mi_malloc]/[mi_free] and friends
Allocator global_allocator(void);

/// Invokes a given [Allocator] interface struct's inner vtable to call [allocate]!
/// Do note that if given [Allocator] interface struct's do not always implement all function on the [AllocVTable]
/// vtable. as such, if any particular Allocator Vtable call returns ((void*)-1)
[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_allocate(Allocator self, isize size, isize align) {
  return self.vtable->allocate(self.ctx, size, align);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_reallocate(Allocator self, void* ptr, isize new_size, isize align) {
  return self.vtable->reallocate(self.ctx, ptr, new_size, align);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_zallocate(Allocator self, isize size, isize align) {
  return self.vtable->zallocate(self.ctx, size, align);
}

[[nodiscard("Must not discard pointer returned from allocator! possible memory leak!")]]
static inline void* allocator_expand(Allocator self, void* ptr, isize new_size) {
  return self.vtable->expand_in_place(self.ctx, ptr, new_size);
}

static inline void allocator_free(Allocator self, void* ptr) { self.vtable->free(self.ctx, ptr); }

/// Wrapper struct around a pointer to a [mi_heap_t]
struct HeapAllocator {
  struct mi_heap_s* heap;
};
typedef struct HeapAllocator HeapAllocator;

/// Creates a new [HeapAllocator] by calling [mi_heap_new]
HeapAllocator heap_allocator_new(void);

/// forwards call  to [mi_heap_malloc]
void* heap_allocator_malloc(HeapAllocator self, isize size, isize align);
/// forwards call to [mi_heap_realloc]
void* heap_allocator_realloc(HeapAllocator self, void* ptr, isize new_size, isize align);
/// forwards call to [mi_heap_zalloc]
void* heap_allocator_zalloc(HeapAllocator self, isize size, isize align);
/// forwards call to [mi_expand]
void* heap_allocator_expand(HeapAllocator self, void* ptr, isize new_size);
/// forwwards call to [mi_free]
void heap_allocator_free(HeapAllocator self, void* ptr);

/// Returns a const pointer to [HeapAllocator]'s vtable ([AllocVTable])
PURE_FUNC
const AllocVTable* heap_allocator_vtable(void);

/// Converts a [HeapAllocator] into the [Allocator] struct interface
METHOD
static inline Allocator heap_allocator(HeapAllocator* self) {
  return (Allocator){.ctx = ((void*)self), .vtable = heap_allocator_vtable()};
}

/// forwards call to [mi_heap_delete]
METHOD
void heap_allocator_delete(HeapAllocator* self);
/// forwards call to [mi_heap_destroy]
/// Caution as this can cause program crashes if you are not careful
METHOD
void heap_allocator_destroy(HeapAllocator* self);

/// A simple Arena Allocator
///
/// If this Arena is not really meant to resize the buffer
/// it owns, as that would cause a nightmare where pointers allocated up to that point
/// are going to be invalidated after arena resize.
///
/// TODO: Implement a roped arena, where each arena has a prev field
/// of its own type (a linked list of Arenas, where the top-most arena is the root of the
/// chain, going back, each arena block has a pointer to the previous (full) Arena.) This way
/// we append new blocks of memory when we run out, we allocate a new block, set the current root to the next
/// arena block previous field, then set the new arena block as the new root. boom. We are able
/// to grow in size and pointers dont get invalidated. This is at the cost of fragmenting memory a little bit,
/// but we can get even crazier with it, and allow for the roped/chained Arena to take a parent allocator to
/// allocate out of. That way theoretically, you could have a master allocator that allocates out of a huge block
/// of static memory, and then have several roped/chained arena allocate our of that every time they need more space,
/// and there you have a growable, non-pointer-invalidating, Arena that is contiguous in memory. But all thats for
/// another day i have some other things to write
struct Arena {
  u8* mem;
  isize capacity;
  isize used;
};
typedef struct Arena Arena;

/// Creates a new [Arena] struct.
/// If this function fails to allocate with the global allocator,
/// or runs into an unexpected error during its execution at runtime,
/// then this function will return a zeroed/null [Arena] instance.
///
/// You can use [arena_is_ok] function to check that the returned [Arena] instance
/// is valid and ready to be used.
[[nodiscard("Must check returned Arena is not zeroed, in which case it must be freed before going out of scope")]]
Arena arena_new(isize capacity);

/// Creates a new [Arena], using given @param (alloc) to allocate
/// the initial memory for it. Use [arena_destroy_in] after done with this arena, NOT [arena_destroy], which uses the
/// global allcoator, which may be different from the allocator used to create the Allocator

[[nodiscard("Must check returned Arena is not zeroed, in which case it must be freed before going out of scope")]]
Arena arena_new_in(isize capacity, Allocator alloc);

/// Checks that a newly created/initialized Arena non-null/non-zeroed
/// and has a valid pointer to memory and a valid capacity
///
/// This is necessary as ZII (Zero Is Initialization), or rather, zero as error.
/// so if [arena_new] cannot allocate for some reason, or runs into an unexpected error,
/// it will return a zeroed [Arena] struct instead of one that has valid fields and is
/// ready to be used
///
PURE_FUNC
static inline bool arena_is_ok(const Arena* self) { return self && self->mem && self->capacity > 0; }

METHOD
void* arena_allocate(Arena* self, isize size, isize align);

/// Same as [arena_allocate], but ensure memory is zeroed.
/// [Arena] initially use [mi_calloc] to allocate the memory buffer, so
/// memory is zeroed already initially, but if [arena_clear] was called instead of [arena_clear_zeroed],
/// then there is a possiblity that memory might not be zeroed
METHOD
void* arena_zallocate(Arena* self, isize size, isize align);

CONST_FUNC
const AllocVTable* arena_alloc_vtable(void);

PURE_FUNC
METHOD
static inline Allocator arena_allocator(Arena* self) {
  return (Allocator){.ctx = (void*)self, .vtable = arena_alloc_vtable()};
}

/// Cleans up memory used by this [Arena]
METHOD
void arena_destroy(Arena* self);

/// cleans up memory used by [Arena]. Must use the same [Allocator] that was used to create this [Arena]!
METHOD
void arena_destroy_in(Arena* self, Allocator alloc);

METHOD
void arena_clear(Arena* self);

METHOD
void arena_clear_zeroed(Arena* self);


typedef void* mi_arena_id_t;
typedef mi_arena_id_t VirtMem;


VirtMem vmem_new(i32 size_mb);

struct mi_heap_s* vmem_heap_new(VirtMem self);

isize vmem_size(VirtMem self);



