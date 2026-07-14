#pragma once

#include <bit>

#include "nv/core/log.h"
#include "opt.hpp"

namespace nv {

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

/// @brief lite wrapper around T&
/// @details instance of this type must always contain a non-null pointer (stored as T&),
/// and as such its public constructor will abort execution if passed a nullptr
///
/// Dereferencing this type through operator* (or operator->) is (mostly) always safe, unless
/// reference becomes dangling somehow, as this abstracts non-null pointers, which does not (and cannot) ensure
/// refernces are not dangling without another layer of indirection
template <typename T>
struct NonNull {
  /// @brief aborts execution if given nullptr
  constexpr explicit NonNull(T* val) noexcept {
    if (nullptr == val) [[unlikely]] {
      LOG_FATAL("Cannot create a NonNull<T> from a null pointer!");
    }
    this->ptr = *val;
  }

  constexpr NonNull(const NonNull<T>& other) noexcept : ptr(other.ptr) {}
  constexpr NonNull(NonNull<T>&& other) noexcept : ptr(other.ptr) {}

  NonNull<T>& operator=(const NonNull<T>& other) noexcept {
    if (this != &other) {
      this->ptr = other.ptr;
    }
    return *this;
  }

  NonNull<T>& operator=(NonNull<T>&& other) noexcept {
    if (this != &other) {
      this->ptr = other.ptr;
    }
    return *this;
  }

  ~NonNull() = default;

  /// @brief same as derefing raw pointer with -> operator
  /// @details gauranteed to not be null, but pointer can still be dangling
  constexpr T* operator->() noexcept { return &this->ptr; }

  /// @brief same as derefing raw poitners
  /// @details gauranteed to not be null, but pointer can still be dangling
  constexpr T& operator*() noexcept { return this->ptr; }
  /// @brief same as raw pointer indexing
  constexpr T& operator[](std::ptrdiff_t index) noexcept { return (&this->ptr) + index; }

  /// @brief same as raw poitner addition
  constexpr NonNull<T> operator+(std::ptrdiff_t index) noexcept {
    return {
        .ptr = (&this->ptr) + index,
    };
  }
  /// @brief same as raw pointer subtraction
  constexpr std::ptrdiff_t operator-(const NonNull<T>& other) noexcept {
    return {
        .ptr = (&this->ptr) - (&other.ptr),
    };
  }

  /// @brief returns constant reference to inner pointer
  constexpr const T& as_ref() const noexcept { return &this->ptr; }

  /// @brief returns inner raw pointer
  constexpr T* as_ptr() const noexcept { return &this->ptr; }

  template <typename U>
  constexpr NonNull<U> cast() noexcept {
    return {
        .data = *(std::bit_cast<U*>(&this->ptr)),
    };
  }

  template <typename U>
  constexpr NonNull<const U> ccast() const noexcept {
    return {
        .data = *(std::bit_cast<const U*>(&this->ptr)),
    };
  }

  template <typename U>
  constexpr NonNull<U> reinterp() noexcept {
    return {
        .data = *(reinterpret_cast<U*>(&this->ptr)),
    };
  }

  template <typename U>
  constexpr NonNull<const U> creinterp() const noexcept {
    return {
        .data = *(reinterpret_cast<const U*>(&this->ptr)),
    };
  }

  consteval bool is_null() const noexcept { return false; }
  consteval bool is_not_null() const noexcept { return true; }

 private:
  constexpr explicit NonNull(T& val) noexcept : ptr(val) {}
  T& ptr;
};

using AnyNonNull = NonNull<byte>;
using VoidNonNull = NonNull<void>;

using CanyNonNull = NonNull<const byte>;
using CvoidNonNull = NonNull<const void>;

/// @brief simple wrapper type for raw pointers
/// @details used for a more fluent(ish) api over raw pointers, allowing for:
/// Ptr<int> x = {...};
/// if (x.is_null()) {
/// ...
/// }
///
/// as opposed to
///
/// int* x = ...;
/// if (nullptr == x) {
///    ...
/// }
template <typename T>
struct Ptr {
  T* data;

  static constexpr Ptr<T> make(T* ptr) noexcept { return {.data = ptr}; }

  static constexpr nv::opt::Opt<Ptr<T>> non_null(T* ptr) noexcept {
    using namespace nv::opt;
    if (nullptr == ptr) [[unlikely]] {
      return None<T>();
    }
    return Some(Ptr<T>{.data = ptr});
  }

  static consteval Ptr<T> null() noexcept { return {}; }

  constexpr nv::opt::Opt<const T&> as_ref() const noexcept {
    using namespace nv::opt;
    if (this->is_null()) [[unlikely]] {
      return None<T>();
    }
    return Some(*this->data);
  }

  constexpr const T& ref() const noexcept { return *this->data; }

  constexpr bool is_null() const noexcept { return nullptr == this->data; }
  constexpr bool is_not_null() const noexcept { return nullptr != this->data; }

  constexpr T* operator+(std::ptrdiff_t offset) noexcept { return this->data + offset; }
  constexpr std::ptrdiff_t operator-(const Ptr<T>& other) noexcept { return this->data - other.data; }

  constexpr T* operator->() noexcept { return this->data; }
  constexpr T& operator*() noexcept { return *this->data; }
  constexpr T& operator[](std::ptrdiff_t index) noexcept { return *(this->data + index); }

  template <typename U>
  constexpr Ptr<U> cast() & noexcept {
    return {.data = std::bit_cast<U*>(this->data)};
  }

  template <typename U>
  constexpr Ptr<const U> ccast() const& noexcept {
    return {.data = std::bit_cast<const U*>(this->data)};
  }

  template <typename U>
  constexpr Ptr<U> reinterp() & noexcept {
    return {.data = reinterpret_cast<U*>(this->data)};
  }

  template <typename U>
  constexpr Ptr<const U> creinterp() const& noexcept {
    return {.data = reinterpret_cast<const U*>(this->data)};
  }

  constexpr Ptr<byte> byte_cast() & noexcept { return this->cast<byte>(); }
  constexpr Ptr<const byte> cbyte_cast() const& noexcept { return this->cast<const byte>(); }

  constexpr Slice<byte> as_bytes() & noexcept {
    return {
        .data = std::bit_cast<byte*>(this->data),
        .count = sizeof(T),
    };
  }

  constexpr Slice<const byte> as_cbytes() const& noexcept {
    return {
        .data = std::bit_cast<const byte*>(this->data),
        .count = sizeof(T),
    };
  }
};
using AnyPtr = Ptr<byte>;
using VoidPtr = Ptr<void>;
using CanyPtr = Ptr<const byte>;
using CvoidPtr = Ptr<const void>;

}  // namespace nv
