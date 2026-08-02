#pragma once

#include <functional>
#include <type_traits>
#include <concepts>

#include "nv/core/attributes.h"
#include "nv/core/debug.h"
#include "nv/core/log.h"
#include "nvxx/alloc.hpp"
#include "core/opt.hpp"
#include "core/slice.hpp"

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

  constexpr explicit NonNull(T& val) noexcept : data(&val) {}

  template <typename U>
  constexpr explicit NonNull(const NonNull<U>& other) noexcept
    requires(std::is_convertible_v<U*, T*> || std::is_pointer_interconvertible_base_of_v<U*, T*>)
      : data(reinterpret_cast<T*>(other.ptr())) {}

  /// @brief aborts execution if given nullptr
  constexpr explicit NonNull(T* val) noexcept
      : data(is_not_null(val) ? val : LOG_FATAL("Cannot create a NonNull<T> from a null pointer!")) {}

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
    return this->data;
  }

  template <typename U>
  constexpr explicit operator NonNull<U>() const noexcept
    requires(std::is_layout_compatible_v<T, U>)
  {
    return {this->reinterp<U>()};
  }

  constexpr explicit operator T&() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->ptr();
  }

  template <typename U>
  constexpr explicit operator U&() const noexcept
    requires(!std::is_void_v<T> && std::is_layout_compatible_v<T, U>)
  {
    return *this->cast<U>();
  }

  /// @brief same as derefing raw poitners
  /// @details gauranteed to not be null, but pointer can still be dangling
  constexpr T& operator*() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->data;
  }
  /// @brief same as raw pointer indexing
  constexpr T& operator[](std::ptrdiff_t index) const noexcept
    requires(!std::is_void_v<T>)
  {
    return *((this->data) + index);
  }

  /// @brief same as raw poitner addition
  constexpr NonNull operator+(std::ptrdiff_t index) const noexcept
    requires(!std::is_void_v<T>)
  {
    return NonNull{*((this->data) + index)};
  }
  /// @brief same as raw pointer subtraction
  constexpr std::ptrdiff_t operator-(const NonNull<T>& other) const noexcept
    requires(!std::is_void_v<T>)
  {
    return (this->data) - (&other.data);
  }

  /// @brief returns constant reference to inner pointer
  constexpr const T& as_ref() const noexcept
    requires(!std::is_void_v<T>)
  {
    return *this->data;
  }

  constexpr bool operator==(const NonNull& other) const noexcept { return this->data == other.data; }

  constexpr bool operator>(const NonNull& rhs) const noexcept { return this->data > rhs.data; }
  constexpr bool operator>=(const NonNull& rhs) const noexcept { return this->data >= rhs.data; }

  constexpr bool operator<(const NonNull& rhs) const noexcept { return this->data < rhs.data; }
  constexpr bool operator<=(const NonNull& rhs) const noexcept { return this->data <= rhs.data; }

  constexpr bool operator!=(const NonNull& rhs) const noexcept { return !(*this == rhs); }

  /// @brief returns inner raw pointer
  [[gnu::returns_nonnull]]
  constexpr T* ptr() const noexcept {
    return this->data;
  }

  template <typename U>
    requires(std::is_layout_compatible_v<T, U>)
  constexpr NonNull<U> cast() const noexcept {
    auto* p = std::bit_cast<U*>(this->data);
    return NonNull<U>{*p};
  }

  template <typename U>
  constexpr NonNull<U> reinterp() const noexcept
    requires(std::is_layout_compatible_v<T, U>)
  {
    return NonNull<U>{*reinterpret_cast<U*>(this->data)};
  }

  static consteval bool is_null() noexcept { return false; }
  static consteval bool is_not_null() noexcept { return true; }

 private:
  T* data;
};

template <typename T>
constexpr bool operator==(const T* lhs, const NonNull<T>& rhs) noexcept {
  return lhs == rhs.ptr();
}

template <typename T>
constexpr bool operator!=(const T* lhs, const NonNull<T>& rhs) noexcept {
  return !(lhs == rhs);
}

template <typename T>
constexpr nv::opt::Opt<NonNull<T>> nonnull(T* ptr) noexcept {
  if (ptr) {
    return nv::ptr::NonNull<T>{*ptr};
  }
  return opt::None;
}

template <typename T>
constexpr NonNull<T> nonnull(T& val) noexcept {
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

template <class T>
  requires(!std::is_reference_v<T>)
using Ref = std::reference_wrapper<T>;

using std::cref;
using std::ref;

}  // namespace nv::ptr

#endif

#ifndef __cplusplus
#error "libnv cxx module ptr.hpp can only be included/used from C++!";
#endif
