#pragma once


#include <string.h>
#include "nv/core/intdefs.h"

#define Slice(T) struct { const __typeof(T)* begin; i64 len;  }

typedef Slice(char) SliceChar;
typedef Slice(u8) SliceBytes;
typedef Slice(i16) SliceInt16;
typedef Slice(u16) SliceUInt16;
typedef Slice(i32) SliceInt32;
typedef Slice(u32) SliceUInt32;
typedef Slice(i64) SliceInt64;
typedef Slice(u64) SliceUInt64;
typedef Slice(f32) SliceFloat32;
typedef Slice(f64) SliceFloat64;



#define slice_new(T, ...) ((Slice(T)){ __VA_ARGS__ })
#define slice_empty(T) (slice_new(T, .begin = nullptr, .len = 0))




#define sslice_static_new(static_str)                                      \
  /* Creates a new instance of [sslice] on the stack that points to string \
   literals, which reside in constant static readonly memory*/             \
  (sslice_new(.begin = (static_str), .len = (sizeof((static_str)) - 1))) /* - 1 so we dont include the null-terminating byte*/


#define sslice_empty() (sslice_new())  


#define slice_is_empty(_self) ({\
  const auto _s = &(_self);\
  _s->begin == nullptr && _s->len <= 0;\
})

#define slice_from_range(_base_ptr, _base_len, _from, _to) ({\
  const auto _bl = (_base_len);\
  const auto _f = (_from);\
  const auto _t = (_to);\
  const auto _len = (_t) - (_f); \
  Slice(__typeof((_base_ptr))) _res = {}; \
  if (_len < _bl && _len >= 0) { \
     auto _begin = &(_base_ptr)[_f]\
     _res = slice_new(.begin = _begin, .len = _len);\
  }\
  _res\
})

#define slice_cmp(_lhs, _rhs) ({\
  const auto _llen = (_lhs).len;\
  const auto _rlen = (_rhs).len;\
  bool iseq = false;\
  if (_llen == _rlen) {\
    const i64 _size = sizeof(__typeof(*(_lhs).begin)) * _llen; \
    iseq = memcmp((_lhs).begin, (_rhs).begin, _size); \
  }\
  iseq;\
})


#define slice_eq(_lhs, _rhs) (slice_cmp(_lhs, _rhs) == 0)

