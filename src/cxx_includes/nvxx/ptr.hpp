#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

#include "nv/core/debug.h"
#include "nv/core/log.h"
#include "core/opt.hpp"

#ifdef __cplusplus

namespace nv::ptr {

template <class T>
  requires(!std::is_null_pointer_v<T>)
using ptr = std::add_pointer_t<T>;

template <class T>
  requires(!std::is_null_pointer_v<T>)
using const_ptr = ptr<const T>;

using void_ptr = ptr<void>;

template <class T>
[[gnu::pure]]
constexpr bool is_null(const T* ptr) noexcept {
  return nullptr == ptr;
}

template <class T>
[[gnu::pure]]
constexpr bool is_not_null(const T* ptr) noexcept {
  return nullptr != ptr;
}

template <typename T>
using lvalue = std::add_lvalue_reference_t<T>;

template <class T>
using const_lvalue = std::add_lvalue_reference_t<std::add_const_t<T>>;

template <typename T>
using rvalue = std::add_rvalue_reference_t<T>;

template <class T>
struct NonNull {
  static_assert(!std::is_null_pointer_v<T>, "NonNull<nullptr_t> is not valid!");
  static_assert(!std::is_void_v<T>,
                "NonNull<void> is invalid use nv::byte/std::byte for a NonNull pointer to raw memory!");

  [[gnu::nonnull]]
  static constexpr NonNull make_unsafe(T* ptr) noexcept {
    return NonNull{ptr};
  }

  [[gnu::pure]]
  static constexpr nv::opt::Opt<NonNull> make(T* ptr) noexcept {
    if (nv::ptr::is_null(ptr)) [[unlikely]] {
      return nv::opt::None;
    }
    return nv::opt::Some(NonNull<T>::make_unsafe(ptr));
  }

  constexpr lvalue<T> operator*() const noexcept { return *this->data; }
  [[gnu::returns_nonnull]]
  constexpr nv::ptr::ptr<T> operator->() const noexcept {
    return this->data;
  }

  [[gnu::pure]]
  constexpr nv::ptr::ptr<T> ptr() const noexcept {
    return this->data;
  }

  [[gnu::const]]
  static constexpr bool is_null() noexcept {
    return false;
  }
  [[gnu::const]]
  static constexpr bool is_not_null() noexcept {
    return true;
  }

 private:
  [[gnu::nonnull]]
  constexpr NonNull(T* ptr) noexcept
      : data(ptr) {}

  nv::ptr::ptr<T> data;
};

template <class T>
[[gnu::pure]]
constexpr nv::opt::Opt<NonNull<T>> nonnull(T* ptr) noexcept {
  return NonNull<T>::make(ptr);
}

template <class T>
[[gnu::pure, gnu::nonnull]]
constexpr NonNull<T> nonnull_unsafe(T* ptr) noexcept {
  return NonNull<T>::make_unsafe(ptr);
}

template <class T>
NonNull(T*) -> NonNull<T>;

template <class T>
NonNull(const T*) -> NonNull<const T>;
}  // namespace nv::ptr

#endif

#ifndef __cplusplus
#error "libnv cxx module ptr.hpp can only be included/used from C++!";
#endif
