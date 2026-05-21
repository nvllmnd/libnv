#pragma once

#include <stddef.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/memory/layout.h"

#define IS_POWER_OF_2(n) ((n & (n - 1)) == 0)

PURE_FUNC
PARAMS_NONNULL(1)
u32 fnv_hash32(const char* string, isize len) WHERE(len > 0);

PURE_FUNC
PARAMS_NONNULL(1)
u64 fnv_hash64(const char* string, isize len) WHERE(len > 0);


CONST_FUNC
static inline bool is_power_of_2(isize n) { return IS_POWER_OF_2(n); }


// CLANG_NON_NULL_BEGIN

// struct NonNull {
//   void* CLANG_NON_NULL ptr;
// };
// typedef struct NonNull NonNull;

/// Takes any pointer and if it is null, aborts execution with [log_fatal]. otherwise returns
/// the same pointer unchanged/un-mutated.
/// This is to assert to the compiler that a poitner is not null, as
/// this function is marked with the __returns_nonnull__ compiler attribute
RETURNS_NON_NULL
PURE_FUNC
void* ptr_nonnull_(const void* ptr) WHERE(ptr_nonnull_(ptr) == ptr);

// CLANG_NON_NULL_END

#define ptr_nonnull(_ptr) ((typeof_ptr(_ptr))(ptr_nonnull_((const void*)(_ptr))))

#if !defined(punwrap) && !defined(LIBNV_NO_USE_SHORT_NAMES)
#define punwrap ptr_nonnull
#endif

/// Same as [ptr_nonnull], but fails with a user provided, message.
/// NOTE: Because this function is inteded to be used in hot paths, the user
/// must build error messages themselves. if this function were a printf-style function, it
/// would at the very worst, pass a bunch of parameters onto the stack that will never be used most the time, callers are also
/// encouraged to keep messages light (no formatting, just as simple string literal)
RETURNS_NON_NULL
PURE_FUNC
void* ptr_expect_(const void* ptr, const char* msg);

#if !defined(pexpect) && (!defined(LIBNV_NO_USE_SHORT_NAMES) || LIBNV_NO_USE_SHORT_NAMES == 0)
#define pexpect(_p, _msg) ((typeof_ptr(_p))(ptr_expect_((const void*)(_p), (_msg))))
#endif


#ifndef STRLEN_UPPER_BOUND

#define STRLEN_UPPER_BOUND                                                    \
  /* Upper bound used by [stringlen] as the max_len parameter to [str_len] */ \
  /* NOTE: I decided to make this a macro so that it can be configurable to   \
   * each build (-D compiler flag)*/                                          \
  (INT32_MAX - 1)

#endif  // STRLEN_UPPER_BOUND

/// A safe version of the standard lib: [strlen], which technically may never
/// return if the passed in string never contains a null character to signal
/// that this is the end of the string and return the length.
///
/// This function takes a (max_len) parameter, which, if after
/// iterating through given (string) up to (max_len) characters,
/// and a terminal null character has still not been found,
/// then this funciton will return (max_len).
///
/// As such, consider if:
///
/// isize result = str_len(some_long_string, 255);
/// if (result == 255) {
///  /* failure! especially if the 256th character (in this example) is not a
///  terminal null character! */
/// }
///
/// For a version that does not require a @param (max_len) and passes []
///
///
PURE_FUNC
isize str_len(const char* string, isize max_len);

/// Same as [stringlen], forwards @param (string) to [stringlen], passing
/// [STRLEN_UPPER_BOUND]([INT32_MAX -1]) as the second parameter
PURE_FUNC
static inline isize stringlen(const char* string) { return str_len(string, STRLEN_UPPER_BOUND); }

PURE_FUNC
bool stringeq(const char* left, const char* right);


PARAMS_NONNULL(1)
isize ptr_align_offset(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

/// Checks if a given pointer is aligned to given alignment.
/// @param (align) MUST BE A POWER OF 2. If it is not this funciton returns
/// false
PURE_FUNC
PARAMS_NONNULL(1)
bool ptr_is_aligned(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

/// Aligns pointer up to given alignment, or returns the same pointer if it
/// already is aligned
/// @param (align) MUST BE A POWER OF 2.  If it is not, then this function
/// returns the exact same pointer, doing no calulations and possiby causing
/// confusion if given pointer is misaligned
PARAMS_NONNULL(1)
RETURNS_NON_NULL
PURE_FUNC
void* ptr_alignup(void* ptr, isize align) WHERE(IS_POWER_OF_2(align));

PARAMS_NONNULL(1, 2)
PURE_FUNC
u8* ptr_alignto(u8* ptr, u8* end, MemLayout layout) WHERE(IS_POWER_OF_2(layout.align) && end >= ptr);

/// behaves similarly to C++'s std::align
PARAMS_NONNULL(1, 2)
u8* ptr_alignin(u8* ptr, i32* space, MemLayout layout);

PARAMS_NONNULL(1)
static inline void* move(void** from) {
  void* tmp = *from;
  *from = nullptr;
  return tmp;
}
#define move(from) (move((void**)&from))

PARAMS_NONNULL(1, 2)
static inline void* move_into(void** from, void** to) {
  *to = move(*from);
  return *to;
}
#define move_into(from, to) (move_into((void**)&from, (void**)&to))

PARAMS_NONNULL(1, 2)
static inline void* move_exchange(void** obj, void** new_value) {
  void* tmp = *obj;
  *obj = *new_value;
  return tmp;
}
#define move_exchange(from, to) (move_exchange((void**)&from, (void**)&to))



