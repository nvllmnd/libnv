#pragma once

#include "attributes.h"
#include "intdefs.h"

static constexpr const i32 KB1 = 1024;

#define KILOBYTES(n) (KB1 * (n))

#define MEGABYTES(n) (KILOBYTES((n)) * KB1)

#define GIGABYTES(n) (MEGABYTES((n)) * KB1)

#define TERABYTES(n) (GIGABYTES((n)) * KB1)

#define KB(_n) ({static_assert(HEDLEY_IS_CONSTANT(_n), "Cannot use KB Macro with non-literal! use KILOBTYES/kilobytes instead!"); KILOBYTES(_n##L);})

#define MB(_n) ({static_assert(HEDLEY_IS_CONSTANT(_n), "Cannot use MB Macro with non-literal! use MEGABYTES/megabytes instead!"); MEGABYTES(_n##L);})

#define GB(_n) ({static_assert(HEDLEY_IS_CONSTANT(_n), "Cannot use GB Macro with non-literal! use GIGABYTES/gigabytes instead!"); GIGABYTES(_n##L);})

#define TB(_n) ({static_assert(HEDLEY_IS_CONSTANT(_n), "Cannot use TB Macro with non-literal! use TERABYTES/terabytes instead!"); GIGABYTES(_n##L);})


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
