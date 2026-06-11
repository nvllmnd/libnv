#pragma once

#include <stdarg.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"
#include "nv/core/core_types.h"
#include "nv/memory/alloc.h"

/// @brief A StringPad used for building Strings dynamically
/// @details This type stores a [VMarker] at its creations which it uses to clean up
/// memory it uses during its lifetime by calling [spad_destroy].
/// @warning This StringPad is intended to use its backing [VirtMem] exclusively. No other allocating vmem_*  calls
/// should be made while this StringPad is still being used to build a string. Doing so will corrupt the output of the
/// string at best, at worst you could leak sensitive data/memory to some nefarious user of your software!
struct StringPad {
  /// @brief Borrowed [VirtMem].
  /// @remarks Keep in mind that if this VirtMem is used at all while this StringPad is used,
  /// the resulting strings will be corrupted
  struct VirtMem* vm;
  /// @brief [VMarker], used to reset [VirtMem]
  /// @details resets [VirtMem]'s top pointer back to where it was when we first created this [StringPad], cleaning up
  /// memory used by it back to the VirtMem we allocate out of
  /// @warning Changing this value, and then calling [spad_destroy] is dangerous and should be avoided
  i32 mark;
  /// @brief separator for appended strings
  /// @details if 0, no delimeter is used and strings are appended together with no space or seaparators
  char delim;
  /// @brief size of current string in bytes
  i32 size;
  /// @brief start of stringpad in VirtMem
  const char* begin;
  /// @brief end of stringpad in VirtMem, always points to byte directly after begin + size
  const char* end;
};
alias(StringPad);

/// @brief Creates new StringPad with given delimiter.
/// @details you can change the delimiting character by changing the value of the delim field on StringPad struct
PARAMS_NONNULL(1)
StringPad spad_delim_new(struct VirtMem* vm, char delim);

/// @brief creates new StringPad with no delimiting character
/// @details unless caller sets delim field to a new value other than '\0', strings
/// will be concatenated together with no delimiting charater, or any space in betweent them
PARAMS_NONNULL(1)
static inline StringPad spad_new(struct VirtMem* vm) { return spad_delim_new(vm, '\0'); }

/// @brief copies buff_len - 1 characters from SPad's inner string buffer into given memory buffer (buff_out)
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

/// @brief Writes n characters of given string to this the end of this StringPad
/// @details Concats a delimter to the end of string, if any delimiter is specified
sslice spad_nappend(StringPad* self, const char* s, i32 len);
METHOD

/// @brief Writes string to the back of this StringPad.
/// @details Concats a delimter to the end of string, if any delimiter is specified
sslice spad_append(StringPad* self, const char* s);

/// @brief Formats given format string and appends it to the back of the StringPad's inner buffer. Takes a format string
/// literal and printf-style variadic format value args
///
/// @param (const char* fmt) - printf-style format string literal
HEDLEY_PRINTF_FORMAT(2, 3)
METHOD
sslice spad_fappend(StringPad* self, const char* fmt, ...);

#define spad_fappend_nl(_self, fmt, ...)                                         \
  ({                                                                             \
    const char _tmp = (_self)->delim;                                            \
    (_self)->delim = '\n';                                                       \
    const sslice _res = (spad_fappend((_self), fmt __VA_OPT__(, ) __VA_ARGS__)); \
    (_self)->delim = _tmp;                                                       \
    _res;                                                                        \
  })

#define spad_fappend_sp(fmt, ...) \
  ({                                                                             \
    const char _tmp = (_self)->delim;                                            \
    (_self)->delim = ' ';                                                        \
    const sslice _res = (spad_fappend((_self), fmt __VA_OPT__(, ) __VA_ARGS__)); \
    (_self)->delim = _tmp;                                                       \
    _res;
METHOD
/// @brief same as [spad_fappend] but takes a [va_list] instead of var_args
sslice spad_vfappend(StringPad* self, const char* fmt, va_list args);


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
