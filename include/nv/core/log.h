#pragma once

#include <stdio.h>
#include <stdarg.h>

#include "nv/core/attributes.h"
#include "nv/core/sslice.h"
#include "nv/core/intdefs.h"
#include "nv/memory/cstr.h"
#include "nv/core/debug.h"

// #include "memory/cstr.h"

// #define format(lit, ...)

// FORMAT_FUNC(1,2)
// void println(const char* fmt, ...);

/// Formats arguments int a new [cstr] using
/// a [printf] style format string
///
/// Returns empty string in case of formatting error
///
FORMAT_FUNC(1, 2)
RETURNS_RESOURCE
cstr format_string(const char* fmt, ...);

typedef enum FormatError {  Format__Error = -1,
  Format__Ok = 0
} FormatError;


// FormatError i64_into_str(i64 n, char* dst, i32 dst_len);

/// Creates a string representation of a given i64 integer into a
/// new [cstr]. This function is guaranteed to not allocate. However,
/// any digits past the [SMALL_BUF_SIZE]th (or [SMALL_BUF_SIZE - 1]th digit if (n) is a negative number, to account for the negative sign)
/// digit will be lost/truncated,
///
/// If you know the number you want to turn into
/// a string is <= [INT32_MAX], then see: [i32_to_cstr], as
/// that function is also gauranteed not to allocate memory, but
/// [SMALL_BUF_SIZE] (should) be large enough to represent [INT32_MAX] or [INT32_MIN]
/// as a string without losing any information
///
PURE_FUNC
cstr i64_truncate_into(i64 n);


/// Creates a string representation of a given i32 integer into  a
/// new [cstr]. This function is guaranteed to not allocate (on the heap).
/// as the parsed string is stored on the stack
PURE_FUNC
cstr i32_to_cstr(i32 n);


FORMAT_FUNC(3,4)
RETURNS_ERROR
FormatError format_with(char* dst, isize dst_len, const char* fmt, ...);

#define fdprint(fd, fmt, ...) (fprintf(fd, fmt __VA_OPT__(, ) __VA_ARGS__))

#define print(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(,) __VA_ARGS__))
#define eprint(fmt, ...) (fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__))
#define fprintln(fd, fmt, ...) (fdprint(fd, fmt "\n" __VA_OPT__(, ) __VA_ARGS__))

// #define fprintln(fd, fmt, ...) (fprintf(fd, fmt "\n" __VA_OPT__(, ) __VA_ARGS__))

#define println(fmt, ...) (fprintln(stdout, fmt, __VA_ARGS__))
// (fprintf(stdout, fmt "\n" __VA_OPT__(, ) __VA_ARGS__))

#define eprintln(fmt, ...) (fprintln(stderr, fmt, __VA_ARGS__))

/// Prints a given string [sslice] to
/// a file. This is a verstion of [fdprint] that does not require
/// null-terminated strings. However this function does not
/// do any formatting. If you need to print a formatted string. see [fdprint] and others
///
PARAMS_NONNULL(1)
void sfprint(FILE* fd, sslice str);

/// Same as [sfprint], but appends a newline character to the end of
/// give string [sslice]
/// 
/// Prints a given string [sslice] to
/// a file. This is a verstion of [fdprint] that does not require
/// null-terminated strings. However this function does not
/// do any formatting. If you need to print a formatted string. see [fdprint] and others
///
PARAMS_NONNULL(1)
void sfprintln(FILE* fd, sslice str);

void sprint(sslice str);

void sprintln(sslice str);

void seprint(sslice str);

void seprintln(sslice str);

#define FILE_FMT "%s[%s::%s]:%d => "
#define FILE_FMT_ARGS(CTX_NAME, ...) __FILE__, STRINGIFY(CTX_NAME), __func__, __LINE__ __VA_OPT__(,) __VA_ARGS__


#if ZEAL_DEBUG == 0

#define LOG_DBG(fmt, ...) ((void)fmt)/* inactive in release builds (NDEBUG == 1) */

#define ELOG_DBG(fmt, ...) ((void)fmt) /* inactive in release builds (NDEBUG == 1) */

#define SLOG_DBG(slice) ((void)slice)/* inactive in release builds (NDEBUG == 1) */

#define SELOG_DBG(slice) ((void)slice)/* inactive in release builds (NDEBUG == 1) */

#else

#define LOG_DBG(fmt, ...) (println(fmt __VA_OPT__(,) __VA_ARGS__))
#define ELOG_DBG(fmt, ...) (eprintln(fmt __VA_OPT__(,) __VA_ARGS__))

#define SLOG_DBG(slice) (sprintln((slice)))
#define SELOG_DBG(slice) (seprintln((slice)))

#endif

#define LOG_CTX(CTX_NAME, _fmt, ...) (println(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(,) __VA_ARGS__)))
#define ELOG_CTX(CTX_NAME, _fmt, ...) (eprintln(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(,) __VA_ARGS__)))

#define LOG(fmt, ...) (LOG_CTX(LIBNV, fmt __VA_OPT__(,) __VA_ARGS__))
#define LOG_INFO(fmt, ...) (LOG_CTX(INFO, fmt __VA_OPT__(,) __VA_ARGS__))
#define LOG_WARN(fmt, ...) (LOG_CTX(!WARNING!, fmt __VA_OPT__(,) __VA_ARGS__))
#define LOG_ERROR(fmt, ...) (ELOG_CTX(!!ERROR!!, fmt __VA_OPT__(,) __VA_ARGS__))


#define SSPREAD(slice) ((slice).begin), ((slice).len)
#define RSSPREAD(slice) ((slice).len), ((slice).begin)

HEDLEY_NO_RETURN
FORMAT_FUNC(1, 2)
void log_fatal(const char* fmt, ...);


HEDLEY_NO_RETURN
void vlog_fatal(const char* fmt, va_list args);

#define EXIT_FATAL(msg) (log_fatal( \
"%s:%d :: FatalError in function: %s\n=> %s",\
__FILE__, __LINE__, __func__, msg))

typedef enum RuntimePanic : i32 {
  Panic__OutOfMemory = -(0x404379),
  Panic__NullPointerUnexpected,
  Panic__ExpectedSomeWhenThereWasNone,
  Panic__ConditionFailure,
  Panic__ExceptionType,
  Panic__UnexpectedProgramState,
  Panic__SystemError,
} RuntimePanic;

HEDLEY_NO_RETURN
void panic_abort(RuntimePanic err);


#define TODO_MSG(_msg, ...) (log_fatal((_msg) __VA_OPT__(,) __VA_ARGS__))

#define TODO() TODO_MSG("%s: %s @ LINE: %d => Not Yet Implemented!", __FILE__, __func__, __LINE__)

