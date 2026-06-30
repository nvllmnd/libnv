#pragma once

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/memory/vmem.h"

HEDLEY_BEGIN_C_DECLS

/// @brief first-fit, size-segragated block allocator
/// @details allocations larger than value returned by [heap_max_alloc_size] are NOT supported
typedef struct Heap Heap;

CONST_FUNC
isize heap_min_size(void);

CONST_FUNC
isize heap_good_size(void);

/// @brief creates new heap with a backing [VMem] of given size.
/// @remarks if given size is less than value returned by [heap_good_size], a heap of size [heap_good-size] is allocated
/// and returned
Heap* heap_new(isize vmem_size);

/// @brief uses given VMem to create new Heap.
/// @remarks. if given VMem size is less than [heap_min_size], nullptr is returned
Heap* heap_from_vmem(VMem* vm) PARAMS_NONNULL(1);

/// @brief returns the size of allocation poitned to by given pointer.
/// @remarks if given poitner is null or does not exist in this heap, 0 is returned
usize heap_size_of(const Heap* self, const void* ptr) METHOD PURE_FUNC;

/// @vrief same as [heap_size_of] but accounts for block allocation metadata
/// @dtails as of 06/30/2026, this is the size of allocation + 4 bytes
usize heap_full_size_of(const Heap* self, const void* ptr) METHOD PURE_FUNC;

void heap_free(Heap* self, void* ptr) METHOD;

/// @brief allocates new block with size large enough to accomidate given allocation size
/// @details if size is greater than value returned by [heap_max_alloc_size], nullptr is returned
[[gnu::alloc_size(2)]] [[gnu::alloc_align(3)]] [[gnu::malloc]]
void* heap_alloc(Heap* self, isize size, isize align) METHOD;

[[gnu::alloc_size(2)]] [[gnu::alloc_align(3)]] [[gnu::malloc]]
void* heap_zalloc(Heap* self, isize size, isize align) METHOD;

[[gnu::alloc_size(3)]] [[gnu::alloc_align(4)]] [[gnu::malloc]]
void* heap_realloc(Heap* self, void* ptr, isize new_size, isize align) METHOD;

[[gnu::malloc]]
void* heap_reallocate(Heap* self, void* ptr, Layout old, Layout new) METHOD;

bool heap_resize(Heap* self, void* ptr, Layout old, Layout new) METHOD;

/// @brief spreads layout over [heap_alloc] for convienence
[[gnu::malloc]]
static inline void* heap_allocate(Heap* self, Layout layout) {
  return heap_alloc(self, layout.size, layout.align);
}

/// @brief cleans up memory used by given heap
/// @details WARN: Be sure NOT to use the pointer passed to this function after this function returns.
/// Heap pointer is allocted inside the memory it owns so dereferencing it will cause a segfault (if you are lucky! lol)
void heap_destroy(Heap* self);

/// @brief returns true if given pointer was allocated by this heap.
bool heap_contains(const Heap* self, const void* ptr) PURE_FUNC METHOD;

const byte* heap_begin(const Heap* self) METHOD PURE_FUNC;

const byte* heap_end(const Heap* self) METHOD PURE_FUNC;

usize heap_max_alloc_size(void) CONST_FUNC;

/// @brief returns the differnce of the size of allocation and the size of the size class its allocated in.
/// @details when an allocation requeted is less than the size of the block size class its allocated in, i.e. 256 bytes,
/// a block of size 512 bytes is allocated, as such, if that same pointer is reallocated to a size less than 512, it can
/// be resized in place to save space and time
///
/// Keep in mind every allocation uses at least 4 bytes to fit bookkeeping  data for allocation (size and size class
/// index)
///
/// passing a nullptr to this function is considered an error.
/// passing a pointer that does not belong in this heap is considered an error.
///
/// @returns -1 if error occurs, otherwise the size in bytes of available space in this block
isize heap_avail_ptr_size(const Heap* self, const void* ptr) METHOD PURE_FUNC;

HEDLEY_END_C_DECLS
