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

  EXIT_FATAL();
}
void log_fatal(const char* fmt, ...) {
  va_list args;
  va_start(args);

  vlog_fatal(fmt, args);
}


