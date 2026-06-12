// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdarg.h>

#include "nv/core/attributes.h"
#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/alloc.h"
#include "nv/memory/vmem.h"

/// @brief A StringPad used for building Strings dynamically
/// Essentially a [Vallocator] that only operates on strings, and extended for cloning/copying
struct StringPad {
  /// @brief set to true when spad_build_start is invoked and set to false when spad_build_end is invoked.
  /// If this value is true when spad_build_start is called, this implementation calls abort and halts execution
  bool inuse;

  Vallocator mem;
};
alias(StringPad);

/// @brief creates new StringPad with no delimiting character
/// @details unless caller sets delim field to a new value other than '\0', strings
/// will be concatenated together with no delimiting charater, or any space in betweent them
StringPad spad_new(i64 size_bytes);

/// @brief duplicates currently built string using given allocator
/// @param(Allocator alloc) - Used to duplicate Spad's inner string buffer
/// @returns null-terminated string slice pointing to where the cloned string lives in given allocator. slice length
/// does not include null terminating character
METHOD
sslice spad_clone_string(StringPad* self, Allocator alloc);

METHOD
PURE_FUNC
static inline i64 spad_capacity(const StringPad* self) {
  return self->mem.mem->size; 
}

METHOD
/// @brief ensures inner memory is reset back to start
void spad_build_start(StringPad* self);

METHOD
sslice spad_build_end(StringPad* self, Allocator alloc);

/// @brief copies buff_len - 1 characters from SPad's inner string buffer into given memory buffer (buff_out)
/// @param (char* buff_out) - writes `buff_len` - 1 characters into memory at this address. appends null-character to
/// end of `buff_out`. Must not be null and at least `buff_len` bytes long.
/// @param (i32 buff_len) - number of bytes to write form Spad to `buff_out`
/// @returns Number of bytes written to `buff_out`. May be less than `buff_len` if Spad's inner string buffer is of
/// shorter length than `buff_len`
PARAMS_NONNULL(1, 2)
i32 spad_clone_into(StringPad* self, char* buff_out, i32 buff_len);


PARAMS_NONNULL(1, 2)
i64 spad_build_end_into(StringPad* self, char* buff_out, i64 buff_len);

METHOD
/// @brief Writes n characters of given string to this the end of this StringPad
/// @details Concats a delimter to the end of string, if any delimiter is specified
sslice spad_nappend(StringPad* self, const char* s, i32 len);


METHOD
/// @brief Writes string to the back of this StringPad.
/// @details Concats a delimter to the end of string, if any delimiter is specified
sslice spad_append(StringPad* self, const char* s);

METHOD
/// @brief same as [spad_fappend] but takes a [va_list] instead of var_args
static inline sslice spad_vfappend(StringPad* self, const char* fmt, va_list args) {
  const sslice res = va_vfslice(&self->mem, fmt, args);
  // forget null character that vsnprintf appends to end of formatted string!
  // if we dont do this, the resulting string would have a bunch of null characters (one for each formatted string call)
  self->mem.cursor -= 1;
  return res;
}
/// @brief Formats given format string and appends it to the back of the StringPad's inner buffer. Takes a format string
/// literal and printf-style variadic format value args
///
/// @param (const char* fmt) - printf-style format string literal
HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
static inline sslice spad_fappend(StringPad* self, const char* fmt, ...) {
  va_list args;
  va_start(args);

  const sslice str = spad_vfappend(self, fmt, args);

  va_end(args);
  
  return str;
}



PURE_FUNC
static inline i64 spad_available(const StringPad* self) {
  return va_available(&self->mem); 
}

/// @brief size of string currently built so far
PURE_FUNC
static inline i64 spad_length(const StringPad* self) {
  return va_used_bytes(&self->mem); 
}


/// @brief Adds character to back of this StringPad
METHOD
char spad_putchar(StringPad* self, char c);

/// @brief adds byte to back of this StringPad
METHOD
u8 spad_putbyte(StringPad* self, u8 byte);

METHOD
void spad_clear(StringPad* self);

METHOD
void spad_clear_zeroed(StringPad* self);

/// @brief CLeans up memory used by this StringPad to the VirtMem it allocated out of
/// @details zeroes StringPad to prevent further use by it
void spad_destroy(StringPad* self);
