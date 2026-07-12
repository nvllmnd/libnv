// SPDX-FileCopyrightText: 2025 Matthew McDade <zedex805@protonmail.com>
// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifndef __cplusplus
#define noexcept
#endif

#include "hedley.h"

#ifdef __cplusplus

#define BEGIN_C_DECLS extern "C" {

#define END_C_DECLS }

#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

#if SYSTEM_WINDOWS
#define SYSTEM_POSIX 0
#else
#define SYSTEM_POSIX 1
#endif

#ifndef LIBNV_NO_USE_SHORT_NAMES
#define LIBNV_NO_USE_SHORT_NAMES 0
#endif

#define LIBNV_USE_SHORT_NAMES (LIBNV_NO_USE_SHORT_NAMES == 0 ? 1 : 0)

#if defined(__clang__) && __clang__

// HEDLEY_PRAGMA(clang diagnostic push);

#define CLANG_LIKELY [[clang::likely]]
#define CLANG_UNLIKELY [[clang::unlikely]]

#define CLANG_NON_NULL_BEGIN           \
                                       \
  HEDLEY_PRAGMA(clang diagnostic push) \
                                       \
  HEDLEY_PRAGMA(clang diagnostic ignored "-Wnullability-extension")

#define CLANG_NON_NULL_END HEDLEY_PRAGMA(clang diagnostic pop)

// #pragma clang diagnostic pop

#define CLANG_NON_NULL _Nonnull
#define CLANG_NULLABLE _Nullable

#else

#define CLANG_LIKELY
#define CLANG_UNLIKELY

#define CLANG_NON_NULL
#define CLANG_NULLABLE
#define CLANG_NON_NULL_BEGIN
#define CLANG_NON_NULL_END

#endif  // if defined(__clang__) && __clang__

#if defined(__has_c_attribute)
#define LIBNV_HAS_ATTRIBUTE_SUPPORT 1
#else
#define LIBNV_HAS_ATTRIBUTE_SUPPORT 0
#endif

// NOTE: For support with older compilers
// for use primarily in header files
#if LIBNV_HAS_ATTRIBUTE_SUPPORT
#define nv_nodiscard_msg(_m) [[nodiscard(_m)]]
#define nv_nodiscard [[nodiscard]]

#else
#define nv_nodiscard_msg(_m)
#define no_nodiscard
#define constexpr
#endif

#ifndef LIBNV_USE_SHORT_ATTRIBUTE_NAMES
#define LIBNV_USE_SHORT_ATTRIBUTE_NAMES 1
#endif

#ifndef LIBNV_DEFINE_FEATURE_MACROS
#define LIBNV_DEFINE_FEATURE_MACROS 1
#endif

#if LIBNV_DEFINE_FEATURE_MACROS == 1

#include "nv/core/ext.h"

#endif

// #if LIBNV_USE_SHORT_ATTRIBUTE_NAMES == 1
// #define nodiscard nv_nodiscard
// #define nodiscard_msg nv_nodiscard_msg
// #endif

// #pragma clang assume_nonnull begin
// #if defined(__clang__) && __clang__
// #pragma clang assume_nonnull end
// #endif

// NOTE: Some Hedley aliases for A E S T H E T I C S and
// conveinience

///
#define PURE_FUNC                                                              \
  /*The function has no side-effects, and the return value depends only on the \
    parameters and/or global variables.*/                                      \
  HEDLEY_PURE

#define CONST_FUNC                                                              \
  /* The function has no side-effects, and the return value depends only on the \
     parameters. Note that pointer arguments are not allowed.*/                 \
  HEDLEY_CONST

#define MALLOC_FUNC                                                                                                 \
  /*Inform the compiler that the pointer returned by this function does not alias any other pointer, and that there \
   * are no pointers to valid objects in the storage pointed to. */                                                 \
  HEDLEY_MALLOC

#define FORMAT_FUNC                                                                                              \
  /*                                                                                                             \
   Inform the compiler/analyzer that the function takes a printf-style format string, so that it can check the   \
  arguments.                                                                                                     \
                                                                                                               \ \
  string_idx                                                                                                     \
      Index (starts from 1, left-to-right) of the format string parameter.                                       \
  first_to_check                                                                                                 \
      Index of the first user-supplied parameter to check.                                                       \
  */                                                                                                             \
  HEDLEY_PRINTF_FORMAT

#define BITFLAG_CAST                                                                \
  /*example: const enum Foo foo = HEDLEY_FLAGS_CAST(enum Foo, FOO_BAR | FOO_BAZ);*/ \
  HEDLEY_FLAGS_CAST

#define BITFLAG_ENUM                                    \
  /*Annotate an enumeration as containing bit flags. */ \
  /* example: */                                        \
  /*enum Foo {*/                                        \
  /*        FOO_BAR = 1 << 0,  */                       \
  /*        FOO_BAZ = 1 << 1,  */                       \
  /*       FOO_QUX = 1 << 2   */                        \
  /* } BITFLAG_ENUM;*/                                  \
  HEDLEY_FLAGS

/// Pointer does not alias (no other pointer points to same region of memory)
/// aka this pointer is 'unique'
#define NO_ALIAS HEDLEY_RESTRICT

///  Tell the compiler to always inline the function, even if it thinks doing so
///  would be a bad idea.
#define FORCE_INLINE HEDLEY_ALWAYS_INLINE

#define INLINE_ALWAYS FORCE_INLINE

/// Tell the compiler that the function will never throw a C++ exception
///    .Note that this can improve performance even in C mode.Use only if you're
///    sure your function will never call a function which throws a C++
///    exception, even indirectly.
#define NOTHROW HEDLEY_NO_THROW

#define NORETURN                                                                                                     \
  /*Function is does not return. This is important for the compiler to be able to reason about later events; for     \
   * example, if you call a no-return function if a variable is NULL, then the compiler can assume that the variable \
   * is non-NULL in the remainder of the function, which allows you to pass it to a function as a non-NULL           \
   * parameter.*/                                                                                                    \
  HEDLEY_NO_RETURN

/// Tell the compiler that the pointer will not escape the function call. For
/// more information, see the documentation for clang's noescape attribute.
/// Can be used on functions that free memory to help clang's static analysis
#define CLEANUP_FUNC HEDLEY_NO_ESCAPE

/// Emit a non-fatal compile-time informational message.
/// Compile Time Message
#define CMESSAGE HEDLEY_MESSAGE

/// Alias for CMESSAGE (HEDLEY_MESSAGE)
#define CINFO CMESSAGE

/// Emit a compile-time warning. If compiling with fatal warnings (e.g., -Wall
/// on GCC, or /WX on MSVC), these may be treated as fatal.
#define CWARNING HEDLEY_WARNING

/// Alias for CWARNING (HEDLEY_WARNING)
#define CWARN CWARNING

/// Compile-time precondition.
///
/// expr
///    Expression to check.
///
///
/// Example:
/// ----------------------------------------
///                                        |
/// int foo(int a, int b) WHERE(a < b);    |
///                                        |
/// ----------------------------------------
///
///
#define PRECONDITION HEDLEY_REQUIRE
#define WHERE PRECONDITION

///  Function attribute which signals that the compiler should emit a diagnostic
///  if the return value is discarded without being checked.
///
///  Must provide a reason why. Otherwise just use [[nodiscard]] since that is
///  supported since C++17
#define NODISCARD(reason) HEDLEY_WARN_UNUSED_RESULT_MSG

/// Same as NODISCARD, but with a default message telling the user that the
/// returned value owns a dynamic resource and must be cleaned up!
#define MUST_CLEANUP                                                           \
  NODISCARD(                                                                   \
      "Returned pointer or wrapping struct owns a dynamic resource that must " \
      "be cleaned up after caller is done using! DO NOT IGNORE OR SUFFER "     \
      "MEMORY LEAKS!!!!!!!!!")

///  Convert value to a string. value can be a preprocessor macro.
#define STRINGIFY HEDLEY_STRINGIFY

/// Concatenate two macros.
#define MACRO_CONCAT HEDLEY_CONCAT

/// List parameters which must never be NULL (for example, because they are
/// unconditionally dereferenced in the function body).
///
/// ...
///     Variadic list of indexes (starting from 1, left-to-right) of parameters
///     which must never be NULL.
///
/// Example:
/// ----------------------------------------------------------------
///                                                                |
/// HEDLEY_NON_NULL(1,3)                                           |
/// void do_something(Context* ctx_cant_be_null, Foo* maybe_null,  |
///                  Bar* cant_be_null);                           |
///                                                                |
/// ----------------------------------------------------------------
///
#define PARAMS_NONNULL HEDLEY_NON_NULL

/// Inform the compiler/analyzer that the code should never be reached (even
/// with invalid input).
#define UNREACHABLE HEDLEY_UNREACHABLE

///  Inform the compiler/analyzer that the code should never be reached or, for
///  compilers which don't provide a way to provide such information, return a
///  value.
#define UNREACHABLE_RETURN HEDLEY_UNREACHABLE_RETURN

/// The function will alwoys return a non-null value.
///
/// This may allow the compiler to skip some null-pointer checks.
#define RETURNS_NON_NULL HEDLEY_RETURNS_NON_NULL

///  Explicitly tell the compiler to fall through a case in the switch
///  statement. Without this, some compilers may think you accidentally omitted
///  a "break;" and emit a diagnostic.
#define FALLTHROUGH HEDLEY_FALL_THROUGH

#define METHOD                                                                                                        \
  /* alias for PARAMS_NONNULL(1), which ensures that the first parameter (self, for a method function), is always     \
   * non-null */                                                                                                      \
  /*Use this for functions intended to be used like methods for types, where those functions take a pointer to struct \
   * as thier first parameter */                                                                                      \
  PARAMS_NONNULL(1)

#define RECEIVER                                                                                                   \
  /* same as [METHOD_FN]. Use this for functions intended to be used like methods for types, where those functions \
   * take a pointer to struct as thier first parameter  */                                                         \
  METHOD

// #define LIKELY(expr)                      (__builtin_expect                 (!!(expr),    1                  ))
// #define UNLIKELY(expr)                    (__builtin_expect                 (!!(expr),    0                  ))

#define RETURNS_ERROR [[nodiscard("Ignoring returned error value can cause unexpected results!")]]

#define RETURNS_RESOURCE                                                                                             \
  [[nodiscard(                                                                                                       \
      "Returned value is either a pointer or contains a pointer! Ignoring value would cause memory leak! Caller is " \
      "expected to call appropriate free function on returned value")]]

#define HANDLE_ERROR \
  [[nodiscard("Function called returns an Error value that must be handled to avoid UB/Null ptr errors!")]]

// NOTE: I had to rip out the hedley.h defines for LIKELY and UNLIKELY (and i just brought along PREDICT and
// UNPREDICTABLE cus why not lol) so that i can put () around the compiler extension, so i can write my if statements
// like "if UNLIKELY(ptr == nullptr) {}" instead of "if (UNLIKELY(ptr == false)) {}"
#if defined(PREDICT)
#undef PREDICT
#endif
#if defined(LIKELY)
#undef LIKELY
#endif
#if defined(UNLIKELY)
#undef UNLIKELY
#endif
#if defined(UNPREDICTABLE)
#undef UNPREDICTABLE
#endif
#if HEDLEY_HAS_BUILTIN(__builtin_unpredictable)
#define UNPREDICTABLE(expr) (__builtin_unpredictable((expr)))
#endif
#if (HEDLEY_HAS_BUILTIN(__builtin_expect_with_probability) && !defined(HEDLEY_PGI_VERSION)) || \
    HEDLEY_GCC_VERSION_CHECK(9, 0, 0) || HEDLEY_MCST_LCC_VERSION_CHECK(1, 25, 10)
#define PREDICT(expr, value, probability) (__builtin_expect_with_probability((expr), (value), (probability)))
#define PREDICT_TRUE(expr, probability) (__builtin_expect_with_probability(!!(expr), 1, (probability)))
#define PREDICT_FALSE(expr, probability) (__builtin_expect_with_probability(!!(expr), 0, (probability)))
#define LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#elif (HEDLEY_HAS_BUILTIN(__builtin_expect) && !defined(HEDLEY_INTEL_CL_VERSION)) ||                        \
    HEDLEY_GCC_VERSION_CHECK(3, 0, 0) || HEDLEY_INTEL_VERSION_CHECK(13, 0, 0) ||                            \
    (HEDLEY_SUNPRO_VERSION_CHECK(5, 15, 0) && defined(__cplusplus)) || HEDLEY_ARM_VERSION_CHECK(4, 1, 0) || \
    HEDLEY_IBM_VERSION_CHECK(10, 1, 0) || HEDLEY_TI_VERSION_CHECK(15, 12, 0) ||                             \
    HEDLEY_TI_ARMCL_VERSION_CHECK(4, 7, 0) || HEDLEY_TI_CL430_VERSION_CHECK(3, 1, 0) ||                     \
    HEDLEY_TI_CL2000_VERSION_CHECK(6, 1, 0) || HEDLEY_TI_CL6X_VERSION_CHECK(6, 1, 0) ||                     \
    HEDLEY_TI_CL7X_VERSION_CHECK(1, 2, 0) || HEDLEY_TI_CLPRU_VERSION_CHECK(2, 1, 0) ||                      \
    HEDLEY_TINYC_VERSION_CHECK(0, 9, 27) || HEDLEY_CRAY_VERSION_CHECK(8, 1, 0) ||                           \
    HEDLEY_MCST_LCC_VERSION_CHECK(1, 25, 10)
#define PREDICT(expr, expected, probability) \
  (((probability) >= 0.9) ? __builtin_expect((expr), (expected)) : (HEDLEY_STATIC_CAST(void, expected), (expr)))
#define PREDICT_TRUE(expr, probability)                                                                         \
  (__extension__({                                                                                              \
    double hedley_probability_ = (probability);                                                                 \
    ((hedley_probability_ >= 0.9) ? __builtin_expect(!!(expr), 1)                                               \
                                  : ((hedley_probability_ <= 0.1) ? __builtin_expect(!!(expr), 0) : !!(expr))); \
  }))
#define PREDICT_FALSE(expr, probability)                                                                        \
  (__extension__({                                                                                              \
    double hedley_probability_ = (probability);                                                                 \
    ((hedley_probability_ >= 0.9) ? __builtin_expect(!!(expr), 0)                                               \
                                  : ((hedley_probability_ <= 0.1) ? __builtin_expect(!!(expr), 1) : !!(expr))); \
  }))
#define LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define PREDICT(expr, expected, probability) (HEDLEY_STATIC_CAST(void, expected), (expr))
#define PREDICT_TRUE(expr, probability) (!!(expr))
#define PREDICT_FALSE(expr, probability) (!!(expr))
#define LIKELY(expr) (!!(expr))
#define UNLIKELY(expr) (!!(expr))
#endif
#if !defined(UNPREDICTABLE)
#define UNPREDICTABLE(expr) PREDICT(expr, 1, 0.5)
#endif

// ###################################################
// ######### Custom Extensions to hedley.h ###########
// ###################################################

#if HEDLEY_HAS_ATTRIBUTE(counted_by)
#ifndef ATTR_COUNTED_BY
#define ATTR_COUNTED_BY(_field) __attribute__((counted_by(_field)))
#endif  // ifndef ATTR_COUNTED_BY
#ifndef ATTR_COUNTED_BY

#define ATTR_COUNTED_BY
#endif  // ifndef ATTR_COUNTED_BY

#endif  // HEDLEY_HAS_ATTRIBUTE(counted_by)

#if defined(__has_include)
#define HAS_INCLUDE(...) __has_include(__VA_ARGS__)
#else
#warning "Compiler does not support __has_include! using HAS_INCLUDE macro will always return 0"
#define HAS_INCLUDE(...) 0
#endif
