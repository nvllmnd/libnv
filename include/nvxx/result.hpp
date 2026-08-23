#pragma once

#include <type_traits>
#include <tuple>
#include "nv/core/ctypes.h"
#include "nv/core/log.h"
#include "nvxx/opt.hpp"

namespace nv::result {

enum class Error : u64 {
  None = 0,
  AnyError = 1,
  MmapFailed = 1 << 1,
  /// An implementaiton of the 'interface trait' Allocator returned an error value
  AllocITraitImplError = 1 << 2,
  OutOfMemory = 1 << 3,
};

constexpr Error operator|(const Error& lhs, const Error& rhs) noexcept {
  const auto l = static_cast<std::underlying_type_t<Error>>(lhs);
  const auto r = static_cast<std::underlying_type_t<Error>>(rhs);
  return static_cast<Error>(l | r);
}

constexpr Error operator&(const Error& lhs, const Error& rhs) noexcept {
  const auto l = static_cast<std::underlying_type_t<Error>>(lhs);
  const auto r = static_cast<std::underlying_type_t<Error>>(rhs);
  return static_cast<Error>(l & r);
}

constexpr Error& operator|=(Error& lhs, const Error& rhs) noexcept {
  lhs = (lhs | rhs);
  return lhs;
}

constexpr Error& operator&=(Error& lhs, const Error& rhs) noexcept {
  lhs = (lhs & rhs);
  return lhs;
}

template <class E>
  requires((std::is_standard_layout_v<E> && std::is_trivially_destructible_v<E>) || std::is_enum_v<std::decay_t<E>>)
struct ErrorResult {
  static_assert(!std::is_reference_v<E>,
                "E must not be reference! use std::reference_wrapper if you need a reference!");
  E err;

  constexpr ErrorResult() noexcept = default;
  constexpr ErrorResult(E val) noexcept : err(val) {}
};

template <class T, class E>
  requires(PodLike<T> && (PodLike<E> || std::is_enum_v<E>))
struct Result {
  static_assert(!std::is_reference_v<T>,
                "T must not be reference! use std::reference_wrapper if you need a reference!");

  static_assert(!std::is_reference_v<E>,
                "E must not be reference! use std::reference_wrapper if you need a reference!");

  CONTAINER_TEMPLATE_TYPES(T);
  CONTAINER_TEMPLATE_TYPES_AS(Error, E);

  constexpr explicit Result() noexcept = default;
  constexpr Result(RemoveRef<T> val) noexcept : data(val), is_okay(true) {}
  constexpr Result(ErrorResult<RemoveRef<E>> error) noexcept : err(error), is_okay(false) {}

  constexpr operator bool() const noexcept { return this->is_okay; }

  constexpr Reference operator*() noexcept
    requires(!std::is_const_v<T>)
  {
    return this->data;
  }
  constexpr ConstReference operator*() const noexcept
    requires(std::is_const_v<T>)
  {
    return this->data;
  }

  constexpr Pointer operator->() noexcept { return &this->data; }

  constexpr ErrorConstReference error() const noexcept
    requires(std::is_const_v<E>)
  {
    if (this->is_ok()) {
      LOG_FATAL("Cannot return error value for a Result that contains a T value!");
    }
    return this->err;
  }

  constexpr ErrorReference error() noexcept
    requires(!std::is_const_v<E>)
  {
    if (this->is_ok()) {
      LOG_FATAL("Cannot return error value for a Result that contains a T value!");
    }
    return this->err;
  }

  constexpr ConstReference value() const noexcept
    requires(std::is_const_v<T>)
  {
    if (this->is_err()) {
      LOG_FATAL("Cannot return T value for a Result that contains an error value!");
    }
    return this->data;
  }

  constexpr Reference value() noexcept
    requires(!std::is_const_v<T>)
  {
    if (this->is_err()) {
      LOG_FATAL("Cannot return T value for a Result that contains an error value!");
    }
    return this->data;
  }

  constexpr ValueType unwrap() const noexcept {
    if (this->is_ok()) {
      return this->data;
    }
    LOG_FATAL("Cannot unwrap Result that contains Error value!");
  }

  constexpr ErrorType unwrap_err() const noexcept {
    if (this->is_err()) {
      return this->err;
    }
    LOG_FATAL("Cannot unwrap_err Result that contains Okay value!");
  }

  constexpr bool is_err() const noexcept { return !this->is_okay; }
  constexpr bool is_ok() const noexcept { return this->is_okay; }

 private:
  union {
    T data;
    ErrorResult<E> err;
  };
  bool is_okay : 1;
};

template <class E>
constexpr ErrorResult<E> error_new(E err) noexcept {
  return ErrorResult{err};
}

}  // namespace nv::result
