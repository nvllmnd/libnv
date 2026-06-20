// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "nv/core/log.h"

#ifndef LIBNV_DEBUG

#ifdef NDEBUG
#define LIBNV_DEBUG 0
#else
#define LIBNV_DEBUG 1
#endif  //  NDEBUG
#endif  // LIBNV_DEBUG

#if LIBNV_DEBUG == 1

#include <assert.h>

#define IF_DEBUG(x) x
#define IF_RELEASE(x)
#define ASSERT_PTR(_p) assert((_p))

#define assert_debug assert

#else

#include "nv/core/algo.h"

#define IF_DEBUG(x)
#define IF_RELEASE(x) x
#define ASSERT_PTR(_p) punwrap(_p)

#define assert_debug

#endif  // if LIBNV_DEBUG == 1

#include "hedley.h"

#if HEDLEY_HAS_BUILTIN(__builtin_trap)
#ifndef EXIT_FATAL
#define EXIT_FATAL() __builtin_trap()
#endif  // ifndef EXIT_FATAL
#ifndef EXIT_FATAL
#define EXIT_FATAL() abort()
#endif  // ifndef EXIT_FATAL
#endif  // HEDLEY_HAS_BUILTIN(__builtin_trap)

#define TODO_MSG(_msg, ...) (log_fatal((_msg)__VA_OPT__(, ) __VA_ARGS__))

#define TODO() TODO_MSG("%s: %s @ LINE: %d => Not Yet Implemented!", __FILE__, __func__, __LINE__)

#define TODO_FN(_ret, _fn_name, ...) \
  _ret _fn_name(__VA_ARGS__) { TODO(); }

#define expectm(x, _msg, ...)                      \
  do {                                             \
    const bool _res = (x);                         \
    if UNLIKELY (!_res) {                          \
      LOG_FATAL((_msg)__VA_OPT__(, ) __VA_ARGS__); \
    }                                              \
  } while (0)

#define expect(x) expectm(x, "Expression: " #x " should evaluate to true!! Aborting!")




