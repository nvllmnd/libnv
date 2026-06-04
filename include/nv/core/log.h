#pragma once

#include <stdarg.h>
#include <stdio.h>

#include "nv/core/attributes.h"
#include "nv/core/debug.h"
#include "nv/core/intdefs.h"
#include "nv/core/sslice.h"

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

#define print(fmt, ...) (print_fd(NV_LOG_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))
#define eprint(fmt, ...) (fprintf(NV_ERR_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))
#define println(fmt, ...) (println_fd(NV_LOG_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))
#define eprintln(fmt, ...) (println_fd(NV_ERR_STREAM, fmt __VA_OPT__(, ) __VA_ARGS__))

#else

#define print(fmt, ...) (fprintf(stdout, fmt __VA_OPT__(, ) __VA_ARGS__))
#define eprint(fmt, ...) (fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__))
#define println(fmt, ...) (println_fd(stdout, fmt, __VA_ARGS__))
#define eprintln(fmt, ...) (println_fd(stderr, fmt, __VA_ARGS__))

#endif

/// Prints a given string [sslice] to
/// a file. This is a verstion of [print_fd] that does not require
/// null-terminated strings. However this function does not
/// do any formatting. If you need to print a formatted string. see [print_fd] and others
///
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

#define FILE_FMT "%s[%s::%s]:%d => "
#define FILE_FMT_ARGS(CTX_NAME, ...) __FILE__, STRINGIFY(CTX_NAME), __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__

#if LIBNV_DEBUG == 0

#define LOG_DBG(fmt, ...) ((void)fmt) /* inactive in release builds (NDEBUG == 1) */

#define ELOG_DBG(fmt, ...) ((void)fmt) /* inactive in release builds (NDEBUG == 1) */

#define SLOG_DBG(slice) ((void)slice) /* inactive in release builds (NDEBUG == 1) */

#define SELOG_DBG(slice) ((void)slice) /* inactive in release builds (NDEBUG == 1) */

#else

#define LOG_DBG(fmt, ...) (println(fmt __VA_OPT__(, ) __VA_ARGS__))
#define ELOG_DBG(fmt, ...) (eprintln(fmt __VA_OPT__(, ) __VA_ARGS__))

#define SLOG_DBG(slice) (sprintln((slice)))
#define SELOG_DBG(slice) (seprintln((slice)))

#endif

#define LOG_CTX(CTX_NAME, _fmt, ...) (println(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(, ) __VA_ARGS__)))
#define ELOG_CTX(CTX_NAME, _fmt, ...) (eprintln(FILE_FMT _fmt, FILE_FMT_ARGS(CTX_NAME __VA_OPT__(, ) __VA_ARGS__)))

#define LOG(fmt, ...) (LOG_CTX(LIBNV, fmt __VA_OPT__(, ) __VA_ARGS__))
#define LOG_INFO(fmt, ...) (LOG_CTX(INFO, fmt __VA_OPT__(, ) __VA_ARGS__))
#define LOG_WARN(fmt, ...) (LOG_CTX(!WARNING !, fmt __VA_OPT__(, ) __VA_ARGS__))
#define LOG_ERROR(fmt, ...) (ELOG_CTX(!!ERROR !!, fmt __VA_OPT__(, ) __VA_ARGS__))

#define SSPREAD(slice) ((slice).begin), ((slice).len)
#define RSSPREAD(slice) ((slice).len), ((slice).begin)

HEDLEY_NO_RETURN
FORMAT_FUNC(1, 2)
void log_fatal(const char* fmt, ...);

HEDLEY_NO_RETURN
void vlog_fatal(const char* fmt, va_list args);

#define LOG_FATAL(fmt, ...) (log_fatal(FILE_FMT fmt, FILE_FMT_ARGS(!!FATAL !!__VA_OPT__(, ) __VA_ARGS__)))

#define TODO_MSG(_msg, ...) (log_fatal((_msg)__VA_OPT__(, ) __VA_ARGS__))

#define TODO() TODO_MSG("%s: %s @ LINE: %d => Not Yet Implemented!", __FILE__, __func__, __LINE__)
