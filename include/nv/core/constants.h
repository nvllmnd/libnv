// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "attributes.h"
#include "intdefs.h"

BEGIN_C_DECLS

static constexpr const i64 KB1 = 1024L;

#define KILOBYTES(n) (KB1 * (n))

#define MEGABYTES(n) (KILOBYTES((n)) * KB1)

#define GIGABYTES(n) (MEGABYTES((n)) * KB1)

#define TERABYTES(n) (GIGABYTES((n)) * KB1)

#define KB KILOBYTES

#define MB MEGABYTES

#define GB GIGABYTES

#define TB TERABYTES

#define OF_KILOBYTES(_bytes)                                                                                      \
  ({                                                                                                              \
    static_assert(HEDLEY_IS_CONSTANT((_bytes)),                                                                   \
                  "OF_* macros can only be invoked with a constant expression! use of_* functions instead!");     \
    static_assert((_bytes) >= KB(1),                                                                              \
                  "Value passed to OF_* macros must be greater than or equal to the minimum lookup limit! (e.g. " \
                  "greater than 1GB for OF_GIGABYTES");                                                           \
    static constexpr const auto _v = (_bytes);                                                                    \
    _v / KB1;                                                                                                     \
  })

#define OF_MEGABYTES(_bytes) (OF_KILOBYTES((_bytes) / KB1))
#define OF_GIGABYTES(_bytes) (OF_MEGABYTES((_bytes) / KB1))
#define OF_TERABYTES(_bytes) (OF_GIGABYTES((_bytes) / KB1))

#define OF_KB OF_KILOBYTES
#define OF_MB OF_MEGABYTES
#define OF_GB OF_GIGABYTES
#define OF_TB OF_TERABYTES

CONST_FUNC
static inline i64 of_kilobytes(i64 n) { return n / 1024; }

CONST_FUNC
static inline i64 of_megabytes(i64 n) { return of_kilobytes(n / 1024); }

CONST_FUNC
static inline i64 of_gigabytes(i64 n) { return of_megabytes(n / 1024); }

CONST_FUNC
static inline i64 of_terabytes(i64 n) { return of_gigabytes(n / 1024); }

CONST_FUNC static inline i64 kilobytes(i64 n) { return KILOBYTES(n); }

CONST_FUNC
static inline i64 megabytes(i64 n) { return MEGABYTES(n); }

CONST_FUNC
static inline i64 gigabytes(i64 n) { return GIGABYTES(n); }

CONST_FUNC
static inline i64 terabytes(i64 n) { return TERABYTES(n); }

END_C_DECLS
