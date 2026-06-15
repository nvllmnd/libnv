// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

typedef enum nv_nodiscard_msg("Ignoring functions returning NvError type could lead to segfaults!") NvError : uerror {

  /// Ok, No Error!
  Error__Ok = 0,

  /// General catchall error.
  /// Generally, catchall errors are equal exactly to 1, but dont have to be,
  /// and an error may have this bit set as well as other bits set
  ERROR = 1,

  /// mmap errors...
  Error__FailedMemMap = 1 << 1,
  Error__VMapCannotBeResized = 1 << 2,
  Error__VMapInvalidRemapFlags = 1 << 3,
  Error__VMapInvalidMapFlags = 1 << 4,
  Error__FailedToPreloadOOM = 1 << 5,
  Error__VMapCannotLockToRAM = 1 << 6,
  Error__CannotUnlockRAM = 1 << 7,
  Error__VMapCannotShrink = 1 << 8,
  Error__VMapCannotGrow = 1 << 9,
  Error__VMapCannotBeMoved = 1 << 10,
  Error__VMapCannotExpandInPlace = 1 << 11,
  Error__FailedMemUnmap = 1 << 12,
  Error__NotEnoughPhysicalRAMAavailable = 1 << 13,
  Error__VMemLimitReached = 1 << 14,
  Error__MAdviseWillNeedFailed = 1 << 15,
  Error__FailedRemap = 1 << 16,
  Error__CannotExpandInPlace = 1 << 17,
  Error__InvalidAllocationSize = 1 << 18,
  Error__BufferNeedsResize = 1 << 19,
  Error__VirtMemOutOfMemory = 1 << 20,

  /// @brief Not enough space/cannot allocate memory (POSIX.1-2001).
  Error__OOM = 1 << 21,
  Error__ValTooLargeFoDataType = 1 << 22,
  Error__ResourceTempUnavail = 1 << 23,

  /// @brief for errors not covered by other Param* variants
  Error__ParamInvalid = 1 << 24,
  /// @brief function expected its first parameter to be non-null, but was null!
  Error__ParamInvalidMethod = 1 << 25,
  Error__ParamInvalidNull = 1 << 26,

  /// @brief paramter expected to be positive integer
  Error__ParamUnexpectedNegInt = 1 << 27,

  Error__ParamUnexpectedNegFloat = 1 << 28,
  Error__IndexOutOfRange = 1 << 29,
  Error__ParamUnexpectedNegOrZeroInt = 1 << 30,

  Error__ParamUnexpectedNegOrZeroFloat = 1 << 31,
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
  Error__FilePathTooLong = 1L << 46,
  Error__FileMapFailedToLoad = 1L << 47,

  // Error__ParamExpectedPosNonZeroInt = 1 << 30,

  // Error__ParamExpectedPosNonZeroFloat= 1 << 30,

  ERROR_COUNT = 48,
} HEDLEY_FLAGS NvError;

static constexpr const NvError OK = Error__Ok;

/// @remarks Currently an [NvError] can contain up to [ERROR_COUNT] number of errors simultaneously.
/// but - also currently - this function can only detect if given error value exactly matches each [NvError] variant
/// exactly, so it may report valid error values as invalid
const char* error_string(NvError err) CONST_FUNC RETURNS_NON_NULL;
