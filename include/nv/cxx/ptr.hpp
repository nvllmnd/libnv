#pragma once

#include <bit>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <concepts>

#include "nv/core/attributes.h"
#include "nv/core/debug.h"
#include "nv/core/log.h"
#include "nv/cxx/alloc.hpp"
#include "opt.hpp"
#include "slice.hpp"

#ifdef __cplusplus

namespace nv::ptr {

template <class T>
[[gnu::pure]]
static constexpr bool is_null(const T* ptr) noexcept {
  return nullptr == ptr;
}

template <class T>
[[gnu::pure]]
static constexpr bool is_not_null(const T* ptr) noexcept {
  return nullptr != ptr;
}

template <typename T>
using lvalue = T&;

template <typename T>
using rvalue = T&&;

/// @brief alias for a const reference to T
/// @details there is no alias for non-const references, as they should be used sparingly for implemeting operator
/// overloads and other such impls
template <class T>
using Ref = const T&;

// template <class T>
// concept Pointer = requires(T p) {
//   typename T::Type;
//
//   *p;
//   p.operator->();
//   { std::addressof(*p) } -> std::same_as<typename T::Type*>;
//   { p.is_null() } -> std::same_as<bool>;
//   { p.is_not_null() } -> std::same_as<bool>;
//   p[0];
// };
//
/// @brief lite wrapper around T&
/// @details instance of this type must always contain a non-null pointer (stored as T&),
/// and as such its public constructor will abort execution if passed a nullptr
///
/// Dereferencing this type through operator* (or operator->) is (mostly) always safe, unless
/// reference becomes dangling somehow, as this abstracts non-null pointers, which does not (and cannot) ensure
/// refernces are not dangling without another layer of indirection
template <typename T>
  requires(!std::is_null_pointer<T>())
struct NonNull {
  using PointerType = T;

  template <typename U>
    requires(!std::is_null_pointer<U>())
  friend struct NonNull;

  /// @brief returns a suitably aligned, non-null pointer to T.
  /// @details poitner returned must not be used an way that would normally cause UB for a dangling pointer,
  /// only use returned pointer for lazy initialziation for otherwise non-null pointers, or check rustdocs for Rust's
  /// std::ptr::NonNull::<T>::dangling()
  static constexpr NonNull dangling() noexcept { return NonNull{reinterpret_cast<T*>(alignof(T))}; }

  constexpr explicit NonNull(T& val) noexcept : ptr(&val) {}

  template <typename U>
  constexpr explicit NonNull(const NonNull<U>& other) noexcept
    requires(std::is_convertible_v<U*, T*> || std::is_pointer_interconvertible_base_of_v<U*, T*>)
      : ptr(reinterpret_cast<T*>(other.as_raw())) {}

  /// @brief aborts execution if given nullptr
  constexpr explicit NonNull(T* val) noexcept
      : ptr(is_not_null(val) ? val : LOG_FATAL("Cannot create a NonNull<T> from a null pointer!")) {}

  /// @brief Creates a NonNull pointer to T without checking if it is not null first
  /// @details instance is created by dereferecing the pointer to trigger private constructor
  static constexpr NonNull make_unsafe(T* p) noexcept {
    assert_debug(p, "Attempted to create NonNull with nullpointer!");
    return NonNull(*p);
  }

  /// @brief same as derefing raw pointer with -> operator
  /// @details gauranteed to not be null, but pointer can still be dangling
  [[gnu::returns_nonnull]]
  constexpr T* operator->() const noexcept
    requires(!std::is_void_v<T>)
  {
    return this->ptr;
  }

  template <typename U>
  constexpr explicit operator NonNull<U>() const noexcept
    requires(std::is_convertible_v<T*, U*> || std::is_pointer_interconvertible_base_of_v<T*, U*>)
  {
    return {this->reinterp<U>()};
  }

  constexpr explicit operator T&() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->as_raw();
  }

  template <typename U>
  constexpr explicit operator U&() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->cast<U>();
  }

  /// @brief same as derefing raw poitners
  /// @details gauranteed to not be null, but pointer can still be dangling
  constexpr T& operator*() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->ptr;
  }
  /// @brief same as raw pointer indexing
  constexpr T& operator[](std::ptrdiff_t index) const noexcept
    requires(!std::is_void_v<T>)
  {
    return *((this->ptr) + index);
  }

  /// @brief same as raw poitner addition
  constexpr NonNull operator+(std::ptrdiff_t index) const noexcept
    requires(!std::is_void_v<T>)
  {
    return NonNull{*((this->ptr) + index)};
  }
  /// @brief same as raw pointer subtraction
  constexpr std::ptrdiff_t operator-(const NonNull<T>& other) const noexcept
    requires(!std::is_void_v<T>)
  {
    return (this->ptr) - (&other.ptr);
  }

  /// @brief returns constant reference to inner pointer
  constexpr const T& as_ref() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->ptr;
  }

  constexpr bool operator==(const NonNull& other) const noexcept { return this->ptr == other.ptr; }

  constexpr bool operator>(const NonNull& rhs) const noexcept { return this->ptr > rhs.ptr; }
  constexpr bool operator>=(const NonNull& rhs) const noexcept { return this->ptr >= rhs.ptr; }

  constexpr bool operator<(const NonNull& rhs) const noexcept { return this->ptr < rhs.ptr; }
  constexpr bool operator<=(const NonNull& rhs) const noexcept { return this->ptr <= rhs.ptr; }

  constexpr bool operator!=(const NonNull& rhs) const noexcept { return !(*this == rhs); }

  /// @brief returns inner raw pointer
  [[gnu::returns_nonnull]]
  constexpr T* as_raw() const noexcept {
    return this->ptr;
  }

  template <typename U>
    requires(std::is_convertible_v<T*, U*> || std::is_pointer_interconvertible_base_of_v<T*, U*>)
  constexpr NonNull<U> cast() const noexcept {
    auto* p = std::bit_cast<U*>(this->ptr);
    return NonNull<U>{*p};
  }

  template <typename U>
    requires(std::is_convertible_v<T*, U*> || std::is_pointer_interconvertible_base_of_v<T*, U*>)
  constexpr NonNull<U> reinterp() const noexcept {
    return NonNull<U>{*reinterpret_cast<U*>(this->ptr)};
  }

  static consteval bool is_null() noexcept { return false; }
  static consteval bool is_not_null() noexcept { return true; }

 private:
  T* ptr;
};

template <typename T>
constexpr bool operator==(const T* lhs, const NonNull<T>& rhs) noexcept {
  return lhs == rhs.as_raw();
}

template <typename T>
constexpr bool operator!=(const T* lhs, const NonNull<T>& rhs) noexcept {
  return !(lhs == rhs);
}

template <typename T>
constexpr nv::opt::Opt<NonNull<T>> make_nonnull(T* ptr) noexcept {
  if (ptr) {
    return nv::opt::Some(NonNull<T>{*ptr});
  }
  return nv::opt::None;
}

template <typename T>
constexpr NonNull<T> make_nonnull(T& val) noexcept {
  return NonNull<T>{val};
}

using AnyNonNull = NonNull<byte>;
using VoidNonNull = NonNull<void>;

using CanyNonNull = NonNull<const byte>;
using CvoidNonNull = NonNull<const void>;

template <class T>
NonNull(T*) -> NonNull<T>;

template <class T>
NonNull(const T*) -> NonNull<const T>;

template <class T>
NonNull(T&) -> NonNull<T>;

template <class T>
NonNull(const T&) -> NonNull<const T>;

template <typename T, alloc::Allocator A>
struct Box {
  NonNull<T> ptr;
  A alloc;
};

}  // namespace nv::ptr

#endif

#ifndef __cplusplus
#error "libnv cxx module ptr.hpp can only be included/used from C++!";
#endif
