#pragma once

#include <type_traits>
#include "nvxx/common.hpp"
#include "nv/core/log.h"

#if __cplusplus
#endif

namespace nv::opt {

struct NoneType {
  enum struct Token { Id };

  constexpr NoneType() noexcept = default;
  constexpr NoneType(Token) noexcept {}
};

inline constexpr NoneType None = NoneType{NoneType::Token::Id};

template <class T>
  requires(PodLike<T>)
struct Opt;

template <class T>
  requires(PodLike<T>)
constexpr Opt<T> Some(T val) noexcept;

template <class T>
  requires(PodLike<T>)
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

}  // namespace nv::opt
