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

namespace nv::sslice {

#define StringSliceData \
  const char* begin;    \
  i64 len

struct Sslice {
  const char* begin;
  i64 len;
};

static constexpr Sslice create(const char* begin, i64 len) { return {.begin = begin, .len = len}; }
static consteval Sslice empty() { return {}; }

PURE_FUNC
static inline bool is_empty(Sslice self) { return self.begin == nullptr || self.len <= 0; }

PURE_FUNC
Sslice from_str(const char* string);

PURE_FUNC
/// creates a new [sslice] from given string that points to the range provided by @param (from) and @param (to)
/// such that the new slice points to string[from..to]
Sslice from_range(const char* string, isize from, isize to);

/// Forwards each given [sslice]'s begin pointer to [strncmp], taking the
/// minimum of each [sslice]'s length. for the count parameter of [strncmp]
///
/// Returns:
/// - A negative value if (left) appears before (right) in lexicographical order
/// - Zero if (left) and (right) compare equal, or if count is zero
/// - A positive value if (left) appears after (right) in lexicographical order
///
PURE_FUNC
i32 cmp(Sslice left, Sslice right);

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

PURE_FUNC
static inline bool sstring_cmp(StaticString lhs, StaticString rhs) {
  const auto left = sstring_slice(lhs);
  const auto right = sstring_slice(rhs);
  return cmp(left, right);
}

PURE_FUNC
static inline bool sstring_eq(StaticString lhs, StaticString rhs) {
  const auto left = sstring_slice(lhs);
  const auto right = sstring_slice(rhs);
  return sslice_eq(left, right);
}

#define SSPREAD(slice) ((slice).begin), ((i32)(slice).len)
#define RSSPREAD(slice) ((i32)(slice).len), ((slice).begin)
}  // namespace nv::sslice
