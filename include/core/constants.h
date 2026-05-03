#pragma once

#include "attributes.h"
#include "intdefs.h"

static constexpr const i32 KB1 = 1024;

#define KILOBYTES(n) (KB1 * (n))

#define MEGABYTES(n) (KILOBYTES((n)) * KB1)

#define GIGABYTES(n) (MEGABYTES((n)) * KB1)

#define TERABYTES(n) (GIGABYTES((n)) * KB1)

CONST_FUNC
static inline u64 kilobytes(isize n) {
  return KILOBYTES(n);
}

CONST_FUNC
static inline u64 megabytes(isize n) {
  return MEGABYTES(n);
}

CONST_FUNC
static inline u64 gigabytes(isize n) {
  return GIGABYTES(n);
}

CONST_FUNC
static inline u64 terabytes(isize n) {
  return TERABYTES(n);
}
