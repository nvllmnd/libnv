#include "nv/core/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


FormatError format_with(char* dst, isize len, const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  const i32 err = vsnprintf(dst, len, fmt, args);

  va_end(args);

  if (UNLIKELY(err == -1)) {
    return Format__Error;
  }

  return Format__Ok;
}

cstr format_string(const char* fmt, ...) {
  va_list args = {};
  va_start(args);

  // we need to copy va_list, since the first call to vsnprintf
  // exhausts the entire list, so if we dont copy here, the second call to
  // vsnprintf causes a segfault at runtime
  va_list args_len = {};
  va_copy(args_len, args);

  const i32 len = vsnprintf(nullptr, 0, fmt, args_len) + 1;
  va_end(args_len);

  if (UNLIKELY(len == 0)) {
    va_end(args);
    return cstr_empty();
  }

  char buf[len] = {};

  const i32 err = vsnprintf(buf, len, fmt, args);

  va_end(args);

  if (UNLIKELY(err == -1)) {
    return cstr_empty();
  }

  static constexpr const i32 SMALL_SIZE = cast(i32, SMALL_BUF_SIZE);

  if (UNLIKELY(len <= SMALL_SIZE)) {
    return cstr_small_new(buf);
  }

  return cstr_new(buf);
}

void sfprint(FILE* fd, sslice str) { fprintf(fd, "%.*s", RSSPREAD(str)); }

void sfprintln(FILE* fd, sslice str) { fprintf(fd, "%.*s\n", RSSPREAD(str)); }

void sprint(sslice str) { print("%.*s", RSSPREAD(str)); }

void sprintln(sslice str) { printf("%.*s\n", RSSPREAD(str)); }

void seprint(sslice str) { fprintf(stderr, "%.*s", RSSPREAD(str)); }

void seprintln(sslice str) { fprintf(stderr, "%.*s\n", RSSPREAD(str)); }

void vlog_fatal(const char* fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);

  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");

  va_end(copy);

  exit(1);
}
void log_fatal(const char* fmt, ...) {
  va_list args;
  va_start(args);

  vlog_fatal(fmt, args);
}

void panic_abort(RuntimePanic err) {
  const char* s = nullptr;

  switch (err) {
    case Panic__OutOfMemory: {
      s = STRINGIFY(Panic__OutOfMemory);
    } break;
    case Panic__NullPointerUnexpected: {
      s = STRINGIFY(Panic__NullPointerUnexpected);
    } break;
    case Panic__ExpectedSomeWhenThereWasNone: {
      s = STRINGIFY(Panic__ExpectedSomeWhenThereWasNone);
    } break;
    case Panic__ConditionFailure: {
      s = STRINGIFY(Panic__ConditionFailure);
    } break;
    case Panic__ExceptionType: {
      s = STRINGIFY(Panic__ExceptionType);
    } break;
    case Panic__UnexpectedProgramState: {
      s = STRINGIFY(Panic__UnexpectedProgramState);
    } break;
    case Panic__SystemError:
      [[fallthrough]];
    default: {
      s = STRINGIFY(Panic__SystemError);
    } break;
  }

  log_fatal("%s", s);
}
