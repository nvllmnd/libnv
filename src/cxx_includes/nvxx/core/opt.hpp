#pragma once

#include <optional>
#include <type_traits>
#include "nv/core/debug.h"
#include "nv/core/log.h"

namespace nv::opt {

using NoneType = std::nullopt_t;

inline constexpr NoneType None = std::nullopt;

template <class T>
struct Opt;

template <class T>
constexpr Opt<T> Some(T val) noexcept;

template <class T>
struct Opt {
  static_assert(!std::is_reference_v<T>, "Opt cannot contain reference types. use std::reference_wrapper instead!");
  static_assert(std::is_trivially_destructible_v<T>, "Opt for T must be trivially destructible!");

  constexpr explicit Opt() noexcept : none(None), has_some(false) {}

  constexpr Opt(std::remove_reference_t<T> val) noexcept : some(val), has_some(true) {}
  constexpr Opt(NoneType) noexcept : none(None), has_some(false) {}

  friend constexpr Opt Some(std::remove_reference_t<T> val) noexcept { return {val}; }

  constexpr operator bool() const noexcept { return this->has_some; }

  constexpr bool is_some() const noexcept { return this->has_some; }
  constexpr bool is_none() const noexcept { return !this->has_some; }

  constexpr const std::decay<T>& ref() const noexcept {
    if (this->is_some()) [[likely]] {
      return this->some;
    }
    LOG_FATAL("Cannot get constant reference to inner value of Opt that conatins no value!%s", "");
  }

  constexpr T* operator->() noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return &this->some;
  }

  constexpr const T* operator->() const noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return &this->some;
  }

  constexpr std::decay<T>& operator*() noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return this->some;
  }

  constexpr const std::decay<T>& operator*() const noexcept {
    assert_debug(this->has_some, "Cannot call operator -> on Opt with no value! %s", "");
    return this->some;
  }

 private:
  union {
    NoneType none;
    T some;
  };
  bool has_some : 1;
};

static_assert(std::is_standard_layout_v<Opt<int>> && std::is_trivially_destructible_v<Opt<int>>);

template <class T>
constexpr Opt<T> make_opt(T val) noexcept {
  return std::make_optional<T>(val);
}

}  // namespace nv::opt
