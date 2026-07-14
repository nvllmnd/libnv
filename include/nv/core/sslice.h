// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/// A String slice, consisting of a pointer to the beginning of
/// the slice and a length
///
/// Its possible to create [sslice]s that point to static constant strings
/// in readonly memory; see: [sslice_static_new], as it is not currently
/// possible to create [cstr] instances that point to constant static strings if
/// those strings are greater than [SMALL_BUF_SIZE] in length
///
/// [sslice_static_new] aslo does some additional (rudimentary)
/// static validation (static_assert) to ensure given string
/// is an actual string literal.
///
/// These slices are ment to be immutable, as in it is not typical to
/// mutate strings through a [sslice]. As such these are treated like views
///
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

#define SSPREAD(slice) ((slice).begin), ((slice).len)
#define RSSPREAD(slice) ((slice).len), ((slice).begin)

// C++ Support.
//
#ifdef __cplusplus

struct sslice {
  const char* begin;
  i32 len;
};

#define sslice_new(...)             \
  ({                                \
    const sslice _res{__VA_ARGS__}; \
    _res;                           \
  })

#endif

#ifndef __cplusplus

#define StringSliceData \
  const char* begin;    \
  i32 len

struct sslice {
  StringSliceData;
};
typedef struct sslice sslice;

#define sslice_new(...) ((sslice){__VA_ARGS__})

#define sslice_static_new(static_str)                                      \
  /* Creates a new instance of [sslice] on the stack that points to string \
   literals, which reside in constant static readonly memory*/             \
  (sslice_new(.begin = (static_str),                                       \
              .len = (sizeof((static_str)) - 1))) /* - 1 so we dont include the null-terminating byte*/

#define sslice_empty() (sslice_new())

#define empty_string() static_string("")
#define sstring_new static_string

#endif

BEGIN_C_DECLS

PURE_FUNC
static inline bool sslice_is_empty(sslice self) { return self.begin == nullptr || self.len <= 0; }

PURE_FUNC
sslice sslice_from_str(const char* string);

PURE_FUNC
/// creates a new [sslice] from given string that points to the range provided by @param (from) and @param (to)
/// such that the new slice points to string[from..to]
sslice sslice_from_range(const char* string, isize from, isize to);

/// Forwards each given [sslice]'s begin pointer to [strncmp], taking the
/// minimum of each [sslice]'s length. for the count parameter of [strncmp]
///
/// Returns:
/// - A negative value if (left) appears before (right) in lexicographical order
/// - Zero if (left) and (right) compare equal, or if count is zero
/// - A positive value if (left) appears after (right) in lexicographical order
///
PURE_FUNC
i32 sslice_cmp(sslice left, sslice right);

/// Checks if 2 [sslice]s are exaclty equal.
/// This means that if both [sslice]s have dissimilar lengths, this function will return false.
/// Both [sslice]s must be the same length, and have the exact same characters in the exact same order.
/// i.e.:
///     sslice_eq("asdf", "asdf1") == false;
///     sslice_eq("ayo", "ayo") == true;
///
/// For a version of this function that compares 2 [sslice]s lexicographically,
/// see: [sslice_cmp]
PURE_FUNC
bool sslice_eq(sslice left, sslice right);

#ifndef __cplusplus

struct StaticString {
  StringSliceData;
};
typedef struct StaticString StaticString;

#define static_string(_ss)                                                                                \
  ({                                                                                                      \
    static_assert(HEDLEY_IS_CONSTANT((_ss)), "Static Strings can only be created with string literals!"); \
    (StaticString){.begin = (_ss), .len = (sizeof((_ss)) - 1)};                                           \
  })

PURE_FUNC
static inline sslice sstring_slice(StaticString self) { return sslice_new(.begin = self.begin, .len = self.len); }
PURE_FUNC
static inline bool sstring_cmp(StaticString lhs, StaticString rhs) {
  const auto left = sstring_slice(lhs);
  const auto right = sstring_slice(rhs);
  return sslice_cmp(left, right);
}

PURE_FUNC
static inline bool sstring_eq(StaticString lhs, StaticString rhs) {
  const auto left = sstring_slice(lhs);
  const auto right = sstring_slice(rhs);
  return sslice_eq(left, right);
}

#endif  // #ifndef __cplusplus

END_C_DECLS
