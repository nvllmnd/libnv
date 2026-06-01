#pragma once



#include <stdarg.h>
#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"

/// @brief A StringPad used for building Strings dynamically
struct StringPad {
  /// @brief Borrowed [VirtMem].
  /// @remarks Keep in mind that if this VirtMem is used at all while this StringPad is used,
  /// the resulting strings will be corrupted
  struct VirtMem* vm;
  /// @brief separator for appended strings
  char delim;
  /// @brief size of current string in bytes
  i32 size;
  /// @brief start of stringpad in VirtMem
  const char* begin;
  /// @brief end of stringpad in VirtMem
  const char* end;
};
alias(StringPad);

PARAMS_NONNULL(1)
StringPad spad_new(struct VirtMem* vm, char delim);
/// @brief copies buff_len bytes from SPad's inner string buffer into given memory buffer (buff_out)
/// @param (char* buff_out) - writes `buff_len` - 1 characters into memory at this address. appends null-character to
/// end of `buff_out`. Must not be null and at least `buff_len` bytes long.
/// @param (i32 buff_len) - number of bytes to write form Spad to `buff_out`
/// @returns Number of bytes written to `buff_out`. May be less than `buff_len` if Spad's inner string buffer is of
/// shorter length than `buff_len`
PARAMS_NONNULL(1, 2)
i32 spad_clone_into(StringPad* self, char* buff_out, i32 buff_len);

/// @brief duplicates currently built string using given allocator
/// @param(Allocator alloc) - Used to duplicate Spad's inner string buffer
/// @returns null-terminated string slice pointing to where the cloned string lives in given allocator. slice length
/// does not include null terminating character
METHOD
sslice spad_clone_string(StringPad* self, Allocator alloc);

METHOD
sslice spad_nappend(StringPad* self, const char* s, i32 len);
METHOD
sslice spad_append(StringPad* self, const char* s);

/// @brief Formats given format string and appends it to the back of the StringPad's inner buffer. Takes a format string
/// literal and printf-style variadic format value args
///
/// @param (const char* fmt) - printf-style format string literal
/// @param (char delim) - character delimiter to write to the end of appended formatted string.
///   Passing null character does NOT delimit strings with null character, instead if null character is passed for
///   delim, then this function acts as a way to append strings together, while always keeping a null character at the
///   end, otherwise strings are delimited with given delimiter character,
///
///  - ex:
///     spad_fappend_delim("%s", ',', "ayo"); // "ayo,"
///     spad_fappend_delim("%s", '\0', "yoo"); // "ayo,yoo"
///     spad_fappend_delim("%s", '!' " no null"); // "ayo,yoo nonull!"
HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice spad_fappend(StringPad* self, const char* fmt, ...);

#define spad_fappend_nl(_self, fmt, ...) ({\
  const char _tmp = (_self)->delim; \
  (_self)->delim = '\n'; \
  const sslice _res = (spad_fappend((_self), fmt __VA_OPT__(, ) __VA_ARGS__)); \
  (_self)->delim = _tmp;\
  _res;\
})

#define spad_fappend_sp(fmt, ...)({\
  const char _tmp = (_self)->delim; \
  (_self)->delim = ' '; \
  const sslice _res = (spad_fappend((_self), fmt __VA_OPT__(, ) __VA_ARGS__)); \
  (_self)->delim = _tmp;\
  _res;\
})



/// Same as [spad_fappend_delim], but passes '\0' as delim, making the delimiter effectively be an empty/no space!
HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice spad_fappend(StringPad* self, const char* fmt, ...);

/// Exactly the same as [spad_fappend_delim], but takes a va_list instead of
/// variadic arguments.
METHOD
sslice spad_vfappend(StringPad* self, const char* fmt,  va_list args);

