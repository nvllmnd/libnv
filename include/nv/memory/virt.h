#pragma once
#include <stdint.h>
#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

/// Integral type used for tracking bytes of virtual memory allocated
typedef i32 MemSize;

/// Represents memory that has been paged through mmap/VirtualAlloc. / Ideally
/// you want to create these with significant sizes (2MB+), as this memory is
/// paged on demand and therefore not fully mapped until a read or write into
/// that region of memory is attempted This is not meant to 'resize' or
/// 'relocate'. After there are no more available bytes to allocate, the only
/// way to be able to allocate more is to clear/reset this through
/// [virtmem_clear]/[virtmem_clear_zeroed]. and lets be honest, if your
/// application is exhausting more than 2MB of memory, then you should consider
/// increasing the size of this structure to accommidate the increased demand.
///
/// NOTE: I may eventually add a 'chained' version of this struct, so that when
/// one region of virtual memory is exhausted, we can still keep allocating. OR
/// i can try doing some kind of bookkeeping so that we can try to reuse memory
/// that has been marked as no longer used (freed!), or a combo of both. for now
/// im going to keep it simple, as this on top of [ArenaHeap] is sufficient for
/// most applications methinks
typedef struct VirtMem VirtMem;

/// Maximum value that can be passed to [vmem_new] for its parameter recieving a
/// value for megabytes to allocate
static constexpr const i32 MEMSIZE_MAX_MB = 1024;
/// Minimum value that can be passed to [vmem_new] for its paremter receiveing a
/// value for megabytes to allocate
static constexpr const i32 MEMSIZE_MIN_MB = 2;
/// minimum required allocation size to request when calling [vmem_new] in bytes
static constexpr const MemSize MEMSIZE_MIN = MEGABYTES(MEMSIZE_MIN_MB);

/// maximmum allocation size to request when calling [vmem_new] in bytes.
/// This limit is soley based off the fact that i32 is 4 bytes, and therefore
/// cannot represent a number higher than 1GB, for a signed integer
static constexpr const MemSize MEMSIZE_MAX = GIGABYTES(1);

/// brief Calls [mmap]/[VirtualAlloc] to request memory of @param (size_in_mb)
/// megabytes of virtual paged memory.
///
/// Pointer pointing to virutal memory inside [VirtMem] struct is gauranteed to
/// be aligned, and therefor subsequent allocations will be naturally aligned
/// (therefore allocation methods lack an alignment parameter)
///
/// @param (size_in_mb) :: See [MEMSIZE_MAX_MB] and [MEMSIZE_MIN_MB] for limits
/// on the values that are accepted
///
/// Returns error if virtual allocation fails due to system error or OOM. @param
/// (self) will be zeroed on error and no allocations are made
///
///
METHOD
MemError vmem_init(VirtMem** self, isize size_in_mb);

METHOD
/// returns next available location in memory and increments used counter.
/// This function returns nullptr if there is not enough available space
/// for requested allocation.
void* vmem_allocate(VirtMem* self, MemLayout layout);

/// Same as [vmem_allocate] but ensures that memory is zeroed before returning
/// next poitner. This may be needed if [vmem_clear] is called, but otherwise
/// should not be necessary as allocated vitual memory is zeroed upon
/// successfull [vmem_new] call
METHOD
void* vmem_zallocate(VirtMem* self, MemLayout layout);

/// @brief Byte offset of an allocation
typedef i32 VAddrOffset;

/// @brief calculates the byte offset of a pointer allocated by this allocator. You can use this to ensure you have pointers that point to the correct location in memory, persisting through calls to [vmem_remap]
PURE_FUNC
METHOD
VAddrOffset vmem_offset(const VirtMem* self, const void* ptr);

/// @brief Allocates and sets address offset if not null.
/// @details Same as [vmem_allocate] followed by a call to [vmem_offset]
/// @param (VirtMem* self) -  
/// @param (MemLayout layout) - 
/// @param (VAddrOffset* offset_out) -
METHOD
void* vmem_alloc_offset(VirtMem* self,  MemLayout layout, VAddrOffset* offset_out);


/// @brief Zero Allocates and sets address offset if not null.
/// @details Same as [vmem_zallocate] followed by a call to [vmem_offset]
METHOD
void* vmem_zalloc_offset(VirtMem* self,  MemLayout layout, VAddrOffset* offset);


/// Releases (decommits) virtual memory back to operating system.
/// Note that after this function returns, the structure is zeroed and must be
/// passed to [vmem_new] again in order to allocate again.
///
/// opaque [VirtMem] pointer is set to nullptr before returning
///
/// Returns error if underlying implementation fails to release memory back to
/// system
METHOD
MemError vmem_destroy(VirtMem* self);

// #define vmem_destroy(self) ({\
//   const MemError _err = vmem_destroy((self)); \
//   (self) = nullptr; \
//   _err;\
// })

/// resets allocation used counter back to zero, does not free or release any
/// memory, however any pointers allocated by this [VirtMem] should be
/// considered invalid after calling this function
METHOD
void vmem_clear(VirtMem* self);

/// Resets allocation used counter back to zero and memsets all virtual memory
/// from the beginning of it to given byte index. You can call this instead of
/// [vmem_clear_zeroed] so as to avoid the runtime cost of zeroing 2MB+ of
/// memory,
METHOD
error vmem_zero_range(VirtMem* self, isize index);

/// Same as [vmem_clear], but memsets the entirety of virtual memory to 0.
/// for a version that only clears from the start of virtual memory to a given
/// byte index, see [vmem_zero_range]
METHOD
void vmem_clear_zeroed(VirtMem* self);

/// Returns the size in bytes of the size of virtual memory allocated (not including the header size)
PURE_FUNC
METHOD
isize vmem_size(const VirtMem* self);

/// Same as [vmem_size] but returns the full size of allocation (including the header size)
PURE_FUNC
METHOD
isize vmem_full_size(const VirtMem* self);

/// Returns the number of bytes currently in use
PURE_FUNC
METHOD
isize vmem_used_bytes(const VirtMem* self);

/// Returns number of bytes available for allocation
PURE_FUNC
METHOD
isize vmem_available(const VirtMem* self);

/// Checks if given poitner is a pointer that was allocated from this VirtMem
/// returns true if given pointer lies within the range of virtual memory, owned by [VirtMem]
PURE_FUNC
METHOD
bool vmem_contains(const VirtMem* self, const void* ptr);

CONST_FUNC
RETURNS_NON_NULL
const AllocVTable* vmem_vtable(void);

Allocator vmem_allocator(VirtMem* self);

struct VirtMemView {
  const void* start;
  const void* end;
  i32 size_bytes;
  i32 used_bytes;
  i32 avail_bytes;
};
alias(VirtMemView);

/// Returns a const view into this Virtual Memory
/// For now you can only get a const view into a VirtMem, maybe later
/// ill allow for a mutable view, (but to me that seems like encouraging users to be able to write anywhere in this
/// VirtMem, which would be find if i had the option to inject canaries and such things so i can detect if that happens
/// and abort execution like mimalloc...)
PURE_FUNC
METHOD
VirtMemView vmem_view(const VirtMem* self);

CONST_FUNC
i32 os_page_size(void);

/// @brief remaps virtual memory used by self to given new size in megabytes
/// @warning assume all memory allocated up to before this call is made as invalid after the call has returned
/// @returns pointer to new remapped [VirtMem]
METHOD
VirtMem* vmem_remap(VirtMem* self, i32 new_size_mb);

/// A Heap of Virtual Memory. This is a block style allocator, capable of freeing memory and coalescing adjacent freed
/// blocks
/// TODO: Actually implement this. lol
typedef struct Heap Heap;

static constexpr const i32 VBUFFER_MIN_SIZE = KILOBYTES(4);

/// A large buffer of bytes (4KB+), residing in system virtual memory.
/// This differs from a [VirtMem] in that this type is not an 'allocator' type,
/// this is intended to be used to build strings, arrays, complex datastructures/memory that must
/// be contiguous in memory, before copying that data out of this buffer and into somewhere else, clear this buffer,
/// repeat!
///
/// Since this buffer resides in virtual memory, callers are expectd to keep this buffer alive for a while, and if you
/// think you wont ever need more than [VBUFFER_MIN_SIZE] bytes, then you should probably use a static array
typedef struct VBuffer VBuffer;

// typedef u8 TempBuffer[VBUFFER_MIN_SIZE];

#define Bytes(N) \
  struct {       \
    u8 inner[N]; \
  }

typedef Bytes(KILOBYTES(1)) TempBuffSmall;
typedef Bytes(KILOBYTES(2)) TempBuffMid;
typedef Bytes(KILOBYTES(3)) TempBuffTall;
typedef Bytes(KILOBYTES(4)) TempBuffLarge;

struct TempBuffer {
  u8* inner;
};

// struct TempBuffer {
//   u8 inner[VBUFFER_MIN_SIZE];
// };
// typedef struct TempBuffer TempBuffer;

VBuffer* vbuff_new(i32 cap_in_kb);

// typedef enum SBufferType {
//   SBuffer__Inner,
//   /// non-owned
//   SBuffer__Foreign,
//   SBuffer__VirtualMem

// } SBufferType;

// struct ScratchBuff {
//   SBufferType type;

//   u8* top;

//   union {
//     u8 inner[SBUFFER_INNER_SIZE];
//     struct { u8* start; u8* end; } foreign;
//     VirtMem* mem;
//   };

// };


/// @brief A memory location marker returned from [vmem_mark].
/// @details can later be passed to [vmem_reset_to] to set back its internal used counter back to where it was when
/// [vmem_mark] was first called This allows you to clear sections of virtual memory to be reused by later
/// allocations, while still keeping allocations before first call to [vmem_mark] intact and valid
typedef i32 VMarker;

METHOD
PURE_FUNC
VMarker vmem_mark(const VirtMem* self);

METHOD
/// @brief Resets allocations made from given marker
/// @details a [VMarker] can be returned from a call to [vmem_mark], which is a memory
/// location to 'reset' to. All allocations made after given marker are considered freed for reused
/// and should be considered invalid after this function returns
void vmem_reset_to(VirtMem* self, VMarker marker);
