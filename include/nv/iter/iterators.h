// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

//! This module contains generic types and macro functions for iterators and spans (slice-like object similar to C++'s
//! std::span)
//! The Span and Iter types just wrap SpanData and IterData respectively, so you can extend
//! these types to make your own span-like or iter-like types. Three is also SPANLIKE and ITERLIKE macros
//! which you can use to 'spread' an Iter or Span instance fields over an array or function arguments, kind of like
//! javascripts ... spread operator
#pragma once

#include <stddef.h>
#include "nv/core/ctypes.h"

#define IterData(T) \
  SpanData(T);      \
  __typeof(T)* cursor
#define CIterData(T) \
  CSpanData(T);      \
  const __typeof(T)* cursor

#define SpanData(T)   \
  __typeof(T)* begin; \
  __typeof(T)* end

#define CSpanData(T)        \
  const __typeof(T)* begin; \
  const __typeof(T)* end

#define Span(T)  \
  struct {       \
    SpanData(T); \
  }
#define CSpan(T)  \
  struct {        \
    CSpanData(T); \
  }
#define Iter(T)  \
  struct {       \
    IterData(T); \
  }

#define CIter(T)  \
  struct {        \
    CIterData(T); \
  }

typedef Span(byte) SpanByte;
typedef Span(char) SpanChar;
typedef Span(i16) SpanInt16;
typedef Span(u16) SpanUInt16;
typedef Span(i32) SpanInt32;
typedef Span(u32) SpanUInt32;
typedef Span(i64) SpanInt64;
typedef Span(u64) SpanUInt64;
typedef Span(f32) SpanFloat32;
typedef Span(f64) SpanFloat64;

typedef CSpan(f64) CSpanFloat64;
typedef CSpan(f32) CSpanFloat32;
typedef CSpan(u64) CSpanUInt64;
typedef CSpan(u32) CSpanUInt32;
typedef CSpan(i64) CSpanInt64;
typedef CSpan(i32) CSpanInt32;
typedef CSpan(u16) CSpanUInt16;
typedef CSpan(byte) CSpanByte;
typedef CSpan(i16) CSpanInt16;
typedef CSpan(char) CSpanChar;

typedef Iter(byte) IterByte;
typedef Iter(char) IterChar;
typedef Iter(i16) IterInt16;
typedef Iter(u16) IterUInt16;
typedef Iter(i32) IterInt32;
typedef Iter(u32) IterUInt32;
typedef Iter(i64) IterInt64;
typedef Iter(u64) IterUInt64;
typedef Iter(f32) IterFloat32;
typedef Iter(f64) IterFloat64;

typedef CIter(byte) CIterByte;
typedef CIter(char) CIterChar;
typedef CIter(i16) CIterInt16;
typedef CIter(u16) CIterUInt16;
typedef CIter(i32) CIterInt32;
typedef CIter(u32) CIterUInt32;
typedef CIter(i64) CIterInt64;
typedef CIter(u64) CIterUInt64;
typedef CIter(f32) CIterFloat32;

typedef CIter(f64) CIterFloat64;

#define ITER_NONE(T) ((Iter(T)){})
#define SPAN_NONE(T) ((Span(T)){})

#define SPANLIKE(_s) (_s).begin, (_s).end
#define ITERLIKE(_i) (_i).begin, (_i).cursor, (_i).end

#define span_new(_ptr, _size) ((Span(__typeof(*(_ptr)))){.begin = (_ptr), .end = (_ptr) + (_size)})
#define cspan_new(_ptr, _size) ((CSpan(__typeof(*(_ptr)))){.begin = (_ptr), .end = (_ptr) + (_size)})

#define span_range(_ptr, _from, _to) (span_new(_ptr, (_to) - (_from)))
#define cspan_range(_ptr, _from, _to) (cspan_new(_ptr, (_to) - (_from)))

#define iter_new(_ptr, _size) ((Iter(__typeof(*(_ptr)))){.begin = (_ptr), .end = (_ptr) + (_size), .cursor = (_ptr)})
#define citer_new(_ptr, _size) ((CIter(__typeof(*(_ptr)))){.begin = (_ptr), .end = (_ptr) + (_size), .cursor = (_ptr)})

#define span_is_empty(_it) (memcmp(&(_it), &SPAN_NONE_BYTE, sizeof((_it))) == 0)
#define span_is_ok(_it) (!span_is_empty(_it))

#define spanlen(_span) ((_span).end - (_span).begin)
#define iterlen(_iter) ((_iter).end - (_iter).cursor)
#define itersize(_iter) (spanlen(_iter))

#define span_contains(_iter, _ptr)         \
  ({                                       \
    const auto _it = (_iter);              \
    const byte* _begin = (byte*)_it.begin; \
    const byte* _end = (byte*)_it.end;     \
    const byte* _p = (byte*)(_ptr);        \
    _p && (_p >= _begin && _p < _end);     \
  })

#define span_set(_self, _index, _val)      \
  ({                                       \
    auto _ptr = span_index(_self, _index); \
    if (_ptr) {                            \
      *_ptr = (_val);                      \
    }                                      \
  })

#define span_index(_self, _index)                                  \
  ({                                                               \
    const auto _i = (_index);                                      \
    auto _begin = (_self)->begin;                                  \
    auto _end = (_self)->end;                                      \
    __typeof(_begin) _ptr = nullptr;                               \
    if ((_ptr >= _begin) && (_ptr < _end) && span_is_ok(*_self)) { \
      _ptr = &_begin[_i];                                          \
    }                                                              \
    _ptr;                                                          \
  })

#define subspan(_span, _from, _to)                                          \
  ({                                                                        \
    const auto _f = (_from);                                                \
    const auto _t = (_to);                                                  \
    const auto _delta = _t - _f;                                            \
    auto _result = (__typeof((_span))){};                                   \
    if (_delta >= 0) {                                                      \
      auto _base = (_span).begin + _f;                                      \
      _result = (__typeof(_result)){.begin = _base, .end = _base + _delta}; \
    }                                                                       \
    _result;                                                                \
  })

#define iter_range(_ptr, _from, _to) (iter_new(_ptr, (_to) - (_from)))
#define citer_range(_ptr, _from, _to) (citer_new(_ptr, (_to) - (_from)))

#define iter_is_ok(_it) (!iter_is_empty(_it))

#define iter_tail(_iter) /* Element count between cursor and end */ \
  ({                                                                \
    const auto _it = (_iter);                                       \
    _it.end - _it.cursor;                                           \
  })

#define iter_head(_iter) /* Element count between begin and cursor */ \
  ({                                                                  \
    const auto _it = (_iter);                                         \
    _it.cursor - _it.begin;                                           \
  })

#define iter_first(_iter) ((_iter).begin)
#define iter_last(_iter) ((_iter).end - 1)

#define iter_reset(_iter)     \
  ({                          \
    auto _it = (_iter);       \
    _it->cursor = _it->begin; \
  })

#define iter_at_end(_it)   \
  ({                       \
    const auto _i = (_it); \
    _i.begin >= _i.end;    \
  })

#define iter_next(_iter) ((__typeof((_iter))){.begin = (_iter).begin, .cursor = (_iter).cursor + 1, .end = (_iter).end})

#define iter_write_next(_iter, _val) \
  ({                                 \
    if (!iter_at_end(*_iter)) {      \
      *_iter->begin = (_val);        \
      (_iter) = iter_next(_iter);    \
    }                                \
  })

#define iter_for(_i, _name) for (auto _name = (_i); !iter_at_end(_name); _name = iter_next(_name))

// for (auto _name = _iter.begin; _name < _iter.end; _name++)

#define iter_for_i(_iterator) iter_for(_iterator, i)

#define iter_foreach(_i, _name) \
  auto _iter = (_i);            \
  for (auto _name = _iter.begin; _name < _iter.end; _name++)

#define iter_foreach_i(_iterator) iter_foreach(_iterator, i)

#ifdef __cplusplus

#include <cstddef>

namespace nv {
inline namespace iter {

template <class T>
concept Bufferlike = requires(T it) {
  { it.begin } -> std::convertible_to<typename T::Pointer>;
  { it.end } -> std::convertible_to<typename T::Pointer>;
  { it.cursor } -> std::convertible_to<typename T::Pointer>;
};

template <class T>
struct Iter {
  using DifferenceType = std::ptrdiff_t;
  using ElementType = T;
  using ValueType = std::remove_cv_t<T>;
  using SizeType = usize;
  using Pointer = std::add_pointer_t<ValueType>;
  using ConstPointer = std::add_pointer_t<const ValueType>;
  using Reference = std::add_lvalue_reference_t<ValueType>;
  using ConstReference = std::add_lvalue_reference_t<const ValueType>;

  Pointer begin;
  Pointer end;
  Pointer cursor;

  constexpr bool is_at_end() const noexcept { return this->cursor == this->end; }
};

template <class T>
[[gnu::returns_nonnull]]
constexpr Iter<T>::Pointer begin(const Iter<T>& self) noexcept {
  return self.begin;
}

template <class T>
[[gnu::returns_nonnull]]
constexpr Iter<T>::Pointer end(const Iter<T>& self) noexcept {
  return self.end;
}

template <class T>
[[gnu::returns_nonnull]]
constexpr Iter<const T>::ConstPointer cbegin(const Iter<const T>& self) noexcept {
  return self.begin;
}

template <class T>
[[gnu::returns_nonnull]]
constexpr Iter<const T>::ConstPointer cend(const Iter<const T>& self) noexcept {
  return self.end;
}

template <class T>
[[gnu::nonnull, gnu::returns_nonnull]]
constexpr Iter<T>::Pointer advance(Iter<T>* self, std::ptrdiff_t n) noexcept {
  assert_debug(n >= 0, "cannot advance iterator struct with negative number: %d", n);
  T* const nextp = self->cursor += n;
  if (self->cursor != self->end && nextp < self->end) {
    self->cursor = nextp;
  } else {
    self->cursor = self->end;
  }
  return self->cursor;
}

template <class T>
[[gnu::nonnull, gnu::returns_nonnull]]
constexpr Iter<T>::Poitner advance(Iter<T>* self) noexcept {
  return advance(self, 1);
}

template <class T>
using ConstIter = Iter<const T>;

template <class T>
struct Span {
  T* begin;
  T* end;
};

}  // namespace iter
}  // namespace nv

#endif
