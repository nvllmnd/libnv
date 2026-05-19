//! Block-Style Allocator
//!
//!
#pragma once

#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

/// Block-style allocator
/// Allocates from a backing VirtMem poitner, that can be marked as exclusively owned. If exclusively owned,
/// There are some space and time optimizations that any BlockAllocator can take, knowing it can release backing memory,
/// or easily reset it
///
/// This allocator is somewhat similar to [ArenaHeap] or [VirtMem], except that it is able to free and reallocate
/// memory.
///
/// This type internally keeps a statically sized array of pointers to blocks that are marked as free,
/// the length of this free list can be modified by defining the BA_FL_SIZE macro. Currently a default value of 1024 is used.
/// If the FreeList is filled up when trying to add a new freed block to it, it overwrites the freed block that has remained inside the FreeList the longest.
/// This can techincally lead to memory leaks (though any 'leaked' memory will be cleaned up when a destroying BlockAllocator), but since we are
/// removing the oldest freed allocation, its likely that old allocation is not meant to be freed until BlockAllocator is destroyed. After all, if you end up allocating 1024 objects,
/// the oldest free block in that list most likely is staying around for a lot longer lol.
///
/// All this in mind, this Allocator works best and is most efficient for frequent allcoations and frees, or at least if you throw a free in here and there, there is less chance of
/// ending up in a situtation where memory is leaked, unable to be reused/freed until this instance is destroyed
///
///
typedef struct BlockAllocator BlockAllocator;

/// forward declaration for [VirtMem]
typedef struct VirtMem VirtMem;

/// Default size in megabytes that a [BlockAllocator] initializes its backing [VirtMem] in the case
/// a nullptr is passed to [ba_init]/[ba_new]
static constexpr const isize BA_BACKING_VIRTMEM_SIZE_MB = 4;

/// Creates a new BlockAllocator, that exclusively owns its backing VirtMem. Given parameter is
/// size in megabytes of the backing virtmem virtual memory reservation size
RETURNS_RESOURCE
BlockAllocator* ba_owned_new(isize vm_mb);

/// Initializes new Block Allocator from a backing [VirtMem] poitner.
/// Backing VirtMem pointer is optional, if passed as null, This block allocator will create a new one and
/// own it exclusively.
///
/// @param bool exclusive - ignored if @param (VirtMem* backing) is null, as that would mean that this block allocator
/// will create the backing VirtMem, and therefor own it exclusively
///
/// if @param (bool exclusive) is set to true, but second param (backing [VirtMem]) is begin referenced/used somewhere
/// else, UB may happen if [ba_destroy] is called, as [ba_destroy] releases/decommits its backing VirtMem if it
/// exclusively owns it, similar to [ArenaHeap]
HANDLE_ERROR
MemError ba_init(BlockAllocator** out, VirtMem* backing, bool exclusive);

/// Same as [ba_init], but returns a nullptr in the case of any errors.
///
/// WARN: if @param (bool exclusive) is false, @param(VirtMem* backing) MUST NOT BE USED outside of the lifetime of
/// returned [BlockAllocator], (beyond a matching [ba_destroy] call) as calls to [ba_destroy] will decommit/release its
/// backing [VirtMem] if the [BlockAllocator] passed to it exlusively owns its backing [VirtMem]
RETURNS_RESOURCE
BlockAllocator* ba_new(VirtMem* backing, bool exclusive);

/// Creates a block from size and alignment from @param (MemLayout layout)
/// returns pointer to userdata section of allocated block. Actual size of allocated block may be larger than
/// requested size in @param (MemLayout layout) to accommidate for more allocations per block, or for other memory
/// optimizations, like block merging
METHOD
void* ba_allocate(BlockAllocator* self, MemLayout layout);

/// Same as [ba_allocate], but ensures userdata memory is zeroed before returning
/// Backing VirtMem zeroes out its memory already, so this function should not be necessary unless [ba_clear]/[ba_reset]
/// is called, as they do not zero memory, they just set pointers/counters back to start
METHOD
void* ba_zallocate(BlockAllocator* self, MemLayout layout);

/// Attempts to reallocate @param (void* ptr) to a new location in memory, by searching for a new free block of
/// appropriate size. marks block pointed to by @param (void* ptr) as free if sucessful. The same poitner is returned if
/// @param (MemLayout new_layout) is appropriate size to resize @param (void* ptr) in place (for instance, if old layout
/// is smamller than new layout, or if there is enough space in block to accomidate for new layout)
///
/// @param (MemLayout old_layout) is entirely optional, and can be passed as a zeroed struct, as block size is kept in
/// block header so there is no need to know the old layout size
METHOD
void* ba_reallocate(BlockAllocator* self, void* ptr, MemLayout old_layout, MemLayout new_layout);

/// Marks @param (void* ptr) as free for use by a future allocation of appropriate size
METHOD
void ba_free(BlockAllocator* self, void* ptr);

/// Destroys this BlockAllocator.
///
/// Checks if this BlockAllocator exclusively owns its backing VirtMem, if so it releases/decommits it back to the
/// system. As such if any references to that backing [VirtMem] still exist, they should be considered invalid
///
METHOD
MemError ba_destroy(BlockAllocator* self);

/// Const function, gets VTable associated for any BlockAllocator. Required for implementing the [Allocator] interface
/// struct
PURE_FUNC
const AllocVTable* ba_vtable(void);

/// Returns an [Allocator] interface struct for any given [BlockAllocator]
PURE_FUNC
METHOD
Allocator ba_allocator(BlockAllocator* self);

/// Does @param(const void* ptr) reside inside memory region owned/referenced by @param (const BlockAllocator* self)?
/// Returns true if given ptr exists inside the memory region owned/referenced by self, otherwise false.
///
/// This is a constant time operation, as it simply checks if ptr is >= start or <= end
PURE_FUNC
METHOD
bool ba_contains(const BlockAllocator* self, const void* ptr);

