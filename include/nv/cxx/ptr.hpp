#pragma once

#include <bit>

#include "nv/core/log.h"
#include "opt.hpp"

#ifdef __cplusplus

namespace nv::ptr {

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

#endif

#ifndef __cplusplus
#error "libnv cxx module ptr.hpp can only be included/used from C++!";
#endif
