// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if LIBNV_USE_GLOBAL_INTDEFS == 0
namespace nv {
#endif

using u8 = uint8_t;
using i8 = int8_t;
using byte = u8;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;
using usize = size_t;
using isize = ssize_t;
using f32 = float;
using f64 = double;
using f128 = long double;

static constexpr i8 I8_MAX = INT8_MAX;
static constexpr i8 I8_MIN = INT8_MIN;
static constexpr u8 U8_MAX = UINT8_MAX;
static constexpr i16 I16_MAX = INT16_MAX;
static constexpr u16 U16_MAX = UINT16_MAX;

static constexpr i32 I32_MIN = INT32_MIN;
static constexpr i32 I32_MAX = INT32_MAX;
static constexpr u32 U32_MAX = UINT32_MAX;
static constexpr i64 I64_MIN = INT64_MIN;
static constexpr i64 I64_MAX = INT64_MAX;
static constexpr u64 U64_MAX = UINT64_MAX;

static constexpr isize ISIZE_MIN = PTRDIFF_MIN;
static constexpr isize ISIZE_MAX = PTRDIFF_MAX;

static constexpr usize USIZE_MAX = SIZE_MAX;

#if LIBNV_USE_GLOBAL_INTDEFS == 0
}  // namespace nv
#endif
