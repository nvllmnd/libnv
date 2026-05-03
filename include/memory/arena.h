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



RETURNS_RESOURCE
ArenaHeap* arena_heap_new(isize capacity);

RETURNS_RESOURCE
ArenaHeap* arena_heap_in_vmem(VirtMem vm, isize capacity);

METHOD
void* arena_heap_alloc(ArenaHeap* self, isize size, isize align);

METHOD
void* arena_heap_zalloc(ArenaHeap* self, isize size, isize align);

METHOD
void arena_heap_clear(ArenaHeap* self);

/// Starts from the last Block (or the first block allocated as root, not including the memory block in ArenaHeap root)
/// and frees/releases the memory used by that block back to the system. Tries to release up to @param (nblocks).
/// if @param (nblocks) <= 0, then all blocks used by this [ArenaHeap] are freed, and the memory in [ArenaHeap] root is
/// zeroed
///
/// If you want to keep all blocks currently allocated, but would like to reset/clear all memory and reset all blocks
/// used counters, see [arena_heap_clear]
// METHOD
// void arena_heap_release(ArenaHeap* self, isize nblocks);

/// Destroys given ArenaHeap entirely, freeing all memory used by it
/// As such, the pointer is invalid after this funciton returns and should be discarded
METHOD
void arena_heap_destroy(ArenaHeap* self);

PURE_FUNC
METHOD
ArenaHeapStats arena_heap_stats(ArenaHeap* self);

CONST_FUNC
RETURNS_NON_NULL
const AllocVTable* arena_heap_alloc_vtable(void);

METHOD
Allocator arena_heap_allocator(ArenaHeap* self);


struct OsArena {
  struct ArenaHeap* base;
  VirtMem vm;
};
alias(OsArena);

RETURNS_RESOURCE
OsArena os_arena_new(i32 size_mb, isize init_commit);

#define os_arena_alloc(self, size, align) (arena_heap_alloc((self).base), size, align)
#define os_arena_zalloc(self, size, align) (arena_heap_zalloc((self).base, size, align))
#define os_arena_clear(self) (arena_heap_clear((self).base))
#define os_arena_destroy(self) (arena_heap_destroy((self).base))
#define os_arena_stats(self) (arena_heap_stats((self).base))
#define os_arena_allocator(self) (arena_heap_allocator((self).base))
