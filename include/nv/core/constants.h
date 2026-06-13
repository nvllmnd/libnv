// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "attributes.h"
#include "intdefs.h"

static constexpr const i64 KB1 = 1024L;

#define KILOBYTES(n) (KB1 * (n))

#define MEGABYTES(n) (KILOBYTES((n)) * KB1)

#define GIGABYTES(n) (MEGABYTES((n)) * KB1)

#define TERABYTES(n) (GIGABYTES((n)) * KB1)

#define KB(_n) KILOBYTES(_n)

#define MB(_n) MEGABYTES(_n)

#define GB(_n) GIGABYTES(_n)

#define TB(_n) TERABYTES(_n)


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
