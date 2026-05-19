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
#include <string.h>
#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

struct sslice {
  const char* begin;
  i32 len;
};
typedef struct sslice sslice;


#define sslice_new(...) ((sslice){__VA_ARGS__})


#define sslice_static_new(static_str)                                      \
  /* Creates a new instance of [sslice] on the stack that points to string \
   literals, which reside in constant static readonly memory*/             \
  (sslice_new(.begin = (static_str), .len = (sizeof((static_str)) - 1))) /* - 1 so we dont include the null-terminating byte*/


#define sslice_empty() (sslice_new())  


PURE_FUNC
static inline bool sslice_is_empty(sslice self) {
  return self.begin == nullptr || self.len <= 0;
}

PURE_FUNC
static inline sslice sslice_from_str(const char* string) {
  const isize len = stringlen(string);
  return sslice_new(.begin = string, .len = len);
}


PURE_FUNC
/// creates a new [sslice] from given string that points to the range provided by @param (from) and @param (to)
/// such that the new slice points to string[from..to]
static inline sslice sslice_from_range(const char* string, isize from, isize to) {
  const isize slen = stringlen(string);
  const isize slice_len = to - from;
  if (slice_len > slen || slice_len < 0) {
    return sslice_empty();
  }
  const char* begin = &string[from];
  return sslice_new(.begin = begin, .len = slice_len);
}

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
