#pragma once

#include "attributes.h"
#include "core_types.h"
#include "intdefs.h"
#include "memory/alloc.h"

typedef struct ArenaHeap ArenaHeap;

struct ArenaHeapStats {
  i64 total_used;
  i64 total_allocated;
};

typedef struct ArenaHeapStats ArenaHeapStats;
typedef struct VirtMem VirtMem;

/// Creates a new ArenaHeap with @param (isize init_capacity) of initial allocated space into a newly
/// allocated virtual memory page of size @param (isize vmem_size_in_mb)
/// Returned ArenaHeap exclusively owns its backing VirtMem, and will be released after a matching call to [arena_heap_destroy] or [arena_heap_cleanup]
RETURNS_RESOURCE
ArenaHeap* arena_heap_new(isize vmem_size_in_mb, isize init_capacity);

/// Creates a new ArenaHeap with @param (isize capacity) initial allocated space into given @param (VirtMem* vm).
/// If @param (bool exclusive) is true, the returned ArenaHeap will be deallocated/released after a matching call to [arena_heap_destroy] or [arena_heap_cleanup]
RETURNS_RESOURCE
PARAMS_NONNULL(1)
ArenaHeap* arena_heap_in_vmem(VirtMem* vm, isize capacity, bool exclusive);

METHOD
void* arena_heap_alloc(ArenaHeap* self, MemLayout layout);

METHOD
void* arena_heap_zalloc(ArenaHeap* self, MemLayout layout);

METHOD
void arena_heap_clear(ArenaHeap* self);

/// Destroys given ArenaHeap. If this ArenaHeap owns its VirtMem field exclusively, this function decommits/releases
/// that virtual memory block, otherwise calls arena_heap_clear
///
/// For a version of this function that only tries to release backing virtual memory and does nothing in the case this
/// ArenaHeap does not exclusively own its backing VirtMem, see [arena_heap_cleanup]
///
/// returns true if memory has been decommitted (in case this ArenaHeap exclusively owns its VirtMem), otherwise false
METHOD
bool arena_heap_destroy(ArenaHeap* self);

/// Same as [arena_heap_destroy], but does nothing in the case this ArenaHeap does not exclusively own its backing VirtMem
METHOD
void arena_heap_cleanup(ArenaHeap* self);

/// Returns allocation statistics gathered by use of this ArenaHeap. 
PURE_FUNC
METHOD
ArenaHeapStats arena_heap_stats(ArenaHeap* self);

/// Returns the static constant Allocation vtable associated with any ArenaHeap
CONST_FUNC
RETURNS_NON_NULL
const AllocVTable* arena_heap_alloc_vtable(void);

/// Returns the Allocator interface struct associated with given ArenaHeap
METHOD
Allocator arena_heap_allocator(ArenaHeap* self);

