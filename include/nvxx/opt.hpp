#pragma once

#include <type_traits>
#include "nvxx/common.hpp"
#include "nv/core/log.h"

namespace nv::opt {

struct NoneType {
  enum struct Token { Id };

  constexpr NoneType() noexcept = default;
  constexpr NoneType(Token) noexcept {}
};

inline constexpr NoneType None = NoneType{NoneType::Token::Id};

template <class T>
  requires(Pod<T>)
struct Opt;

template <class T>
  requires(Pod<T>)
constexpr Opt<T> Some(T val) noexcept;

template <class T>
  requires(Pod<T>)
struct Opt {
  static_assert(!std::is_reference_v<T>, "Opt cannot contain reference types. use std::reference_wrapper instead!");

  using Type = T;
  using ValueType = RemoveCvref<T>;
  using Ref = std::add_lvalue_reference_t<ValueType>;
  using ConstRef = const ValueType&;
  using Pointer = ValueType*;
  using ConstPointer = const ValueType*;

  constexpr explicit Opt() noexcept = default;

  constexpr Opt(RemoveRef<T> val) noexcept : some(val), has_some(true) {}
  constexpr Opt(NoneType) noexcept : none(None), has_some(false) {}

  friend constexpr Opt<T> Some<>(T val) noexcept;

  constexpr operator bool() const noexcept { return this->has_some; }

  constexpr bool is_some() const noexcept { return this->has_some; }
  constexpr bool is_none() const noexcept { return !this->has_some; }

  constexpr ValueType value() const noexcept {
    if (this->is_some()) [[likely]] {
      return this->some;
    }
    LOG_FATAL("Cannot get value of Opt with no value!");
  }

  constexpr ValueType value_or(T val) const noexcept {
    if (this->is_some()) {
      return this->some;
    }
    return val;
  }

  constexpr ConstRef ref() const noexcept {
    if (this->is_some()) [[likely]] {
      return this->some;
    }
    LOG_FATAL("Cannot get constant reference to inner value of Opt that conatins no value!%s", "");
  }

  constexpr Pointer operator->() noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return &this->some;
  }

  constexpr ConstPointer operator->() const noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return &this->some;
  }

  constexpr Ref operator*() noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return this->some;
  }

  constexpr ConstRef operator*() const noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return this->some;
  }

  constexpr ValueType unwrap() const noexcept {
    assert_debug(this->is_some(), "Cannot unwrap Opt containing None!");
    return this->some;
  }

 private:
  union {
    NoneType none;
    T some;
  };
  bool has_some : 1;
};

template <class T>
constexpr bool operator==(const Opt<T>& lhs, const NoneType&) noexcept {
  return lhs.is_none();
}
template <class T>
constexpr bool operator==(const NoneType&, const Opt<T>& rhs) noexcept {
  return rhs.is_none();
}

template <class T>
constexpr bool operator!=(const Opt<T>& lhs, const NoneType&) noexcept {
  return lhs.is_some();
}
template <class T>
constexpr bool operator!=(const NoneType&, const Opt<T>& rhs) noexcept {
  return rhs.is_some();
}

template <class T>
constexpr Opt<std::remove_reference<T>> Some(T val) noexcept {
  return Opt{val};
}

template <class T>
constexpr auto opt_new = Some<T>;

template <class T>
using OptPtr = Opt<std::add_pointer_t<T>>;

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
namespace priv {

template <class E>
concept ResultErrTraits = !std::is_void_v<E> && (Pod<E> || std::is_enum_v<E>);

template <class T>
concept ResultValTraits = std::is_void_v<T> || Pod<T>;

template <class T, class E>
concept ResultTraits = ResultValTraits<T> && ResultErrTraits<E>;

}  // namespace priv

template <class E>
  requires(priv::ResultErrTraits<E>)
struct ErrorResult {
  static_assert(!std::is_reference_v<E>,
                "E must not be reference! use std::reference_wrapper if you need a reference!");
  E err;

  constexpr ErrorResult() noexcept = default;
  constexpr ErrorResult(E val) noexcept : err(val) {}
};

// namespace priv

template <class T, class E>
  requires(priv::ResultTraits<T, E>)
struct Result {
  static_assert(!std::is_void_v<T> && !std::is_void_v<E>);
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
    return this->err.err;
  }

  constexpr ErrorReference error() noexcept
    requires(!std::is_const_v<E>)
  {
    if (this->is_ok()) {
      LOG_FATAL("Cannot return error value for a Result that contains a T value!");
    }
    return this->err.err;
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
      return this->err.err;
    }
    LOG_FATAL("Cannot unwrap_err Result that contains Okay value!");
  }

  template <class Func>
    requires(std::invocable<Func, ConstReference>)
  constexpr Result<RemoveCvref<ReturnType<Func, ConstReference>>, ErrorValueType> and_then(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return Result{std::forward<Func>(fun)(this->data)};
    }
    return Result{};
  }

  template <class Func, class U = ValueType>
    requires(std::invocable<Func, ValueType>)
  constexpr Result<U, E> map(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return Result{std::forward<Func>(fun)(this->data)};
    }
    return Result{};
  }

  template <class Func>
    requires(std::invocable<Func> && std::same_as<Result, RemoveCvref<ReturnType<Func>>>)
  constexpr Result or_else(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return *this;
    }
    return std::forward<Func>(fun)();
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

template <class E>
  requires(priv::ResultErrTraits<E>)
struct Result<void, E> {
  static_assert(!IsRef<E>);

  CONTAINER_TEMPLATE_TYPES(E);

  constexpr explicit Result() noexcept = default;
  constexpr Result(ErrorResult<E> error) noexcept : err(error), is_error(true) {}

  constexpr operator bool() const noexcept { return this->is_ok(); }

  constexpr Reference operator*() noexcept { return this->err.err; }
  constexpr ConstReference operator*() const noexcept { return this->err.err; }

  constexpr Pointer operator->() noexcept { return &this->err.err; }

  constexpr ConstReference error() const noexcept {
    if (this->is_ok()) {
      LOG_FATAL("Cannot return error value for a const T&& argResult that contains a T value!");
    }
    return this->err.err;
  }

  constexpr Reference error() noexcept {
    if (this->is_ok()) {
      LOG_FATAL("Cannot return error value for a Result that contains a T value!");
    }
    return this->err.err;
  }

  constexpr void value() const noexcept {}
  constexpr void unwrap() const noexcept {}

  constexpr Type unwrap_err() const noexcept {
    if (this->is_err()) {
      return this->err.err;
    }
    LOG_FATAL("Cannot unwrap_err Result that contains Okay value!");
  }
  // TODO: Need to give these monadic operation methods some more TLC...
  template <class Func>
    requires(std::invocable<Func>)
  constexpr Result<RemoveCvref<ReturnType<Func>>, ValueType> and_then(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return Result{std::forward<Func>(fun)()};
    }
    return Result{};
  }

  template <class Func, class U = void>
    requires(std::invocable<Func>)
  constexpr Result<U, E> map(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return Result{std::forward<Func>(fun)()};
    }
    return Result{};
  }

  template <class Func>
    requires(std::invocable<Func> && std::same_as<Result, RemoveCvref<ReturnType<Func>>>)
  constexpr Result or_else(Func&& fun) const noexcept {
    if (this->is_ok()) {
      return *this;
    }
    return std::forward<Func>(fun)();
  }

  constexpr bool is_err() const noexcept { return this->is_error; }
  constexpr bool is_ok() const noexcept { return !this->is_error; }

 private:
  ErrorResult<E> err;
  bool is_error : 1;
};

template <class T, class E>
  requires priv::ResultTraits<T, E>
constexpr T unwrap(const Result<T, E>& res) noexcept {
  if (res.is_ok()) {
    return *res;
  }
}

}  // namespace nv::opt
