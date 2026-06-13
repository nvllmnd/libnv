// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


#include "nv/core/intdefs.h"

#define Array(T, N)      \
  struct {               \
    __typeof(T) data[N]; \
    i64 len;             \
  }

#define BoolArray(N) Array(bool, N)
#define CharArray(N) Array(char, N)
#define ByteArray(N) Array(u8, N)
#define IntArray(N) Array(int, N)
#define Int16Array(N) Array(i16, N)
#define UInt16Array(N) Array(u16, N)
#define UIntArray(N) Array(u32, N)
#define Int64Array(N) Array(i64, N)
#define UInt64Array(N) Array(u64, N)
#define FloatArray(N) Array(f32, N)
#define Float64Array(N) Array(f64, N)

#define arr_new(T, N) ((Array(T, N)){.data = {}, .len = 0})

#define arr_elem_typeof(_self) __typeof((_self).data[0])

#define arr_elem_size(_self) (sizeof(arr_elem_typeof(_self)))

#define arr_size_bytes(_self) (sizeof(__typeof((_self).data)))

#define arr_capacity(_self) (arr_size_bytes(_self) / arr_elem_size(_self))

#define arr_push(_self, _v)                                                     \
  ({                                                                            \
    const auto _val = (_v);                                                     \
    static_assert(sizeof(__typeof(_val)) == sizeof(__typeof((_self).data[0]))); \
    bool success = false;                                                       \
    if ((_self).len < (i64)arr_capacity(_self)) {                               \
      (_self).data[(_self).len] = _val;                                         \
      (_self).len += 1;                                                         \
      success = true;                                                           \
    }                                                                           \
    success;                                                                    \
  })

#define arr_pop(_self)                      \
  ({                                        \
    arr_elem_typeof(_self) _val = {};       \
    if ((_self).len >= 1) {                 \
      _val = (_self).data[(_self).len - 1]; \
      (_self).len -= 1;                     \
    }                                       \
    _val;                                   \
  })

#define arr_wrapping_push(_self, _v)                                            \
  ({                                                                            \
    const auto _val = (_v);                                                     \
    static_assert(sizeof(__typeof(_val)) == sizeof(__typeof((_self).data[0]))); \
    (_self).data[(_self).len % arr_capacity(_self) - 1] = _val;                 \
    (_self).len += 1;                                                           \
  })

#define arr_index(_self, _i)                                             \
  ({                                                                     \
    arr_elem_typeof(_self)* _val = nullptr;                              \
    const auto _index = (_i);                                            \
    if (_index >= 0 && _index < (__typeof(_index))arr_capacity(_self)) { \
      _val = &(_self).data[_index];                                      \
    }                                                                    \
    _val;                                                                \
  })

#define arr_cindex(_self, _i)                                   \
  ({                                                            \
    static constexpr const auto _INDEX = (_i);                  \
    static_assert(_INDEX >= 0 && _INDEX < arr_capacity(_self)); \
    auto _val = &(_self).data[_INDEX];                          \
    _val;                                                       \
  })

#define arr_set(_self, _i, _v)                         \
  ({                                                   \
    const auto _index = (_i);                          \
    const auto _val = (_v);                            \
    bool success = false;                              \
    if (_index >= 0 && _index < arr_capacity(_self)) { \
      (_self).data[_index] = _val;                     \
      success = true;                                  \
    }                                                  \
    success;                                           \
  })

#define arr_cset(_self, _i, _v)                                 \
  ({                                                            \
    static constexpr const auto _INDEX = (_i);                  \
    static_assert(_INDEX >= 0 && _INDEX < arr_capacity(_self)); \
    (_self).data[_INDEX] = (_v);                                \
  })

#define arr_clear(_self) ((_self).len = 0)

#define arr_clear_zeroed(_self) ({ memset(&(_self), 0, sizeof((_self))); })

// TODO: Write tests for this module!
