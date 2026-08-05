#pragma once

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

  constexpr usize len() const noexcept { return this->count; }
  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t i) noexcept { return *(self.data + i); }
  constexpr nv::ptr::ptr<T> begin() const noexcept { return this->data; }
  constexpr nv::ptr::ptr<const T> cbegin() const noexcept { return this->data; }

  constexpr nv::ptr::ptr<T> end() const noexcept { return this->begin() + this->len(); }
  constexpr nv::ptr::ptr<const T> cend() const noexcept { return this->cbegin() + this->len(); }
};

template <class T>
inline constexpr Slice<T> EmptySlice = {};

template <typename T>
constexpr Slice<T> subspan(const Slice<T> self, i32 begin, i32 end) noexcept {
  const auto len = end - begin;
  ASSERT(len >= 0, "End index must be >= begin index for subspan! got: begin: %d, end: %d, as delta: %d", begin, end,
         delta);
  return {
      .data = self.data + begin,
      .count = len,
  };
}

template <typename T>
constexpr Slice<T> subspan(const Slice<T> self, i32 begin = 0) noexcept {
  ASSERT(begin <= self.len(), "begin index must be <= slice length of: %d, got: %d", self.len(), begin);
  const auto delta = self.len() - begin;
  return subspan(self, begin, delta);
}

template <class T>
constexpr bool is_empty(const Slice<T>& self) noexcept {
  return &self == &EmptySlice<T> || is_null(self.data) || self.count <= 0;
}

template <class T>
constexpr bool operator==(const Slice<T>& lhs, const Slice<T>& rhs) noexcept {
  return lhs.data == rhs.data && lhs.count == rhs.count;
}

template <class T>
constexpr bool operator!=(const Slice<T>& lhs, const Slice<T>& rhs) noexcept {
  return !(lhs == rhs);
}

template <class T>
Slice(T*, std::type_identity_t<T>*) -> Slice<T>;

template <class T>
Slice(const T*, std::type_identity_t<const T>*) -> Slice<const T>;

template <class T>
constexpr nv::ptr::ptr<T> begin(const Slice<T>& self) noexcept {
  return self.data;
}

template <class T>
constexpr nv::ptr::ptr<T> end(const Slice<T>& self) noexcept {
  return nv::slice::begin(self) + self.count;
}

template <class T>
constexpr nv::ptr::ptr<const T> begin(const Slice<const T>& self) noexcept {
  return self.data;
}

template <class T>
constexpr nv::ptr::ptr<const T> end(const Slice<const T>& self) noexcept {
  return nv::slice::begin(self) + self.count;
}

template <class T>
constexpr Slice<T> slice_new(T* begin, std::type_identity_t<T>* end) noexcept {
  ASSERT(begin <= end, "expected non-negative integeger but got: %d", count);
  const auto len = end - begin;
  return {.data = begin, .count = len};
}

template <class T>
constexpr Slice<T> slice_new(T* ptr, i32 count) noexcept {
  ASSERT(count >= 0, "expected non-negative integeger but got: %d", count);
  return {.data = ptr, .count = count};
}

template <i32 N, class T>
constexpr Slice<T> slice_new(T* start) noexcept {
  static_assert(N >= 0, "N must be >= 0!");
  return {.data = start, .count = N};
}

}  // namespace nv::slice
