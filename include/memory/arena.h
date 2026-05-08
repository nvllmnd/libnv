#pragma once

#include "attributes.h"
#include "core_types.h"
#include "intdefs.h"
#include "memory/alloc.h"

typedef struct Arena Arena;

struct ArenaStats {
  i64 total_used;
  i64 total_allocated;
};

typedef struct ArenaStats ArenaStats;
typedef struct VirtMem VirtMem;

/// Creates a new ArenaHeap with @param (isize init_capacity) of initial allocated space into a newly
/// allocated virtual memory page of size @param (isize vmem_size_in_mb)
/// Returned ArenaHeap exclusively owns its backing VirtMem, and will be released after a matching call to [arena_heap_destroy] or [arena_heap_cleanup]
RETURNS_RESOURCE
Arena* arena_new(isize vmem_size_in_mb, isize init_capacity);

/// Creates a new ArenaHeap with @param (isize capacity) initial allocated space into given @param (VirtMem* vm).
/// If @param (bool exclusive) is true, the returned ArenaHeap will be deallocated/released after a matching call to [arena_heap_destroy] or [arena_heap_cleanup]
RETURNS_RESOURCE
PARAMS_NONNULL(1)
Arena* arena_in_vmem(VirtMem* vm, isize capacity, bool exclusive);

METHOD
void* arena_alloc(Arena* self, MemLayout layout);

METHOD
void* arena_zalloc(Arena* self, MemLayout layout);

METHOD
void arena_clear(Arena* self);

/// Destroys given ArenaHeap. If this ArenaHeap owns its VirtMem field exclusively, this function decommits/releases
/// that virtual memory block, otherwise calls arena_heap_clear
///
/// For a version of this function that only tries to release backing virtual memory and does nothing in the case this
/// ArenaHeap does not exclusively own its backing VirtMem, see [arena_heap_cleanup]
///
/// returns true if memory has been decommitted (in case this ArenaHeap exclusively owns its VirtMem), otherwise false
METHOD
bool arena_destroy(Arena* self);

/// Same as [arena_heap_destroy], but does nothing in the case this ArenaHeap does not exclusively own its backing VirtMem
METHOD
void arena_cleanup(Arena* self);

/// Returns allocation statistics gathered by use of this ArenaHeap. 
PURE_FUNC
METHOD
ArenaStats arena_stats(Arena* self);

/// Returns the static constant Allocation vtable associated with any ArenaHeap
CONST_FUNC
RETURNS_NON_NULL
const AllocVTable* arena_alloc_vtable(void);

/// Returns the Allocator interface struct associated with given ArenaHeap
METHOD
Allocator arena_allocator(Arena* self);

