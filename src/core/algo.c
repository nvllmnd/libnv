// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/algo.h"

#include <assert.h>
#include <string.h>

#include "nv/core/constants.h"
#include "nv/core/core_types.h"
#include "nv/core/log.h"
#include "nv/core/spad.h"
#include "nv/core/sslice.h"
#include "nv/core/stb_sprintf.h"
#include "nv/iter/iterators.h"
#include "nv/memory/error.h"

void* ptr_expect_(const void* ptr, const char* msg) {
  if UNLIKELY (nullptr == ptr) {
    log_fatal("%s", msg);
  }
  // NOTE: We dont mutate this pointer at all, so its safe to cast this back to non-const, since
  // we cast it back to exactly the same type as the pointer was before being passed to this function though the
  // implementation macro
  return pcast(void, ptr);
}

bool stringeq(const char* left, const char* right) {
  if (left == right) {
    return true;
  }

  const i32 llen = stringlen(left);
  const i32 rlen = stringlen(right);

  if (llen == rlen) {
    return strncmp(left, right, llen) == 0;
  }

  return false;
}

static constexpr const u32 PRIME32 = 0x010001930;
static constexpr const u32 OFFSET32 = 0x811c9dc5;
static constexpr const u64 PRIME64 = 0x00000100000001b3;
static constexpr const u64 OFFSET64 = 0xcbf29ce484222325;

u32 fnv_hash32(const char* string, isize len) {
  assert(string);
  assert(len > 0);

  u32 hash = OFFSET32;
  for (i32 i = 0; i < len; i++) {
    const u32 c = string[i];
    hash = (hash ^ c) * PRIME32;
  }
  return hash;
}

u64 fnv_hash64(const char* string, isize len) {
  assert(string);
  assert(len > 0);

  u64 hash = OFFSET64;

  for (i32 i = 0; i < len; i++) {
    const u64 c = string[i];
    hash = (hash ^ c) * PRIME64;
  }
  return hash;
}

isize str_len(const char* string, isize max_len) {
  if UNLIKELY (is_null(string)) {
    return 0;
  }

  isize len = 0;
  while ((string[len] != 0) && (len <= max_len)) {
    len++;
  }
  return len;
}

bool sslice_eq(sslice left, sslice right) {
  if (left.len == right.len) {
    return sslice_cmp(left, right) == 0;
  }
  return false;
}

i32 sslice_cmp(sslice left, sslice right) {
  if (left.begin == nullptr) {
    return -1;
  }
  if (right.begin == nullptr) {
    return 1;
  }
  return strncmp(left.begin, right.begin, min(left.len, right.len));
}

void* ptr_nonnull_(const void* ptr) {
  return ptr_expect_(ptr, " Expected given pointer to be non-null, but was nullptr! Aborting program!");
}

static inline i64 find_term(const char* str) {
  if UNLIKELY (is_null(str)) {
    LOG_FATAL("Cannot find null terminal with nullptr!");
  }

  for (i64 i = 0; i < STRLEN_UPPER_BOUND; i++) {
    if (str[i] == 0) {
      return i;
    }
  }

  static constexpr const i32 SAMPLE_SIZE = 128;
  LOG_FATAL(
      "string beginning with the characters: %.*s, does not have a terminal null character! (or has length/size larger "
      "than: "
      "%li, which is libnv's upper limit for finding the length of C-style null-terminated strings)",
      SAMPLE_SIZE, str, STRLEN_UPPER_BOUND);
}

isize ptr_align_offset(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const u64ptr mask = align - 1;
  return cast(isize, cast(u64ptr, ptr) & mask);
}

bool ptr_is_aligned(const void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));

  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

void* ptr_alignup(void* ptr, isize align) {
  assert(ptr);
  assert(IS_POWER_OF_2(align));
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  LOG_DBG("Pointer: %p not currently aligned! aligning to: %p", ptr, (void*)aligned);

  return pcast(void, aligned);
}

u8* ptr_alignto(u8* ptr, u8* end, Layout layout) {
  assert(ptr);
  assert(end);
  assert(end >= ptr);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  u8* const top = ptr_alignup(ptr, layout.align);

  if (top + layout.size >= end) {
    return nullptr;
  }

  return top;
}

u8* ptr_alignin(u8* ptr, i32* space, Layout layout) {
  assert(ptr);
  assert(space);
  assert(IS_POWER_OF_2(layout.align));
  assert(layout.size > 0);

  const i32 avail = *space;

  if (avail < layout.size) {
    return nullptr;
  }

  u8* const end = ptr + *space;

  u8* const aligned = ptr_alignto(ptr, end, layout);

  if (is_null(aligned)) {
    return nullptr;
  }

  const i32 delta = aligned - ptr;
  *space -= delta;

  return aligned;
}

i32 fstring_length(const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const i32 len = vfstring_length(fmt, args);

  va_end(args);

  return len;
}

i32 vfstring_length(const char* fmt, va_list args) {
  va_list cpy = {};
  va_copy(cpy, args);

  const i32 len = stbsp_vsnprintf(nullptr, 0, fmt, cpy);

  va_end(cpy);
  return len;
}

const char* error_string(NvError err) {
  if (err == 0) {
    return STRINGIFY(Error__Ok) " :: Ok! no error.";
  } else if (err == Error__UnknownError) {
    return STRINGIFY(Error__UnknownError);
  }
  // TODO: Write a print_error version of this function. OR change this functions parameters to take a string
  // buffer to write into, then i can write up to caller defined limit (-1 for null terminal) and return

  switch (err) {
    case Error__Ok: {
    } break;
    case Error__FailedMemMap: {
      return STRINGIFY(Error__Ok) " :: Ok! no error.";
    } break;
    case Error__FailedMemUnmap: {
      return STRINGIFY(Error__UnknownError);
    } break;
    case Error__BufferNeedsResize: {
      return STRINGIFY(Error__BufferNeedsResize) " :: Buff must be resized in order to append a given layout!";
    } break;
    case Error__VirtMemOutOfMemory: {
      return STRINGIFY(Error__VirtMeOutOfMemory) " :: Virtual Memory owned by VirtMem does not have enough memory for a given MemLayout!";
    } break;
    case Error__OOM: {
      return STRINGIFY(Error__OOM) " :: General/Unspecified Out of Memory Error.";
    } break;
    case Error__ValTooLargeFoDataType: {
      return STRINGIFY(Error__ValTooLargeForDataType) " :: Alias for ERRNO: EOVERFLOW";
    } break;
    case Error__ResourceTempUnavail: {
      return STRINGIFY(Error__ResourceTempUnavail) " :: Alias for ERRNO: EAGAIN";
    } break;
    case Error__VMapCannotBeResized:
      return STRINGIFY(Error__VMapCannotBeResized);
    case Error__VMapInvalidRemapFlags:
      return STRINGIFY(Error__VMapInvalidRemapFlags);
    case Error__VMapInvalidMapFlags:
      return STRINGIFY(Error__VMapInvalidMapFlags);
    case Error__FailedToPreloadOOM:
      return STRINGIFY(mError__FailedToPreloadOOM);
    case Error__VMapCannotLockToRAM:
      return STRINGIFY(Error__VMapCannotLockToRAM);
    case Error__VMapCannotShrink:
      return STRINGIFY(Error__VMapCannotShrink);
    case Error__VMapCannotGrow:
      return STRINGIFY(Error__VMapCannotGrow);
    case Error__VMapCannotBeMoved:
      return STRINGIFY(Error__VMapCannotBeMoved);
    case Error__VMapCannotExpandInPlace:
      return STRINGIFY(Error__VMapCannotExpandInPlace);
    case Error__NotEnoughPhysicalRAMAavailable:
      return STRINGIFY(Error__NotEnoughPhysicalRAMAavailable);
    case Error__VMemLimitReached:
      return STRINGIFY(Error__VMemLimitReached);
      break;
    case Error__CannotUnlockRAM:
      return STRINGIFY(Error__CannotUnlockRAM);
    case Error__MAdviseWillNeedFailed:
      return STRINGIFY(Error__MAdviseWillNeedFailed);
      break;
    case Error__FailedRemap:
      return STRINGIFY(Error__FailedRemap);
    case Error__CannotExpandInPlace:
      return STRINGIFY(Error__CannotExpandInPlace);
      break;
    case Error__InvalidAllocationSize:
      return STRINGIFY(Error__InvalidAllocationSize) " :: Size null, negative, or larger than INT64_MAX!";
      break;
    case Error__ParamInvalid:
      return STRINGIFY(Error__ParamInvalid);
    case Error__ParamInvalidMethod:
      return STRINGIFY(ParamInvalidMethod);
    case Error__ParamInvalidNull:
      return STRINGIFY(Error__ParamInvalidNull);
    case Error__ParamUnexpectedNegInt:
      return STRINGIFY(Error__ParamUnexpectedNegInt);
    case Error__ParamUnexpectedNegFloat:
      return STRINGIFY(Error__ParamUnexpectedNegFloat);
    case Error__IndexOutOfRange:
      return STRINGIFY(Error__IndexOutOfRange);
    case Error__ParamUnexpectedNegOrZeroInt:
      return STRINGIFY(Error__ParamUnexpectedNegOrZeroInt);
    default:
      return "Invalid NvError Value!";
  }

  return "NvError is Invalid, or may contain more than 1 NvError value. function error_string cannot currently detect "
         "such values, but will in the soon future (pending an API change)! Its probably best to not use this function "
         "in the first place until multi-value error are supported!";
}

sslice sslice_from_str(const char* string) {
  const isize len = stringlen(string);
  return sslice_new(.begin = string, .len = len);
}

sslice sslice_from_range(const char* string, const isize from, const isize to) {
  const isize slen = stringlen(string);
  const isize slice_len = to - from;
  if (slice_len > slen || slice_len < 0) {
    return sslice_empty();
  }
  const char* begin = &string[from];
  return sslice_new(.begin = begin, .len = slice_len);
}

static inline i64 stringcat_impl(char* dest, i64 dest_count, i64 dest_size, const char* src, i64 srclen) {
  const i64 size = dest_count + srclen;
  assert(size < dest_size);

  if (dest[dest_count] != 0) {
    const i64 i = find_term(dest);
    LOG_FATAL("Null term for string: %.*s is at index %li, not index: %li!", (i32)i, dest, i, dest_count);
  }

  char* begin = &dest[dest_count];

  strncpy(begin, src, srclen);
  begin[srclen] = 0;

  return size;
}

NvError try_stringcat(char* dest, const i64 dest_count, const i64 dest_size, const char* src, const i64 srclen,
                      i64* out_new_count) {
  NvError err = OK;

  if UNLIKELY (is_null(dest)) {
    bitset(err, Error__ParamInvalidMethod);
  }

  if UNLIKELY (srclen >= dest_size || (dest_count + srclen) >= srclen) {
    LOG_ERROR(
        "destination string: %.*s with capacity: %li, and size: %li, is not properly sized to concatenate string: %.*s",
        (i32)dest_count, dest, dest_size, dest_count, (i32)srclen, src);
    bitset(err, Error__IndexOutOfRange);
    return err;
  }

  if UNLIKELY (is_not_null(dest) && dest[dest_count] != 0) {
    LOG_ERROR(
        "string: %.*s does not end with a null character at caller provided index: %li. make sure the dest_count "
        "parameter you pass to stringcat can be used to index dest string's null character terminal",
        (i32)dest_count, dest, dest_count);

    bitset(err, Error__IndexOutOfRange);
    return err;
  }

  if UNLIKELY (is_null(src)) {
    bitset(err, Error__ParamInvalidNull);
  }
  if UNLIKELY (dest_count <= 0 || srclen <= 0) {
    bitset(err, Error__ParamUnexpectedNegOrZeroInt);
  }

  if UNLIKELY (err != OK) {
    LOG_ERROR("String Concat Error: %s", error_string(err));
    return err;
  }

  assert(dest[dest_count] == 0);

  const i64 size = stringcat_impl(dest, dest_count, dest_size, src, srclen);

  if (is_not_null(out_new_count)) {
    *out_new_count = size;
  }

  return OK;
}

i64 stringcat(char* dest, i64 dest_count, i64 dest_size, const char* src, i64 srclen) {
  assert(dest);
  assert(src);
  const i64 count = min(dest_count + srclen, dest_size);

  if UNLIKELY (count > dest_size) {
    LOG_ERROR(
        "Cannot concat source string: %.*s of length: %li, into destination string: %.*s with capacity: %li bytes!",
        (i32)dest_count, dest, dest_count, (i32)srclen, src, dest_size);
    return -1;
  }

  return stringcat_impl(dest, dest_count, dest_size, src, srclen);
}

sslice vfconcat(char* const dest, i64 dest_len, const i64 dest_cap, const char* const fmt, va_list args) {
  assert(dest);
  assert(dest_len >= 0);
  assert(dest_cap > 0);
  assert(fmt);

  if (dest[dest_len] != 0) {
    const i64 i = find_term(dest);

    LOG_ERROR("vfconcat dest string: %.*s null character does not exist at index: %li, but at index: %li instead!",
              (i32)i, dest, dest_len, i);

    dest_len = i;
  }

  char* buf = &dest[dest_len];
  assert(*buf == 0);

  const i64 size = dest_cap - dest_len;
  assert(size >= 0);

  const i64 n = stbsp_vsnprintf(buf, size, fmt, args);

  if UNLIKELY (n <= 0) {
    LOG_FATAL("Error occurred while formatting string: %s before concatenating to destination string: %.*s", fmt,
              (i32)dest_len, dest);
  }

  return sslice_new(.begin = dest, .len = dest_len + n);
}

sslice fconcat(char* dest, i64 dest_size, const i64 dest_cap, const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const sslice sl = vfconcat(dest, dest_size, dest_cap, fmt, args);

  va_end(args);
  return sl;
}
