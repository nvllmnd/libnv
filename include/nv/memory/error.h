// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

BEGIN_C_DECLS

typedef enum nv_nodiscard_msg("Ignoring functions returning NvError type could lead to segfaults!") NvError : uerror {

  /// Ok, No Error!
  Error__Ok = 0,

  /// General catchall error.
  /// Generally, catchall errors are equal exactly to 1, but dont have to be,
  /// and an error may have this bit set as well as other bits set
  NVERROR = 1,

  /// mmap errors...
  Error__FailedMemMap = 1L << 1,
  Error__VMapCannotBeResized = 1L << 2,
  Error__VMapInvalidRemapFlags = 1L << 3,
  Error__VMapInvalidMapFlags = 1L << 4,
  Error__FailedToPreloadOOM = 1L << 5,
  Error__VMapCannotLockToRAM = 1L << 6,
  Error__CannotUnlockRAM = 1L << 7,
  Error__VMapCannotShrink = 1L << 8,
  Error__VMapCannotGrow = 1L << 9,
  Error__VMapCannotBeMoved = 1L << 10,
  Error__VMapCannotExpandInPlace = 1L << 11,
  Error__FailedMemUnmap = 1L << 12,
  Error__NotEnoughPhysicalRAMAavailable = 1L << 13,
  Error__BFileErrorTooSmall = 1L << 14,
  Error__MAdviseWillNeedFailed = 1L << 15,
  Error__FailedRemap = 1L << 16,
  Error__CannotExpandInPlace = 1L << 17,
  Error__InvalidAllocationSize = 1L << 18,
  Error__BufferNeedsResize = 1L << 19,
  Error__VirtMemOutOfMemory = 1L << 20,

  /// @brief Not enough space/cannot allocate memory (POSIX.1-2001).
  Error__OOM = 1 << 21,
  Error__FilePathTooLong = 1L << 22,
  Error__FileMapFailedToLoad = 1L << 23,
  /// @brief for errors not covered by other Param* variants
  Error__ParamInvalid = 1L << 24,
  /// @brief function expected its first parameter to be non-null, but was null!
  Error__ParamInvalidMethod = 1L << 25,
  Error__ParamInvalidNull = 1L << 26,

  /// @brief paramter expected to be positive integer
  Error__ParamUnexpectedNegInt = 1L << 27,

  Error__ParamUnexpectedNegFloat = 1L << 28,
  Error__IndexOutOfRange = 1L << 29,
  Error__ParamUnexpectedNegOrZeroInt = 1L << 30,
  Error__ParamUnexpectedNegOrZeroFloat = 1L << 31,
  Error__VMemFailedToLockRangeToRAM = 1L << 32,
  Error__VMemFailedToUnlockRangeFromRAM = 1L << 33,
  Error__PtrNotOwnedByVMem = 1L << 34,
  Error__VMemFailedToPrefaultRange = 1L << 35,
  Error__UnexpectedMisAlignedPtr = 1L << 36,
  Error__AllocationSizeTooSmall = 1L << 37,
  Error__FileNotFound = 1L << 38,
  Error__DirectoryNotFound = 1L << 39,
  Error__FailedFileMemoryMap = 1L << 40,
  Error__FileMapAlreadyInitialized = 1L << 41,
  Error__TooManyFiles = 1L << 42,
  Error__FilePermissionDenied = 1L << 43,
  Error__FileNotADirectory = 1L << 44,
  Error__FileTooBig = 1L << 45,

  Error__FileIO = 1L << 46,

  // Error__ParamExpectedPosNonZeroInt = 1 << 30,

  // Error__ParamExpectedPosNonZeroFloat= 1 << 30,

  ERROR_COUNT = 47L,
} HEDLEY_FLAGS NvError;

static constexpr const NvError NVOK = Error__Ok;

/// @remarks Currently an [NvError] can contain up to [ERROR_COUNT] number of errors simultaneously.
/// but - also currently - this function can only detect if given error value exactly matches each [NvError] variant
/// exactly, so it may report valid error values as invalid
const char* error_string(NvError err) CONST_FUNC RETURNS_NON_NULL;

END_C_DECLS
