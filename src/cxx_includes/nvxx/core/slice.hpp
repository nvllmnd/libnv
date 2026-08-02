#pragma once

#include <span>
#include "nv/core/intdefs.h"

namespace nv::slice {
using std::span;

template <class T, usize N = std::dynamic_extent>
using Slice = std::span<T, N>;

template <class T>
constexpr Slice<T> make_slice(T* begin, T* end) noexcept {
  return std::span<T>{begin, end};
}

template <class T>
constexpr Slice<T> make_slice(T* begin, isize count) noexcept {
  return make_slice(begin, begin + count);
}

template <usize N, class T>
constexpr Slice<T> make_slice(T* begin) noexcept {
  return std::span<T, N>{begin, N};
}

}  // namespace nv::slice
