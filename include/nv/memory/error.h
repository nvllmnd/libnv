// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <errno.h>

#include "nv/core/attributes.h"
#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"

typedef enum nv_nodiscard_msg("Ignoring functions returning NvError type could lead to segfaults!") NvError : uerror {

  /// Ok, No Error!
  Error__Ok = 0,

  /// mmap errors...
  Error__FailedMemMap = 1 << 0,
  Error__VMapCannotBeResized = 1 << 1,
  Error__VMapInvalidRemapFlags = 1 << 2,
  Error__VMapInvalidMapFlags = 1 << 3,
  Error__FailedToPreloadOOM = 1 << 4,
  Error__VMapCannotLockToRAM = 1 << 5,
  Error__CannotUnlockRAM = 1 << 6,
  Error__VMapCannotShrink = 1 << 7,
  Error__VMapCannotGrow = 1 << 8,
  Error__VMapCannotBeMoved = 1 << 9,
  Error__VMapCannotExpandInPlace = 1 << 10,
  Error__FailedMemUnmap = 1 << 11,
  Error__NotEnoughPhysicalRAMAavailable = 1 << 12,
  Error__VMemLimitReached = 1 << 13,
  Error__MAdviseWillNeedFailed = 1 << 14,
  Error__FailedRemap = 1 << 15,
  Error__CannotExpandInPlace = 1 << 16,
  Error__InvalidAllocationSize = 1 << 17,
  Error__BufferNeedsResize = 1 << 18,
  Error__VirtMemOutOfMemory = 1 << 19,

  /// @brief Not enough space/cannot allocate memory (POSIX.1-2001).
  Error__OOM = 1 << 20,
  Error__ValTooLargeFoDataType = 1 << 21,
  Error__ResourceTempUnavail = 1 << 22,

  /// @brief for errors not covered by other Param* variants
  Error__ParamInvalid = 1 << 23,
  /// @brief function expected its first parameter to be non-null, but was null!
  Error__ParamInvalidMethod = 1 << 24,
  Error__ParamInvalidNull = 1 << 25,

  /// @brief paramter expected to be positive integer 
  Error__ParamUnexpectedNegInt = 1 << 26,

  Error__ParamUnexpectedNegFloat = 1 << 27,
  Error__IndexOutOfRange = 1 << 28,
  Error__ParamUnexpectedNegOrZeroInt = 1 << 29,

  Error__ParamUnexpectedNegOrZeroFloat = 1 << 29,
  Error__VMemFailedToLockRangeToRAM = 1 << 30,
  Error__VMemFailedToUnlockRangeFromRAM = 1 << 31,
  Error__PtrNotOwnedByVMem = 1L << 32,
  Error__VMemFailedToPrefaultRange = 1L << 33,

  // Error__ParamExpectedPosNonZeroInt = 1 << 30,

  // Error__ParamExpectedPosNonZeroFloat= 1 << 30,

  Error__UnknownError = UINT64_MAX - 20,

  ERROR_COUNT = 34,
} HEDLEY_FLAGS NvError;

static constexpr const NvError OK = Error__Ok;

const char* error_string(NvError err) CONST_FUNC RETURNS_NON_NULL;
