#pragma once

#include <concepts>
#include <type_traits>
#include "nv/core/assert.h"
#include "nvxx/ptr.hpp"
#include "nv/core/intdefs.h"

namespace nv::slice {

template <class T>
struct Slice {
  static_assert(!std::is_void_v<T>, "Cannot create Slice<void>! use Slice<byte> instead!");

  nv::ptr::ptr<T> data;
  i32 count;

  [[gnu::pure]]
  constexpr isize len() const noexcept {
    return this->count;
  }

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t i) noexcept { return *(self.data + i); }
  [[gnu::pure]]
  constexpr nv::ptr::ptr<T> begin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr::ptr<const T> cbegin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr::ptr<T> end() const noexcept {
    return this->begin() + this->len();
  }

  [[gnu::pure]]
  constexpr nv::ptr::ptr<const T> cend() const noexcept {
    return this->cbegin() + this->len();
  }

  [[gnu::pure]]
  constexpr bool is_empty() const noexcept {
    return is_null(this->data) || this->count <= 0;
  }

  constexpr Slice<const T> as_const() const noexcept { return {.data = this->data, .count = this->count}; }

  [[gnu::pure]]
  constexpr Slice subspan(i32 begin, i32 end) const noexcept {
    const auto len = end - begin;
    ASSERT(len >= 0, "End index must be >= begin index for subspan! got: begin: %d, end: %d, as delta: %d", begin, end,
           len);
    return {
        .data = this->data + begin,
        .count = len,
    };
  }

  [[gnu::pure]]
  constexpr Slice subspan(i32 begin = 0) noexcept {
    ASSERT(begin <= this->len(), "begin index must be <= slice length of: %d, got: %d", this->len(), begin);
    const auto delta = this->len() - begin;
    return this->subspan(begin, delta);
  }
};

template <class T>
inline constexpr Slice<T> EmptySlice = {};

template <class T>
[[gnu::pure]]
constexpr bool is_empty(const Slice<T>& self) noexcept {
  return &self == &EmptySlice<T> || self.is_empty();
}

template <class T>
[[gnu::pure]]
constexpr bool operator==(const Slice<T>& lhs, const Slice<T>& rhs) noexcept {
  return lhs.data == rhs.data && lhs.count == rhs.count;
}

template <class T>
[[gnu::pure]]
constexpr bool operator!=(const Slice<T>& lhs, const Slice<T>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class T>
Slice(T*, std::add_pointer_t<T>) -> Slice<T>;

template <class T>
Slice(const T*, std::add_pointer_t<const T>) -> Slice<const T>;

template <class T>
Slice(T*, i32) -> Slice<T>;

template <class T>
Slice(T*, isize) -> Slice<T>;

template <class T>
Slice(const T*, i32) -> Slice<const T>;

template <class T>
Slice(const T*, isize) -> Slice<const T>;

template <class T>
[[gnu::pure]]
constexpr Slice<T> slice_new(T* begin, std::add_pointer_t<T> end) noexcept {
  const auto len = end - begin;
  ASSERT(begin <= end, "expected non-negative integeger but got: %d", len);
  return {.data = begin, .count = len};
}

template <class T>
[[gnu::pure]]
constexpr Slice<T> slice_new(T* ptr, i32 count) noexcept {
  ASSERT(count >= 0, "expected non-negative integeger but got: %d", count);
  return {.data = ptr, .count = count};
}

template <i32 N, class T>
[[gnu::pure]]
constexpr Slice<T> slice_new(T* start) noexcept {
  static_assert(N >= 0, "N must be >= 0!");
  return {.data = start, .count = N};
}

using Sview = std::string_view;

}  // namespace nv::slice
