#include "core/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "memory/cstr.h"
#include "stdarg.h"

static inline void fdwrite(i32 fd, const char* src, isize len) {
  i32 nbytes = 0;


  while (nbytes < len) {
    const void* start = &src[nbytes];
    const isize remaining = len - nbytes;
    const i32 n = write(fd, start, remaining);
    if (n == -1) {
      perror("POSIX write returned -1 while attempting to write to Stdout Stream!");
      return;
    }

    nbytes += n;
  }
}

static inline void stdout_write(const char* src, isize len) { fdwrite(STDOUT_FILENO, src, len); }

static inline void stderr_write(const char* src, isize len) { fdwrite(STDERR_FILENO, src, len); }

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
  // exhausts the entire list, so if we dont copy here, the second call to vsnprintf
  // causes a segfault at runtime
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

void sfprint(FILE* fd, sslice str) {
  const void* src = pcast(void, str.begin);
  const isize len = str.len;
  fwrite(src, len, 1, fd);
  fflush(fd);
}

static constexpr const i32 NL_SIZE = sizeof("\n");
static constexpr const char NL[] = "\n";

void sfprintln(FILE* fd, sslice str) {
  const void* src = pcast(void, str.begin);
  const isize len = str.len;
  fwrite(src, len, 1, fd);
  fwrite(NL, NL_SIZE, 1, fd);
  fflush(fd);
}

void sprint(sslice str) { stdout_write(str.begin, str.len); }

void sprintln(sslice str) {
  stdout_write(str.begin, str.len);
  stdout_write(NL, NL_SIZE);
}

void seprint(sslice str) { stderr_write(str.begin, str.len); }

void seprintln(sslice str) {
  stderr_write(str.begin, str.len);
  stderr_write(NL, NL_SIZE);
}


void log_fatal(const char* fmt, ...) {
 va_list args; 
 va_start(args);


 vfprintf(stderr, fmt, args);

 va_end(args);
 
  exit(1); 
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
    case Panic__SystemError: [[fallthrough]];
    default: {
      s =  STRINGIFY(Panic__SystemError);
    } break;
  }

  log_fatal("%s", s);
}

