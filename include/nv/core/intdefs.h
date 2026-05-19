#pragma once

#include <bits/floatn.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint8_t byte;
typedef uint16_t u16;
typedef int16_t i16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef size_t usize;
typedef ptrdiff_t isize;
/// AKA index :: same as `std::size_t` or `usize`
typedef usize index_t;
/// AKA: pointer offset :: same as `std::ptrdiff_t` or `isize`
typedef isize poffset_t;
typedef uintptr_t u64ptr;

typedef uintptr_t addr;

typedef float f32;
typedef double f64;
typedef _Float128 f128;
typedef long double ldouble;

typedef typeof(void *) voidptr;

/// type alias to help clarify functions that return errors.
/// also for setting the undlying type of an enum to : error.
/// Unless returned value is an enum, usually a value of 0 means that
/// no error has occured. positive values could also indicate success, but check
/// each funcitons documentation specifics. Negative numbers usually correlate
/// to some error code, assuming that numbers >= 0 are not errors
typedef i32 error;

#define bint(N) _BitInt(N)
#define ubint(N) unsigned bint(N)

static constexpr const i8 I8_MAX = INT8_MAX;
static constexpr const i8 I8_MIN = INT8_MIN;
static constexpr const u8 U8_MAX = UINT8_MAX;
static constexpr const i16 I16_MAX = INT16_MAX;
static constexpr const u16 U16_MAX = UINT16_MAX;

static constexpr const i32 I32_MIN = INT32_MIN;
static constexpr const i32 I32_MAX = INT32_MAX;
static constexpr const u32 U32_MAX = UINT32_MAX;
static constexpr const i64 I64_MIN = INT64_MIN;
static constexpr const i64 I64_MAX = INT64_MAX;
static constexpr const u64 U64_MAX = UINT64_MAX;

static constexpr const isize ISIZE_MIN = PTRDIFF_MIN;
static constexpr const isize ISIZE_MAX = PTRDIFF_MAX;

static constexpr const usize USIZE_MAX = SIZE_MAX;
