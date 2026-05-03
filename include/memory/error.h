#pragma once


#include <errno.h>
typedef enum MemError {

  /// Ok, No Error!
  MemError__Ok = 0,
  /// Not enough space/cannot allocate memory (POSIX.1-2001).
  MemError__OOM = ENOMEM,

} MemError;
