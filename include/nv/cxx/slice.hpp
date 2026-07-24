#pragma once

#include "nv/core/intdefs.h"

#include "nv/core/log.h"

namespace nv::slice {

/// @brief a lightweight Slice of T
/// @details this type does not consider complex move semantics or RAII types and as such,
/// T should be of std::is_standard_layout_v<T> == true. Otherwise im honestly not confident in its behavior not being
/// undefined!
///
/// @remarks I made this type because i hate seeing my LSP shit the bed anytime i use std template types, so this needs
/// more rigourous testing!
///
template <typename T>
struct Slice {
  T* data;
  isize count;

  /// @brief indexes this Slice<T> as if it were a (flat and contiguous) 2d Array
  /// @details caller must provide width of slice, as its not tracked by this type, nor can it be inferred from total
  /// length (as far as i am aware lol)
  constexpr T* index_flat(isize width, isize x, isize y) noexcept {
    const isize index = x * width + y;
    if (index > this->len()) [[unlikely]] {
      return nullptr;
    }
    return this->index(index);
  }

  /// @brief creates a Slice<T> from a begin and end pointer
  /// @details this function does not check to ensure end pointer is after begin pointer,
  /// so be caustious and know what you are doing if you use this function!
  static constexpr Slice<T> make_unchecked(T* begin, T* end) noexcept {
    const isize count = end - begin;
    return {
        .data = begin,
        .count = count,
    };
  }
  /// @brief creates a Slice<T> from a begin and end pointer.
  /// @details this function will abort execution if end poitner is not after begin pointer
  static constexpr Slice<T> make(T* begin, T* end) noexcept {
    if (end >= begin) [[likely]] {
      return Slice<T>::make_unchecked(begin, end);
    } else {
      LOG_FATAL("Cannot create Slice<T> where end pointer is behind begin pointer!");
    }
  }

  /// @brief creates Slice<T> from pointer and count
  static constexpr Slice<T> make(T* data, isize count) noexcept {
    return Slice<T>{.data = data, .count = static_cast<isize>(count <= 0 ? 1 : count)};
  }
  /// @brief creates Slice<T> from pointer and count of 1
  static constexpr Slice<T> make(T* data) noexcept { return Slice<T>::make(data, 1); }

  /// @brief creates an empty (zeroed) Slice<T>
  static constexpr Slice<T> empty() noexcept { return Slice<T>{}; }

  constexpr bool is_null() const noexcept { return nullptr == this->data; }

  constexpr bool is_not_null() const noexcept { return !this->is_null(); }

  /// @brief inner count
  /// @details values <= 0 are meaningless and will likely cause unexpected things to happen! spoopy!!
  constexpr isize len() const noexcept { return this->count; }

  /// @brief returns true if inner count <= 0
  constexpr bool is_empty() const noexcept { return this->len() <= 0; }

  constexpr T* begin() noexcept { return this->data; }
  constexpr T* end() noexcept { return this->begin() + this->len(); }

  constexpr const T* cbegin() const noexcept { return this->data; }
  constexpr const T* cend() const noexcept { return this->cbegin() + this->len(); }

  /// @brief returns inner pointer unchecked
  constexpr T* operator->() noexcept { return this->data; }

  /// @brief dereferences inner pointer unchecked
  constexpr T& operator*() noexcept { return *this->data; }

  /// @brief returns pointer offset by given amount
  constexpr T* index(std::ptrdiff_t offset) noexcept { return (this->data + offset); }

  /// @brief returns slice at given offset of given length
  /// @details this method does not do any checking to ensure offset and length are in bounds
  constexpr Slice<T> offset_slice(std::ptrdiff_t offset, isize len) noexcept {
    return {
        .data = this->data + offset,
        .len = len,
    };
  }

  /// @brief returns a sublice of this slice
  /// @details if offset < 0 that parameter is treated as if 0 was passed in,
  /// this is to avoid accidentally grabbing a slice behind the current slice
  /// if you want to get a slice behind current slice, use operator[] (or Slice<T>::offset_slice)with a negaive offset
  constexpr Slice<T> subslice(isize offset, isize len) noexcept {
    offset = offset < 0 ? 0 : offset;
    this->offset_slice(offset, len);
  }

  /// @brief dereferences pointer returned by [Slice<T>::offset] with given offset unchecked
  constexpr T& operator[](std::ptrdiff_t offset) noexcept { return *this->index(offset); }

  /// @brief does a deep equality comparison of this slice and another slice
  /// @details this is different from [Slice<T>::addr_eq] since [Slice<T>::addr_eq] nly does a shallow comparison
  /// (checks the data poitner and count are equal in each Slice)
  ///
  /// This method does a deep equality comparison on each slice, and as such, will only compile
  /// if underlying type also implements the operator==
  constexpr bool operator==(const Slice<T>& other) const noexcept {
    if (this == &other) {
      return true;
    }
    if (this->count == other.count) {
      for (i32 i = 0; i < this->len(); i++) {
        if (this->data[i] != other.data[i]) {
          return false;
        }
      }
      return true;
    }
  }

  /// @brief @see [Slice<T>::operator==]
  /// @details returns flipped boolean value returned by [Slice<T>::operator==]
  /// This method does a deep equality comparison on each slice, and as such, will only compile
  /// if underlying type also implements the operator==
  constexpr bool operator!=(const Slice<T>& other) const noexcept { return !(*this == other); }

  /// @brief does a shalow comparison of 2 slices
  /// @details for a deep comparison, @see [Slice<T>::operator==] or @see [Slice<T>::operator!=]
  constexpr bool addr_eq(const Slice<T>& other) const noexcept {
    if (this == &other) {
      return true;
    }
    return this->data == other.data && this->count == other.count;
  }
};

/// @brief a constant view into a slice characters
using Str = Slice<const char>;

template <typename T>
Slice(T*, i32) -> Slice<T>;

template <typename T>
Slice(const T*, i32) -> Slice<const T>;

}  // namespace nv::slice
