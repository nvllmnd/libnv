#pragma once

#include <iterator>
#ifdef __cplusplus

#endif

#include <cstddef>
#include <type_traits>
#include "nvxx/common.hpp"

namespace nv::inline slice {

template <class T>
struct Slice {
  static_assert(!std::is_void_v<T>, "Cannot create Slice<void>! use Slice<byte> instead!");

  CONTAINER_TEMPLATE_TYPES(T);

  nv::ptr<T> data;
  i32 count;

  [[gnu::pure]]
  constexpr isize len() const noexcept {
    return this->count;
  }

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t i) noexcept { return *(self.data + i); }

  [[gnu::pure]]
  constexpr nv::ptr<T> begin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr<const T> cbegin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr<T> end() const noexcept {
    return this->begin() + this->len();
  }

  [[gnu::pure]]
  constexpr nv::ptr<const T> cend() const noexcept {
    return this->cbegin() + this->len();
  }

  [[gnu::pure]]
  constexpr bool is_empty() const noexcept {
    return is_null(this->data) || this->count <= 0;
  }

  constexpr Slice<const T> as_const() const noexcept { return {.data = this->data, .count = this->count}; }

  [[gnu::pure]]
  constexpr Slice subslice(i32 begin, i32 end) const noexcept {
    const auto len = end - begin;
    ASSERT(len >= 0, "End index must be >= begin index for subslice! got: begin: %d, end: %d, as delta: %d", begin, end,
           len);
    return {
        .data = this->data + begin,
        .count = len,
    };
  }

  [[gnu::pure]]
  constexpr Slice subslice(i32 begin = 0) const noexcept {
    ASSERT(begin <= this->len(), "begin index must be <= slice length of: %d, got: %d", this->len(), begin);
    const auto delta = this->len() - begin;
    return this->subslice(begin, delta);
  }

  template <isize N>
  constexpr Slice<T[N]> to_sized() const noexcept {
    static_assert(N > 0);
    ASSERT(N < this->len(), "N(%li) is too large for slice of length: %li", N, this->len());
    return {.data = this->data};
  }
};

template <class T, isize N>
struct Slice<T[N]> {
  static_assert(N >= 1, "Non-Type template parameter N (for Slice) must be >= 1");

  nv::ptr<T> data;

  constexpr isize len() const noexcept { return N; }

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t i) noexcept { return *(self.data + i); }

  template <isize O>
  constexpr decltype(auto) index() noexcept {
    static_assert(O >= 0 && O < N, "Index falls outside the valid range for Slice with statically known size!");
    return this->operator[](O);
  }

  [[gnu::pure]]
  constexpr nv::ptr<T> begin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr<const T> cbegin() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr<T> end() const noexcept {
    return this->begin() + this->len();
  }

  [[gnu::pure]]
  constexpr nv::ptr<const T> cend() const noexcept {
    return this->cbegin() + this->len();
  }

  [[gnu::pure]]
  constexpr bool is_empty() const noexcept {
    return is_null(this->data) || this->len() <= 0;
  }

  constexpr Slice<const T> as_const() const noexcept { return {.data = this->data, .count = this->len()}; }

  /// @brief creates new slice beginning at index offset and extending to n elements
  /// @details this method does a runtime assert on parameters to ensure they do not cause any access violation
  [[gnu::pure]]
  constexpr Slice subslice(i32 begin_offset, i32 n) const noexcept {
    ASSERT((begin_offset >= 0) && (begin_offset < N) && (n >= 0) && (begin_offset + n) < N,
           "begin_offset: %li (or begin_offset + count(%li) = %li) falls outside range of Slice with static size: %li!",
           begin_offset, n, begin_offset + n, N);
    return {
        .data = this->data + begin_offset,
        .count = n,
    };
  }

  /// @brief creates new subslice starting at begin offset and extending to the end of the slice
  /// @remarks if called with no arguments, makes a shallow copy of this slice
  [[gnu::pure]]
  constexpr Slice subslice(i32 begin = 0) const noexcept {
    ASSERT(begin <= this->len(), "begin index must be <= slice length of: %d, got: %d", this->len(), begin);
    const auto delta = this->len() - begin;
    return this->subslice(begin, delta);
  }

  /// @brief converts a Slice of statically known size to a slice with runtime length tracking
  constexpr Slice<T> to_slice() const noexcept { return {.data = this->data, .count = N}; }
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

template <class T, isize L, isize R = L>
[[gnu::pure]]
constexpr bool operator==(const Slice<T[L]>& lhs, const Slice<T[R]>& rhs) noexcept {
  if constexpr (L == R) {
    return lhs.begin() == rhs.begin();
  } else {
    return false;
  }
}

template <class T, isize L, isize R = L>
[[gnu::pure]]
constexpr bool operator!=(const Slice<T[L]>& lhs, const Slice<T[R]>& rhs) noexcept {
  return !(lhs == rhs);
}

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
constexpr Slice<T[N]> slice_new(T* start) noexcept {
  static_assert(N >= 0, "N must be >= 0!");
  return {
      .data = start,
  };
}

}  // namespace nv::inline slice
