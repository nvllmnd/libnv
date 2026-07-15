// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdarg.h>
#include <stdio.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"

#include "nv/memory/error.h"

#include "nv/core/sslice.h"

#ifndef __cplusplus


#define SLOG_DBG(slice) (sprintln((slice)))
#define SELOG_DBG(slice) (seprintln((slice)))

PARAMS_NONNULL(1)
void sfprint(FILE* fd, sslice str);

/// Same as [sfprint], but appends a newline character to the end of
/// give string [sslice]
///
/// Prints a given string [sslice] to
/// a file. This is a verstion of [print_fd] that does not require
/// null-terminated strings. However this function does not
/// do any formatting. If you need to print a formatted string. see [print_fd] and others
///
PARAMS_NONNULL(1)
void sfprintln(FILE* fd, sslice str);

void sprint(sslice str);

void sprintln(sslice str);

void seprint(sslice str);

void seprintln(sslice str);

#endif

BEGIN_C_DECLS

typedef enum FormatError { Format__Error = -1, Format__Ok = 0 } FormatError;

FORMAT_FUNC(3, 4)
RETURNS_ERROR
FormatError format_with(char* dst, isize dst_len, const char* fmt, ...);

#define print_fd(fd, fmt, ...) (fprintf(fd, fmt __VA_OPT__(, ) __VA_ARGS__))
#define println_fd(fd, fmt, ...) (print_fd(fd, fmt "\n" __VA_OPT__(, ) __VA_ARGS__))

#ifndef NV_LOG_FILE
#define NV_LOG_FILE 0
#endif

#if NV_LOG_FILE == 1

extern FILE* NV_LOG_STREAM;
extern FILE* NV_ERR_STREAM;

#define eprint(fmt, ...) (fprintf(NV_ERR_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))
#define eprintln(fmt, ...) (println_fd(NV_ERR_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))


#if defined(__cplusplus) && __cplusplus >= 202207L

#define PRINT(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(, ) __VA_ARGS__))
#define PRINTLN(fmt, ...) (println_fd(stdout, fmt, __VA_ARGS__))

#else

#define print(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(, ) __VA_ARGS__))
#define println(fmt, ...) (println_fd(stdout, fmt, __VA_ARGS__))

#endif

#define println(fmt, ...) (println_fd(NV_LOG_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))
#define print(fmt, ...) (print_fd(NV_LOG_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))

#else

#define eprint(fmt, ...) (fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__))
#define eprintln(fmt, ...) (println_fd(stderr, fmt, __VA_ARGS__))

#if defined(__cplusplus) && __cplusplus >= 202207L


#define PRINT(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(, ) __VA_ARGS__))
#define PRINTLN(fmt, ...) (println_fd(stdout, fmt, __VA_ARGS__))

#else

#define print(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(, ) __VA_ARGS__))
#define println(fmt, ...) (println_fd(stdout, fmt, __VA_ARGS__))

#endif



#endif

/// @brief print a formatted string message,alongside [strerror] and aborts the program
/// @details also prints a stack trace if debug build (LIBNV_DEBUG == 1) and LIBNV_TRACE_ON_ABORT is defined to a
/// non-zero value.
/// @returns does not return
HEDLEY_NO_RETURN
FORMAT_FUNC(1, 2)
void log_fatal(const char* fmt, ...);

HEDLEY_NO_RETURN
void vlog_fatal(const char* fmt, va_list args);

/// @brief uses backtrace in execinfo.h,
/// @details i dont think this is available on musl...
/// @param (i32 depth) number of function calls that will be printed. if there are any more they are truncated and not
/// reported
/// @returns this function calls [backtrace_symbols], which mallocs some strings
/// containing stack call information. so if any of those calls fail, an error is returned
NvError print_stack_trace(i32 depth);
/// Prints a given string [sslice] to
/// a file. This is a verstion of [print_fd] that does not require
/// null-terminated strings. However this function does not
/// do any formatting. If you need to print a formatted string. see [print_fd] and others
///
/// @brief similar to libnv's other logging macros, but includes [strerror] output
/// You can think of this function as a [perror] that formats a string message
void print_error(const char* fmt, ...);
/// @see [print_error]
void vprint_error(const char* fmt, va_list args);

#define FILE_FMT "%s[%s::%s]:%d => "
#define FILE_FMT_ARGS(CTX_NAME, ...) __FILE__, STRINGIFY(CTX_NAME), __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__

#ifndef LIBNV_SUPPRESS_RUNTIME_ERROR_LOG
#define LIBNV_SUPPRESS_RUNTIME_ERROR_LOG 0
#endif

#define LOG_CTX(CTX_NAME, _fmt, ...) (print_error(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(, ) __VA_ARGS__)))
#define ELOG_CTX(CTX_NAME, _fmt, ...) (print_error(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(, ) __VA_ARGS__)))

#define NVERROR(_fmt, ...) (eprintln(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(, ) __VA_ARGS__)))

#ifdef NDEBUG

#undef LIBNV_DEBUG
#define LIBNV_DEBUG 0

#ifndef LIBNV_VERBOSE_LOGGING
#define LIBNV_VERBOSE_LOGGING 0
#endif

#else

#undef LIBNV_DEBUG
#define LIBNV_DEBUG 1

#ifndef LIBNV_VERBOSE_LOGGING
#define LIBNV_VERBOSE_LOGGING 1
#endif

#endif

#if LIBNV_DEBUG == 0

#define LOG_DBG(fmt, ...)
#define ELOG_DBG(fmt, ...)

#define DERROR(_fmt, ...)
#define SLOG_DBG(slice)
#define SELOG_DBG(slice)
#define LOG_WARN(fmt, ...)

#define LOG_ERROR(fmt, ...)

#define LOG_FATAL(fmt, ...) (log_fatal(fmt __VA_OPT__(, ) __VA_ARGS__))

#define PERROR_FATAL() (LOG_FATAL(""))

#define DNVERROR(_fmt, ...)

#define DWARN(...)

#define DERR(...)

#else

#define LOG_DBG(fmt, ...) (LOG_CTX([[DEBUG]], fmt __VA_OPT__(, ) __VA_ARGS__))
#define ELOG_DBG(fmt, ...) (LOG_CTX([[DEBUG]], fmt __VA_OPT__(, ) __VA_ARGS__))

#define LOG_WARN(fmt, ...) (LOG_CTX([[WARN]], fmt __VA_OPT__(, ) __VA_ARGS__))

#define DERROR(_fmt, ...) (LOG_CTX([[ERROR]], _fmt __VA_OPT__(, ) __VA_ARGS__))

#define LOG_ERROR(fmt, ...) (ELOG_CTX([[ERROR]], fmt __VA_OPT__(, ) __VA_ARGS__))

#define DWARN LOG_WARN
#define DERR DERROR

#define LOG_FATAL(fmt, ...) (log_fatal(fmt __VA_OPT__(, ) __VA_ARGS__))

#define PERROR_FATAL() (LOG_FATAL(""))

#define DNVERROR(_fmt, ...) (NVERROR(_fmt __VA_OPT__(, ) __VA_ARGS__))

#endif

#define LOG(fmt, ...) (println(fmt __VA_OPT__(, ) __VA_ARGS__))

END_C_DECLS
