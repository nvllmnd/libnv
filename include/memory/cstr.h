#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "attributes.h"
#include "core_types.h"
#include "intdefs.h"

static constexpr i32 SMALL_BUF_SIZE = 14;

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
static inline isize str_len(const char* string, isize max_len) {
  if (is_null(string)) {
    return 0;
  }

  isize len = 0;
  while ((string[len] != 0) && (len <= max_len)) {
    len++;
  }
  return len;
}

/// Same as [stringlen], forwards @param (string) to [stringlen], passing
/// [STRLEN_UPPER_BOUND]([INT32_MAX -1]) as the second parameter
PURE_FUNC
static inline isize stringlen(const char* string) { return str_len(string, STRLEN_UPPER_BOUND); }

/// Type alias to make it more clear that
/// this is a pointer to a string that has an [i32] prefix length
/// which you can get by subtracting sizeof([i32]) from this [char]*
/// (or by casting this to a i32* and then indexing that i32* with -1,
/// i.e.:
///     ((i32*)some_prefix_str)[-1]; // gives you the length of this prefix string as an i32
///
/// )
typedef char* prefix_str;

/// Type alias to make it more clear that
/// this is a pointer to a string that has an [i32] prefix length
/// which you can get by subtracting sizeof([i32]) from this [char]*
///
/// [const] version of [prefix_str]
typedef const char* const_prefix_str;

typedef enum CStrType {
  CStrType__Small,
  CStrType__Heap,
} CStrType;

struct cstr {
  union {
    struct CStrSmall {
      u8 len;
      char mem[SMALL_BUF_SIZE + 1];  // +1 for null terminator
    } buf;
    prefix_str heap;
  };

  // CStrType type;
  bool is_large;
};

typedef struct cstr cstr;
typedef struct CStrSmall CStrSmall;

typedef enum CStrError {
  /// Ok! No Errors!
  CStrError__Ok = 0,

  /// String is too big to create a small [cstr]
  /// Returned from [cstr_init_small] when passed in null-terminated string is
  /// larger than [SMALL_BUF_SIZE]
  CStrError__StringTooBig,
  /// Error was caused in Underlying allocator and thus
  /// we are unable to create a new [cstr]
  CStrError__AllocatorError,
} CStrError;

CONST_FUNC
static inline cstr cstr_empty(void) { return (cstr){}; }

PURE_FUNC
static inline bool cstr_is_small(const cstr* self) { return !self->is_large; }

PURE_FUNC
static inline bool cstr_is_heap_allocated(const cstr* self) { return self->is_large; }

/// Gets the [cstr] inner string's length
PURE_FUNC
static inline usize cstr_len(cstr self) {
  if (self.is_large) {
    const i32* str_begin = pcast(i32, self.heap);
    const usize len = cast(usize, str_begin[-1]);
    return len;
  }

  const usize len = cast(usize, self.buf.len);
  return len;
}

/// Returns a [const char*] pointer to beginning of inner string
PURE_FUNC
METHOD
static inline const char* cstr_as_ptr(const cstr* self) {
  if (self->is_large) {
    return self->heap;
  }

  return self->buf.mem;
}

/// Creates a new [cstr] from given null-terminated c-style string.
[[nodiscard(
    "Must use returned cstr! While this type may not always allocate, "
    "its still best practice to treat it as if it did allocate")]]
cstr cstr_new(const char* string);

/// Creates a new [cstr] from given string, up to size @param(len)
[[nodiscard(
    "Must use returned cstr! While this type may not always allocate, "
    "its still best practice to treat it as if it did allocate")]]
cstr cstr_from_slice(const char* string, usize len);

/// Creates a cstr on the heap wtih given capacity
/// Keep in mind the cstr returned by this function is completely zeroed out,
/// but contains a prefix length of given capacity.
/// So inspecting this resulting cstr, you will observe that it is reporting it
/// has a length, despite being completely zeroed
[[nodiscard(
    "Must use returned cstr! This function always allocates when "
    "creating new cstr's")]]
cstr cstr_large_with_capacity(usize capacity);

/// Tries to init a zeroed/uninit [cstr] without allocating on the heap at all.
/// Given null-terminated c-style string must be of length less than
/// [SMALL_BUF_SIZE], otherwise this function returns a [CStrError] and leaves
/// given [cstr] in an invalid/zeroed state
[[nodiscard(
    "Caller must handle returned CStrError to ensure cstr was properly "
    "initialized!")]]
CStrError cstr_init_small(cstr* self, const char* string);

/// Same as [cstr_init_small], but only
/// copies up to the first [SMALL_BUF_SIZE] of given string if
/// it is longer than that instead of returning an error
///
/// This function does not allocate, and therefore caller is
/// not required to call [cstr_free] when done with returned [cstr].
/// It is still safe to pass a [cstr] that has not allocated on the heap to
/// [cstr_free], as that function does nothing in the case that [cstr_is_small]
/// returns false
cstr cstr_small_new(const char* string);

/// Shrinks [cstr] to length of given @param(smaller_size) in bytes.
/// if [cstr] has a length already smaller than given @param(smaller_size),
/// then this function returns false and bails out early, changing nothing.
///
/// This function will not, for instance, move a heap allocated string of size
/// larger than [SMALL_BUF_SIZE], onto the stack, even if @param(smaller_size) <
/// [SMALL_BUF_SIZE]. Allocations do not move from where they currently are as
/// of invoking this function. This is a checked operation around a simple
/// assignment of this cstr's current length field to @param(smaller_size)
///
/// This function will return true upon successful shrinking to
/// @param(smaller_size), otherwise false
///
bool cstr_shrink_to(cstr* self, usize smaller_size);

/// Free a created [cstr].
///
/// Only makes a call to free if given cstr
/// is_large flag is set and owns a string allocated on the heap
///
/// This function does nothing if [cstr] is small and
/// less than size [SMALL_BUF_SIZE] ([cstr_is_small] returns true)
///
void cstr_free(cstr* self);

/// Concatenates 2 [cstr]s into a new cstr by copying their contents into
/// the resulting [cstr]
///
/// For a version that frees the 2 strings to concatenating after
/// its done using them, (i.e., moving 2 concatenated strings into a single new
/// one) see [cstr_move_concat]
///
[[nodiscard(
    "Caller must not ignore returned cstr as doing so will most likely "
    "leak memory!")]]
cstr cstr_concat(const cstr* left, const cstr* right);

/// same as [cstr_concat], but frees the concatenated [cstr]s after copying
/// their data to the new [cstr] that is returned contains a string that is
/// (left + right)
///
/// For a version of the function that does not clean up given [cstr]s,
/// see [cstr_concat]

[[nodiscard(
    "Caller must not ignore returned cstr as doing so will most likely "
    "leak memory!")]]
cstr cstr_move_concat(cstr* left, cstr* right);

/// Appends a given string up to @param (len) characters by copying its contents
/// into @param (self) [cstr]
void cstr_append_string(cstr* self, const char* s, usize len);

/// Helper function for [cstr_token] macro. Not for any other use otherwise
/// PLS DONT USE KTHNX
static inline cstr priv_cstr_token_impl(const char* s, usize len) {
  cstr self = {};
  strncpy(self.buf.mem, s, len);
  self.buf.len = cast(u8, len);
  return self;
}

#define cstr_token(string_literal)                                            \
  /* Creats a new cstr allocated on the stack. Contains a static assurtion to \
   * check that*/                                                             \
  /* the provided string_literal argument is of length < [SMALL_BUF_SIZE]*/   \
  ({                                                                          \
    constexpr const usize _LEN = sizeof((string_literal));                    \
    static_assert(_LEN < SMALL_BUF_SIZE);                                     \
    priv_cstr_token_impl(string_literal, _LEN);                               \
  })

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
static inline i32 sslice_cmp(sslice left, sslice right) {

  if (left.begin == nullptr) { return -1; }
  if (right.begin == nullptr) { return 1; }
  return strncmp(left.begin, right.begin, min(left.len, right.len));
}

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
static inline bool sslice_eq(sslice left, sslice right) {
  if (left.len == right.len) {
    return sslice_cmp(left, right) == 0;
  }
  return false;
}

/// Attempts to create a new [sslice] from a sub range of
/// this [cstr]'s inner string.
/// Returns a null/empty [sslice] if @param (from) or @param (to)
/// are invalid/negative values, or values that are out of range for this [cstr]
sslice cstr_slice(const cstr* self, isize from, isize to);

/// Returns a new [sslice] pointing to the inner string of
/// self [cstr]
/// 
/// WARN: The pointer in the [sslice] returned by this function is
/// only valid for as long as the given pointer is in scope! (careful for small, stack [cstrs])
///
/// 
/// This function requires taking a pointer to [cstr], because
/// if this function took [cstr] by copy, this function could possible return
/// a pointer to stack memory that is no longer valid after this fuction returns
/// (cstr is small, gets copied onto this functions new stack frame, the this function returns a pointer to that
/// buffer on the stack, buffer goes out of scope and pointer is immediately invalid, UB follows!)
static inline sslice cstr_as_slice(const cstr* self) {
  const isize len = cstr_len(*self);
  const char* begin = cstr_as_ptr(self);

  return sslice_new(.begin = begin, .len = len);
}

/// A [cstr] variant of [sslice_cmp].
/// Takes the [sslice] of both given [ctr]s and forwards call to [sslice_cmp]
///
/// For more details, see: [sslice_cmp]
PURE_FUNC
static inline i32 cstr_cmp(cstr left, cstr right) {
  const sslice l = cstr_as_slice(&left);
  const sslice r = cstr_as_slice(&right);
  return sslice_cmp(l, r);
}

/// A [cstr] variant of [sslice_eq].
/// Takes the [sslice] of both given [ctr]s and forwards call to [sslice_eq].
///
/// For more details, see: [sslice_eq]
PURE_FUNC
static inline bool cstr_eq(const cstr* left, const cstr* right) {
  const sslice l = cstr_as_slice(left);
  const sslice r = cstr_as_slice(right);
  return sslice_eq(l, r);
}


