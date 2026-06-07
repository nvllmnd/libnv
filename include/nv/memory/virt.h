#pragma once
#include <stdarg.h>
#include <stdint.h>

#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

/// @brief opaque type representing demand-paged virtual memory
/// @details This type is functionally similar to an Arena Allocator.
/// NOTE: (06/07/2026) Due to the above observation (about this being functionally similar to an Arena Allocator), i
/// have removed the Arena type in arena.c/arena.h, just doesnt seem needed, as most the time I reach for this type
/// anyway lol
typedef struct VirtMem VirtMem;

/// @brief opaque pointer to [VirtMem]
typedef VirtMem* VirtHndl;

/// @brief descriptive alias for VirtMem method functions
typedef VirtMem* VirtSelf;

/// @brief Essentially a wrapper around flags passed to [mmap]
/// @details This falls under the [VirtMem] API, so any mmap flags specific to memory mapping of files
/// are not supported/present here (MAP_PRIVATE|MAP_ANONYMOUS are the base/minimum flags passed to [mmap])
typedef enum HEDLEY_FLAGS VMapAccess {
  VMap__NoAccess = 0,
  VMap__Read = 1,
  VMap__Write = 2,
  VMap__DefaultAccess = VMap__Read | VMap__Write,
} VMapAccess;

typedef enum HEDLEY_FLAGS VMapMode {

  /// @brief default behavior
  /// @details passes MAP_PRIVATE|MAP_ANONYMOUS with no additional flags
  VMap__PrivAnon = 0,
  /// @brief passes [MAP_NORESERVE] to mmap.
  /// @details adds [MAP_NORESERVE] on top of the default flags
  ///
  /// @remarks Turns out this flag is pretty important if we plan on using overcommit, so this is added to the default
  /// flags
  VMap__NoReserve = 1,
  /// @brief passes [MAP_LOCKED] to mmap
  /// @details adds [MAP_LOCKED] on top of the default flags
  //
  VMap__LockAll = 1 << 1,
  /// @brief passes [MAP_POPULATE] to mmap
  /// @details adds [MAP_POPULATE] on top of the default flags
  VMap__CommitAll = 1 << 2,
  /// @brief calls [mlock] on caller given size after successful mmap.
  /// @details memory remains locked until [vmem_unlock] is called
  VMap__LockPages = 1 << 3,
  /// @brief calls [madvise] with [MADV_WILLNEED] on caller given sizes after successful mmap
  VMap__CommitPages = 1 << 4,

  /// @brief MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE
  VMap__Default = VMap__PrivAnon | VMap__NoReserve
} VMapMode;

/// @brief Remap flags for [vmem_remap].
/// @details right now i dont support [MREMAP_DONTUNMAP], but im keeping this an enum for when i do (if ever)
typedef enum VRemapMode {
  /// @brief default mremap flags, will fail if system cannot expand this virtual memory in place
  VRemap__ExpandInPlace = 0,
  /// @brief allows mapping to be reloacted to a differnt region of memory
  VRemap__AllowRelocate,
} VRemapMode;

/// @brief Initialization struct for initializing new [VirtMem] with extended options
struct VirtMemOpts {
  /// @details count in bytes of requested virtual memory mapping
  i64 size_bytes;
  /// @brief count bytes user wishes to be prefaulted.
  /// @details This value is ignored if [VMap__PrefaultSize] is not set.
  /// if value is negative, all pages are prefaulted and the behavior is equivalent to passing [VMap__PrefaultAll]. If
  /// value is 0, nothing is done if value is greater than the amount of pages this virtual mapping owns, then all pages
  /// are prefaulted just as if enabling the [VMap__PrefaultAll] flag
  i64 commit_bytes;

  /// @brief count of bytes user wishes to be locked into physical RAM.
  /// @details This value is ignored if [VMap__LockSize] is not set.
  /// if value is negative, all pages are locked and the behavior is equivalent to passing [VMap__LockAll]. If value is
  /// 0, nothing is done if value is greater than the amount of pages this virtual mapping owns, then all pages are
  /// locked just as if enabling the [VMap__LockAll] flag
  i64 lock_bytes;
  /// @brief a striped down version of POSIX mmap/unmap/mremap API
  /// @details We dont support mapping files or shared memory (yet) as well as some other <sys/mman.h> flags
  VMapMode mode;

  VMapAccess access;
};
alias(VirtMemOpts);

/// @brief cretes new [VirtMemOpts] with given size and default flags
CONST_FUNC
static inline VirtMemOpts vmem_opts_default_new(i64 size_bytes) {
  return make(VirtMemOpts, .size_bytes = size_bytes, .commit_bytes = 0, .lock_bytes = 0, .mode = VMap__Default,
              .access = VMap__DefaultAccess);
}

static constexpr const i64 VMEM_MAX_SIZE_BYTES = INT64_MAX;

/// brief Calls [mmap]/[VirtualAlloc] to request memory of @param (size_in_mb)
/// megabytes of virtual paged memory.
///
/// @param (i64 size_bytes) :: Minimum size allowed is the size returned by [os_page_size]. Smaller values are ignored
/// and [os_page_size] is used instead
///
/// Returns error if virtual allocation fails due to system error or OOM. @param
/// (self) will be zeroed on error and no allocations are made
///
///
METHOD
NvError vmem_init(VirtHndl* self, i64 size_bytes);

/// @brief same as [vmem_init], but with extended initialization params!
// METHOD
NvError vmem_init_ex(VirtHndl* self, VirtMemOpts opts);

METHOD
/// returns next available location in memory and increments used counter.
/// This function returns nullptr if there is not enough available space
/// for requested allocation.
void* vmem_allocate(VirtSelf self, MemLayout layout);

/// Same as [vmem_allocate] but ensures that memory is zeroed before returning
/// next poitner. This may be needed if [vmem_clear] is called, but otherwise
/// should not be necessary as allocated vitual memory is zeroed upon
/// successfull [vmem_new] call
METHOD
void* vmem_zallocate(VirtSelf self, MemLayout layout);

/// @brief Byte offset of an allocation
/// @details Must always be a positive value, any negative values are treated as errors
typedef i64 VAddrOffset;

/// @brief calculates the byte offset of a pointer allocated by this allocator.
/// @remarks You can use this to ensure you have
/// pointers that point to the correct location in memory, persisting through calls to [vmem_remap].
/// Keep in mind this is a simple calculation, so if ptr is a pointer that was in this mapping prior to a reloaction, it
/// will appear as if we dont own it and this function will return -1
/// @details if pointer parameter does not exist in this virtual memory region, this function returns -1
PURE_FUNC
METHOD
VAddrOffset vmem_offset(const VirtMem* self, const void* ptr);

/// @brief Allocates and sets address offset if not null.
/// @details Same as [vmem_allocate] followed by a call to [vmem_offset],
/// @param (MemLayout layout) - Allocation size
/// @param (VAddrOffset* offset_out) - if not-null, sets as the relative offset of the pointer returned by this
/// function. Ignored if null
METHOD
void* vmem_alloc_offset(VirtSelf self, MemLayout layout, VAddrOffset* offset_out);

/// @brief Allocates and sets address offset if not null.
/// @details Same as [vmem_allocate] followed by a call to [vmem_offset],
/// @param (MemLayout layout) - Allocation size
/// @param (VAddrOffset* offset_out) - if not-null, sets as the relative offset of the pointer returned by this
/// function. Ignored if null//
METHOD
void* vmem_zalloc_offset(VirtSelf self, MemLayout layout, VAddrOffset* offset);

#define vmem_allocate_tp(_self, T) ((__typeof(T)*)vmem_allocate((_self), mlayout_new(T)))
#define vmem_zallocate_tp(_self, T) ((__typeof(T)*)vmem_zallocate((_self), mlayout_new(T)))

#define vmem_allocate_array(_self, T, N) ((__typeof(T)*)vmem_allocate((_self), mlayout_array(T, N)))
#define vmem_allocate_vec(_self, T, _n) ((__typeof(T)*)vmem_allocate((_self), mlayout_vec(T, _n)))

#define vmem_zallocate_array(_self, T, N) ((__typeof(T)*)vmem_zallocate((_self), mlayout_array(T, N)))
#define vmem_zallocate_vec(_self, T, _n) ((__typeof(T)*)vmem_zallocate((_self), mlayout_vec(T, _n)))

#define vmem_alloc_offset_tp(_self, T, _out) ((__typeof(T)*)vmem_alloc_offset((_self), mlayout_new(T), (_out)))
#define vmem_zalloc_offset_tp(_self, T, _out) ((__typeof(T)*)vmem_zalloc_offset((_self), mlayout_new(T), (_out)))
#define vmem_alloc_offset_array(_self, T, N, _out) \
  ((__typeof(T)*)vmem_alloc_offset((_self), mlayout_array(T, N), (_out)))
#define vmem_alloc_offset_vec(_self, T, _n, _out) ((__typeof(T)*)vmem_alloc_offset((_self), mlayout_vec(T, _n), (_out)))
#define vmem_zalloc_offset_array(_self, T, N, _out) \
  ((__typeof(T)*)vmem_zalloc_offset((_self), mlayout_array(T, N), (_out)))
#define vmem_zalloc_offset_vec(_self, T, _n, _out) \
  ((__typeof(T)*)vmem_zalloc_offset((_self), mlayout_vec(T, _n), (_out)))

/// @brief updates a pointer to a relative address.
/// @remarks use this to update a poitner and ensure its correctly poiting to the right location in memory (this is how
/// you get away with relocatable remaps, after which you can still refer to all your memory through VAddrOffset)
PARAMS_NONNULL(1, 2)
void vmem_update_ptr(VirtSelf self, void** ptr, VAddrOffset rel_address);

/// @brief looks up a pointer at a relative address.
/// @remarks This function will terminate execution if rel_address is out of range or negative
METHOD
RETURNS_NON_NULL
void* vmem_lookup_offset(VirtSelf self, VAddrOffset rel_address);

/// Releases (decommits) virtual memory back to operating system.
/// Note that after this function returns, the structure is zeroed and must be
/// passed to [vmem_new] again in order to allocate again.
///
/// opaque [VirtMem] pointer is set to nullptr before returning
///
/// Returns error if underlying implementation fails to release memory back to
/// system
/// NOTE: This function does not take a pointer to a pointer (so it can automatically be set to nullptr),
/// because a common patter is to store a poitner to VirtMem in a child allocations header data, so
/// when we set that pointer to nullptr after destroying the VirtMem, we get a segfault because we just wrote nullptr to a memory that was just freed!
METHOD
NvError vmem_destroy(VirtSelf self);

// #define vmem_destroy(self) ({\
//   const MemError _err = vmem_destroy((self)); \
//   (self) = nullptr; \
//   _err;\
// })

/// resets allocation used counter back to zero, does not free or release any
/// memory, however any pointers allocated by this [VirtMem] should be
/// considered invalid after calling this function
METHOD
void vmem_clear(VirtSelf self);

/// Resets allocation used counter back to zero and memsets all virtual memory
/// from the beginning of it to given byte index. You can call this instead of
/// [vmem_clear_zeroed] so as to avoid the runtime cost of zeroing 2MB+ of
/// memory,
METHOD
error vmem_zero_range(VirtSelf self, isize index);

/// Same as [vmem_clear], but memsets the entirety of virtual memory to 0.
/// for a version that only clears from the start of virtual memory to a given
/// byte index, see [vmem_zero_range]
METHOD
void vmem_clear_zeroed(VirtSelf self);

/// Returns the size in bytes of the size of virtual memory allocated (not including the header size)
PURE_FUNC
METHOD
i64 vmem_size(const VirtMem* self);

/// Same as [vmem_size] but returns the full size of allocation (including the header size)
PURE_FUNC
METHOD
i64 vmem_full_size(const VirtMem* self);

/// Returns the number of bytes currently in use
PURE_FUNC
METHOD
i64 vmem_used_bytes(const VirtMem* self);

/// Returns number of bytes available for allocation
PURE_FUNC
METHOD
i64 vmem_available(const VirtMem* self);

/// Checks if given poitner is a pointer that was allocated from this VirtMem
/// returns true if given pointer lies within the range of virtual memory, owned by [VirtMem]
PURE_FUNC
METHOD
bool vmem_contains(const VirtMem* self, const void* ptr);

CONST_FUNC
RETURNS_NON_NULL
const AllocVTable* vmem_vtable(void);

Allocator vmem_allocator(VirtSelf self);

struct VirtMemView {
  const void* start;
  const void* end;
  i64 size_bytes;
  i64 used_bytes;
  i64 avail_bytes;
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
i64 os_page_size(void);

/// @brief remaps virtual memory used by self to given new size in megabytes
///
/// @returns pointer to new remapped [VirtMem]
METHOD
NvError vmem_remap(VirtHndl* self, i64 size_bytes, VRemapMode mode);

/// @brief same as @see [vmem_remap], but always passes [VRemap__ExpandInPlace] as mode parameter
METHOD
static inline NvError vmem_expand(VirtHndl* self, i64 size_bytes) {
  return vmem_remap(self, size_bytes, VRemap__ExpandInPlace);
}

/// @brief same as @see [vmem_remap], but always passes [VRemap__AllowRelocate] as mode parameter
METHOD
static inline NvError vmem_remap_move(VirtHndl* self, i64 new_size) {
  return vmem_remap(self, new_size, VRemap__AllowRelocate);
}

/// @brief A memory location marker returned from [vmem_mark].
/// @details can later be passed to [vmem_reset_to] to set back its internal used counter back to where it was when
/// [vmem_mark] was first called This allows you to clear sections of virtual memory to be reused by later
/// allocations, while still keeping allocations before first call to [vmem_mark] intact and valid
typedef i64 VMarker;

METHOD
PURE_FUNC
VMarker vmem_mark(const VirtMem* self);

METHOD
/// @brief Resets allocations made from given marker
/// @details a [VMarker] can be returned from a call to [vmem_mark], which is a memory
/// location to 'reset' to. All allocations made after given marker are considered freed for reuse
/// and should be considered invalid after this function returns
void vmem_reset_to(VirtSelf self, VMarker marker);

/// @brief Same as [vmem_reset_to], but zeroes the memory that was backtracked/reset
METHOD
void vmem_reset_zeroed(VirtSelf self, VMarker marker);

/// @brief calls [mlock] on up to n bytes
///
/// @details pages remain locked in physical memory until [vmem_unlock] is called
/// pages locked begin from start of virtual memory up to n pages
/// @returns an error associated with inner [mlock] call
///
NvError vmem_lock(VirtSelf self, i64 nbytes);

/// @brief calls [munlock] on up to n bytes
/// @details reverses the effects of [vmem_lock]
/// @returns an error associated with inner [munlock] call
NvError vmem_unlock(VirtSelf self, i64 nbytes);

/// @brief calls [madvise] with [MADV_WILLNEED] on up to n bytes
NvError vmem_commit(VirtSelf self, i64 nbytes);

/// @brief format allocates a null-terminated string slice in printf style
///@remarks If the expanded formatted string is larger than available memory,
/// string is truncated by the available size. After which any allocations made wil result in a nullptr (unless caller
/// remaps!)
HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice vmem_fslice(VirtSelf self, const char* fmt, ...);

/// @brief format allocates a null-terminated string slice in printf style
///@remarks If the expanded formatted string is larger than available memory,
/// string is truncated by the available size. After which any allocations made wil result in a nullptr (unless caller
/// remaps!)

METHOD
sslice vmem_vfslice(VirtSelf self, const char* fmt, va_list args);

/// @brief format allocates a null-terminated string in printf style
/// @details you can pass an optional pointer to i64 to also get the allocated string's length
/// @param (VirtMem* self) - selfptr
/// @param (i64* len_out) - optional length out parameter, excluding null character
/// @remarks If the expanded formatted string is larger than available memory,
/// string is truncated by the available size. After which any allocations made wil result in a nullptr (unless caller
/// remaps!)
HEDLEY_PRINTF_FORMAT(3, 4)
METHOD
char* vmem_fstring(VirtSelf self, i64* len_out, const char* fmt, ...);

/// @brief format allocates a null-terminated string in printf style
///
/// @details you can pass an optional pointer to i64 to also get the allocated string's length
/// @remarks If the expanded formatted string is larger than available memory,
/// string is truncated by the available size. After which any allocations made wil result in a nullptr (unless caller
/// remaps!)
METHOD
char* vmem_vfstring(VirtSelf self, i64* len_out, const char* fmt, va_list args);

/// @brief duplicates given string of length into this VirtMem and appends a null character to the end
/// @details if this VirtMem has less available free bytes than given length, string is truncated to fill the remaining space
METHOD
char* vmem_strndup(VirtSelf self, const char* str, i32 len);


/// @brief duplicates given string of length into this VirtMem and appends a null character to the end
/// @details if this VirtMem has less available free bytes than given length, string is truncated to fill the remaining space
/// @see [vmem_strndup]
METHOD
char* vmem_strdup(VirtSelf self, const char* str);


METHOD
/// @brief 'deletes' n most recently allocated bytes.
/// @details This is a constant time function, all it does it subtract given count of bytes
/// from its inner used counter(or pointer if it doesnt use a counter), effectively deleting
/// byte from the most recent allocation (and prior allocations before that if large enough)
/// @warning Use this function with care! treat memory that has been deleted this way as if you no longer own it!
/// allocation made after this call will use the bytes that were most deleted by this function as new memory!
/// To be really sure/safe you can call [vmem_delzero_back], which does the exact same thing as this function, but
/// zeroes the bytes it deletes
/// @returns Bytes available after this function completes (or [vmem_available](prior to call ing this function) -
/// nbytes)
i64 vmem_delete_back(VirtSelf self, i64 nbytes);

/// @brief same as [vmem_delete_back] but zeroes its memory
METHOD
i64 vmem_delzero_back(VirtSelf self, i64 nbytes);

#ifndef NV_ALIAS_VIRTMEM_AS_ARENA
#define NV_ALIAS_VIRTMEM_AS_ARENA 0
#endif

// NOTE: The following typedef is to minimize the work needed to replace code already using the Arena type,
#if defined(NV_ALIAS_VIRTMEM_AS_ARENA) && NV_ALIAS_VIRTMEM_AS_ARENA == 1

typedef VirtMem Arena;

#endif
