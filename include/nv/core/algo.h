// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdarg.h>
#include <stddef.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include <math.h>

#include "nv/core/log.h"

BEGIN_C_DECLS

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define CONCAT3_(a, b, c) a##b##c
// taken from any kind of AI. Which, would you lookie here, not even 5 min of searching on internet, Claude stole this
// code from here: https://github.com/donmccaughey/va_args_count/blob/master/va_args_count.h 11 years ago!!! I gotta
// give credit where credit is due. Fuck AI. Fuck Claude.
#define VA_ARGS_LEN(...) \
  VA_ARGS_LEN_(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define VA_ARGS_LEN_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, \
                     ...)                                                                                          \
  N

#define array(T, N)                                                    \
  /* conveinence for declaring static array of type (T) of size (N) */ \
  __typeof__(T[N])

#define ptr(T)                                                 \
  /* conveinence for declaring pointer types. bye, bye '*'! */ \
  __typeof__(T)*

#define type_eq(a, b)                                                         \
  /* compares an expression (a) with given type (b) to see if their types are \
   * the same */                                                              \
  _Generic((a), __typeof__(b): true, default: false)

#define typeof_ptr(_p) __typeof(*(_p))*

#define assert_type_eq(a, b)                                               \
  /* same as [type_eq] macro, but fails a static assertion if types do not \
   * match */                                                              \
  static_assert(type_eq(a, b))

#define is_string(s)                                                      \
  /* checks to see if s is of type [const char*] */                       \
  /* Do not that this will return false if given string is a mutable char \
   * buffer (char*) */                                                    \
  type_eq((s), char*)

#define cast(T, _src)                                                          \
  /* casts expression _src to be of type T */                                  \
  /* for a version of this macro that is specialized for casting pointers, see \
   * [pcast]*/                                                                 \
  ((__typeof__(T))(_src))

#define pcast(T, _ptr)                                                    \
  /* casts a given pointer (_ptr) to be of type T */                      \
  /* for a version of this macro that just does general casts between any \
   * given type and an expresion; see: [cast]*/                           \
  (cast(__typeof__(T*), (_ptr)))

#define size_of(_e) /* Same as the standard keyword 'sizeof', but may be a little more 'correct' and 'type-safe'(ish), \
                       as it wraps the target of sizeof in __typeof first */                                           \
  sizeof(__typeof((_e)))

#define align_of(_e) /*Same as the standard keyword 'alignof', but may be a little more 'correct' and \
                        'type-safe'(ish), as it wraps the target of alignof in __typeof first  */     \
  alignof(__typeof((_e)))

#define fmin(a, b)                     \
  (cast(__typeof__((a)), _Generic((a), \
            i8: fmin,                  \
            u8: fmin,                  \
            i16: fmin,                 \
            u16: fmin,                 \
            i32: fmin,                 \
            u32: fmin,                 \
            i64: fmin,                 \
            u64: fmin,                 \
            f32: fmin,                 \
            f64: fmin,                 \
            f128: fminl)(a, b)))

#define fmax(a, b)                     \
  (cast(__typeof__((a)), _Generic((a), \
            i8: fmax,                  \
            u8: fmax,                  \
            i16: fmax,                 \
            u16: fmax,                 \
            i32: fmax,                 \
            u32: fmax,                 \
            i64: fmax,                 \
            u64: fmax,                 \
            f32: fmax,                 \
            f64: fmax,                 \
            f128: fmaxl)(a, b)))

#define cmin(_a_, _b_)                          \
  ({                                            \
    constexpr const __typeof__(_a_) _a = (_a_); \
    constexpr const __typeof__(_b_) _b = (_b_); \
    _a < _b ? _a : _b;                          \
  })

#define cmax(_a_, _b_)                          \
  ({                                            \
    constexpr const __typeof__(_a_) _a = (_a_); \
    constexpr const __typeof__(_b_) _b = (_b_); \
    _a > _b ? _a : _b;                          \
  })

#define min(_a_, _b_)                 \
  ({                                  \
    const __typeof__(_a_) _a = (_a_); \
    const __typeof__(_b_) _b = (_b_); \
    _a < _b ? _a : _b;                \
  })

#define max(_a_, _b_)                 \
  ({                                  \
    const __typeof__(_a_) _a = (_a_); \
    const __typeof__(_b_) _b = (_b_); \
    _a > _b ? _a : _b;                \
  })

#define is_null(_p)                                   \
  /* checks if given pointer (p) is equal to null. */ \
  (nullptr == (_p))

#define is_not_null(_p)                                  \
  /* checks if given pointer (p) is not equal to null */ \
  (!(is_null((_p))))

#define clamp(_x, _min, _max)                                          \
  /* clamps a value values (x) to be between (min) and (max) */        \
  /* i.e.: 'clamp(-1, 0, 5) == 0;', or 'clamp(650, 0, 100) == 100;' */ \
  (max((_min), min((_x), (_max))))

// NOTE: I kind of like these 2 macros in a guilty pleasure kind of way lmao...
//  i might one day use them, but idk its kinda ugg and seems too distant to C
//  for it to make any sense to other developers coming across it while reading
//  this codebase
//  #define deref(p) (*(p))
//  #define ref &
//

#define UNUSED(_v) ((void)_v)

#define make(T, ...) /* Conveinence macro for creating new structs on stack. its possible  to pass a value instead of \
                        a type as the first parameter to this   macro. the type of the resulting struct will be the   \
                        type of value  given. Note that this does not do anything with the value, and   does not      \
                        create a copy of the value passed in (if any)*/                                               \
  ((__typeof__(T)){__VA_ARGS__})

#define make_zeroed(T) /* Same as [make] macro, but initializes given type T's \
                          fields to all be set to 0. */                        \
  (make(T))

/// Offsetof polyfill
#ifndef offsetof
#define offsetof(T, _m) ((isize)(&((T*)0)->_m))
#endif

#define typeof_field(T, _name) __typeof((__typeof(T)*){}->_name)

#define sizeof_field(T, _name_) (sizeof(__typeof(make_zeroed(T)._name_)))

#define alias(T) /* conveinence macro for defining structs to avoid having to \
                    write out the struct name 3 times*/                       \
  typedef struct T T

#define bailerr_with(_expr, _retval)                                         \
  /* evaluates given expression that returns [error] (int), and returns from \
   * surrounding function with the error value if it is no equal to 0. This  \
   * macro can only be used inside functions that return [error](int) */     \
  do {                                                                       \
    const error _err = (_expr);                                              \
    if (_err != 0) {                                                         \
      LOG_ERROR("<<Bail>> => %s", error_string(_err));                       \
      return (_retval);                                                      \
    }                                                                        \
  } while (0)

#define bailerr_withv(_expr)                                                 \
  /* evaluates given expression that returns [error] (int), and returns from \
   * surrounding function with the error value if it is no equal to 0. This  \
   * macro can only be used inside functions that return [error](int) */     \
  do {                                                                       \
    const error _err = (_expr);                                              \
    if (_err != 0) {                                                         \
      LOG_ERROR("<<Bail>> => %s", error_string(_err));                       \
      return;                                                                \
    }                                                                        \
  } while (0)

#define bailerr(_expr)                                                       \
  /* evaluates given expression that returns [error] (int), and returns from \
   * surrounding function with the error value if it is no equal to 0. This  \
   * macro can only be used inside functions that return [error](int) */     \
  do {                                                                       \
    const error _err = (_expr);                                              \
    if (_err != 0) {                                                         \
      LOG_ERROR("<<Bail>> => %s", error_string(_err));                       \
      return _err;                                                           \
    }                                                                        \
  } while (0)

#define tryerr bailerr

// #define tryerr_or(expr, orelse) do {\
//     const error _err = (expr); \
//     if (_er != 0) { (orelse); }\
// } while(0)

#define tryerr_or(_expr, _orelse)                                             \
  /* Same as [tryerr] macro, but instead of returning error value in the case \
   * it is not equal to 0, a given expression is ran. you can use this macro  \
   * anywhere in the case you want to handle an error dynamicaly inside a     \
   * function that does not return [error](int)*/                             \
  do {                                                                        \
    const error _err = (_expr);                                               \
    if (_err != 0) {                                                          \
      (_orelse);                                                              \
    }                                                                         \
  } while (0)

#define tryerr_or_cb(_expr, _cb, ...)                                          \
  /* Same as [tryerr_or], but instead of running a given expression in the     \
   * case where error is not equal to 0, a given callback function is called.  \
   * Callback function can have any signature, as long as it has at least a    \
   * parameter of type [error](int) as its first parameter. extra parameters   \
   * to this macro are forwarded to error handling callback function. For a    \
   * version of this macro that returns from the surrounding function with the \
   * return value of given callback, see: [tryerr_or_ret]*/                    \
  do {                                                                         \
    const error _err = (_expr);                                                \
    if (_err != 0) {                                                           \
      (_cb)(_err, __VA_ARGS__);                                                \
    }                                                                          \
  } while (0)

#define tryerr_or_ret(_expr, _cb, ...)                                 \
  /* Same as [tryerr_or_cb], but returns from the surrounding function \
     with the value returned by given callback function. Due to this,  \
     this macro can only be called inside functions with the same      \
     return type as the surrounding function. */                       \
  (tryerr_or_cb((_expr), (_cb), __VA_ARGS__))

// #define tryerr_orelse(_try_expr, _else_expr, _def) ({ (_try_expr) != OK ? (_else_expr) : (_def); })

#define tptr_new(p, enable) /*create a tagged pointer that uses unused bits for a boolean flag  */ \
  (__typeof((p)))(((addr)(p)) | ((enable) ? 1 : 0))
#define tptr_ptr(p) /* get original pointer value from a tagged pointer */ ((__typeof((p)))((addr)(p) & ~1UL))
#define tptr_tag(p) /*get the tag value from a tagged pointer*/ (((addr)(p)) & 1)

#define prefix_offset(                                                                                         \
    _ptr, T) /*jump pointer backwards to where its metadata is. Used in implementations of opaque pointers  */ \
  (&((pcast(T, (_ptr)))[-1]))

#define bitset(_set, _flag) ((_set) |= (_flag))

#define bitclear(_set, _flag) ((_set) &= ~(_flag))
#define bittoggle(_set, _flag) ((_set) ^= (_flag))
#define bithas(_set, _flag) (cast(bool, (_set) & (_flag)))

#define bithasall(_set, _flags) (((_set) & (_flags)) == (_flags))
#define bithasany(_set, _flags) ((_set) & (_flags))

#define is_empty(_v) /* generic over any type with a 'len' field */ ((_v).len == 0)
#define is_invalid(_v) /* generic over any type with a 'len' field */ ((_v).len <= 0)
#define is_falsey(_v) /* does value coerce to false?  */ ((bool)(!(_v)))
#define is_truthy(_v) /* does value coerce to true?  */ (!is_falsey((_v)))

#define zeroed /* Easily get a zeroed struct of any type */ make_zeroed

#define is_none(_v)                                              \
  ({                                                             \
    static constexpr const auto _NONE = zeroed(__typeof(*(_v))); \
    const auto _val = (_v);                                      \
    memcmp(_val, &_NONE, sizeof(__typeof(_NONE))) == 0;          \
  })

#define is_zeroed is_none

#define DynSizeType(T, ...) \
  struct {                  \
    __VA_ARGS__;            \
    __typeof(T) data[];     \
  }

#define SlimDST(T) DynSizeType(T, i64 size; i64 size2)

typedef SlimDST(byte) ByteDST;

#define IS_POWER_OF_2(n) ((n & (n - 1)) == 0)

PURE_FUNC
PARAMS_NONNULL(1)
u32 fnv_hash32(const char* string, isize len) WHERE(len > 0);

PURE_FUNC
PARAMS_NONNULL(1)
u64 fnv_hash64(const char* string, isize len) WHERE(len > 0);

CONST_FUNC
static inline bool is_power_of_2(isize n) { return IS_POWER_OF_2(n); }

/// @brief a safer, more efficient [strncat]
/// @details Its not required that dest string ends with a null terminal, however after this function returns
/// there is gauranteed to be a null terminal, which will get overwritten on the next invokation of this function using
/// the original dest buffer
/// @param (i64 dest_count) :: The current number of characters in dest string, not including null terminal (if any)
/// @param (i64* out_new_count) new count of destination string, not including null character. You can use this value to
/// pass to the next invokation of this function using same original destination string
NvError try_stringcat(char* dest, i64 dest_count, i64 dest_size, const char* src, i64 srclen, i64* out_new_count)
    PARAMS_NONNULL(1, 4);

/// @brief fastpath version of [try_stringcat]
/// @details [try_stringcat] does a lot of checking to verify that its parameters are valid, this function
/// does not do any of it and operates under the assumption that the caller is passing good parameters
/// it only does the bare minimum and asserts pointer parameters are non-null, and checking the input string can fit in
/// dest buffer, truncating it if it doesnt
/// @returns  new character count of destination string, not including null terminal. You can use this value to
/// pass to the next invokation of stringcat
i64 stringcat(char* dest, i64 dest_count, i64 dest_size, const char* src, i64 srclen) PARAMS_NONNULL(1, 4);

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
void* ptr_nonnull_(const void* ptr) WHERE(ptr_nonnull_(ptr) == ptr);

// CLANG_NON_NULL_END

#define ptr_nonnull(_ptr) cast(typeof_ptr(_ptr), ptr_nonnull_((const void*)(_ptr)))

#if LIBNV_USE_SHORT_NAMES == 1

#ifndef punwrap
#define punwrap ptr_nonnull
#endif

#endif

/// Same as [ptr_nonnull], but fails with a user provided, message.
/// NOTE: Because this function is inteded to be used in hot paths, the user
/// must build error messages themselves. if this function were a printf-style function, it
/// would at the very worst, pass a bunch of parameters onto the stack that will never be used most the time, callers
/// are also encouraged to keep messages light (no formatting, just as simple string literal)
RETURNS_NON_NULL
void* ptr_expect_(const void* ptr, const char* msg);

#if !defined(pexpect) && (!defined(LIBNV_NO_USE_SHORT_NAMES) || LIBNV_NO_USE_SHORT_NAMES == 0)
#define pexpect(_p, _msg) cast(typeof_ptr(_p), ptr_expect_((const void*)(_p), (_msg)))
#endif

#ifndef STRLEN_UPPER_BOUND

#define STRLEN_UPPER_BOUND                                                    \
  /* Upper bound used by [stringlen] as the max_len parameter to [str_len] */ \
  /* NOTE: I decided to make this a macro so that it can be configurable to   \
   * each build (-D compiler flag)*/                                          \
  (INT64_MAX - 1)

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
/// @details reutrns number of characters in string, not including null terminator
PURE_FUNC
static inline isize stringlen(const char* string) { return str_len(string, STRLEN_UPPER_BOUND); }

PURE_FUNC
bool stringeq(const char* left, const char* right);

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

/// @brief Determines printf-style format string resulting length, excluding null-terminator
HEDLEY_PRINTF_FORMAT(1, 2)
PURE_FUNC
i32 fstring_length(const char* fmt, ...);

/// @brief Determines printf-style format string resulting length excluding, null-terminator
/// @details does not modify va_list args
PURE_FUNC
i32 vfstring_length(const char* fmt, va_list args);

/// @brief concat no more than dest_len bytes of expanded printf-style string to dest
sslice vfconcat(char* dest, i64 dest_len, i64 dest_capacity, const char* fmt, va_list args) PARAMS_NONNULL(1, 4);

/// @brief concat no more than dest_len bytes of expanded printf-style string to dest
sslice fconcat(char* dest, i64 dest_len, i64 dest_capaccity, const char* fmt, ...) HEDLEY_PRINTF_FORMAT(4, 5);

static constexpr const i32 ONE = 1;
#define IS_BIG_ENDIAN() ((*(char*)&ONE) == 0)

typedef enum Endianness { LITTLE_ENDIAN, BIG_ENDIAN, NETWORK_BYTEORDER = BIG_ENDIAN } Endianness;

PURE_FUNC
Endianness endianness(void);

PURE_FUNC
bool is_little_endian(void);

PURE_FUNC
bool is_big_endian(void);

#define Bytes(N)  \
  struct {        \
    byte data[N]; \
  }

#define BytesOf(T)                   \
  union {                            \
    __typeof(T) val;                 \
    byte bytes[sizeof(__typeof(T))]; \
  }

#define bytesof_new(_val) ((BytesOf(__typeof((_val)))){.val = (_val)})
#define bytes_of(_val)                                  \
  ({                                                    \
    static constexpr const auto _SIZE = sizeof((_val)); \
    auto _v = (_val);                                   \
    auto _bs = (BytesOf(__typeof(_v))){.val = _v};      \
    Bytes(_SIZE) _res = {};                             \
    memcpy(_res.data, _bs.bytes, _SIZE);                \
    _res;                                               \
  })

typedef BytesOf(bool) BoolBytes;
typedef BytesOf(i16) Int16Bytes;
typedef BytesOf(u16) UInt16Bytes;
typedef BytesOf(i32) Int32Bytes;
typedef BytesOf(u32) UInt32Bytes;
typedef BytesOf(i64) Int64Bytes;
typedef BytesOf(u64) UInt64Bytes;
typedef BytesOf(usize) UsizeBytes;
typedef BytesOf(isize) IsizeBytes;
typedef BytesOf(f32) FloatBytes;
typedef BytesOf(f64) Float64Bytes;

END_C_DECLS
