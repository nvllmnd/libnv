// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nv/core/log.h"

#include <asm-generic/errno-base.h>
#include <err.h>
#include <errno.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "nv/core/algo.h"
#include "nv/core/stb_sprintf.h"

FormatError format_with(char* dst, isize len, const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const i32 err = stbsp_vsnprintf(dst, len, fmt, args);

  va_end(args);

  if (UNLIKELY(err == -1)) {
    return Format__Error;
  }

  return Format__Ok;
}

void sfprint(FILE* fd, sslice str) { fprintf(fd, "%.*s", RSSPREAD(str)); }

void sfprintln(FILE* fd, sslice str) { fprintf(fd, "%.*s\n", RSSPREAD(str)); }

void sprint(sslice str) { print("%.*s", RSSPREAD(str)); }

void sprintln(sslice str) { printf("%.*s\n", RSSPREAD(str)); }

void seprint(sslice str) { fprintf(stderr, "%.*s", RSSPREAD(str)); }

void seprintln(sslice str) { fprintf(stderr, "%.*s\n", RSSPREAD(str)); }

// TODO: Add this macro (and a few others) as meson options so users of libnv can
// more easily configure these build options
#ifndef LIBNV_TRACE_ON_ABORT
#define LIBNV_TRACE_ON_ABORT 1
#endif

#ifndef LIBNV_STACK_TRACE_DEPTH
#define LIBNV_STACK_TRACE_DEPTH 100
#endif

NvError print_stack_trace(const i32 depth) {
  const error err = errno;
  const i32 max_depth = min(depth, LIBNV_STACK_TRACE_DEPTH);

  void* tsym_storage[max_depth] = {};
  char** tsyms = (char**)tsym_storage;

  const i64 tsym_len = backtrace(tsym_storage, LIBNV_STACK_TRACE_DEPTH);

  tsyms = backtrace_symbols(tsym_storage, tsym_len);
  if UNLIKELY (is_null(tsyms)) {
    errno = err;
    perror("backtrace_symbols returned nullptr!");
    return ERROR;
  }

  eprintln("##### Stack Trace ####");
  for (i32 i = 0; i < tsym_len; i++) {
    eprintln("in function: %s", tsyms[i]);
  }
  eprintln("#### End Stack Trace ####");
  free(tsyms);

  return NVOK;
}

static inline NvError trace_abort(void) {
#if LIBNV_TRACE_ON_ABORT == 1
  return print_stack_trace(LIBNV_STACK_TRACE_DEPTH);
#else
  return NVOK;
#endif
}

// TODO: Maybe i should have a means of aborting execution that doesnt allocate at all to compliment this one?
// see below note
void vlog_fatal(const char* fmt, va_list args) {
  // cache the original error, we call library functions that may call other
  // library functions that may set errno
  const error err = errno;

  if UNLIKELY (trace_abort() != NVOK) {
    eprintln("#### Failed to print stack trace! ####");
  }

  errno = err;
  vwarn(fmt, args);

  va_end(args);

  EXIT_FATAL();
}

void log_fatal(const char* fmt, ...) {
  va_list args;
  va_start(args);

  vlog_fatal(fmt, args);
}

/// @brief similar to libnv's other logging macros, but includes [strerror] output
void print_error(const char* fmt, ...) {
  const error err = errno;

  va_list args = {};
  va_start(args);

  errno = err;
  vprint_error(fmt, args);

  va_end(args);
}
/// @see [print_error]
void vprint_error(const char* fmt, va_list args) {
  if (errno == 0) {
    vwarnx(fmt, args);
  } else {
    vwarn(fmt, args);
  }
}
