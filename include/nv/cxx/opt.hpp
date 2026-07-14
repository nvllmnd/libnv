#pragma once

#include <type_traits>

#include "nv/core/log.h"

#ifdef __cplusplus

namespace nv::nv {
/// @brief an Optional value over T, or None (Opt<T>::None)
template <typename T>
struct Opt {
  static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>,
                "Type parameter for Opt must satisfy std::is_standard_layout && std::is_trivial!");

  using Some = T;

  struct None {};
  bool has_none : 1;

  union {
    None nonev;
    T somev;
  };

  static constexpr Opt<T> some(T val) noexcept { return Opt<T>{.has_none = false, .somev = val}; }
  static constexpr Opt<T> none() noexcept { return Opt<T>{.has_none = true, .nonev = None{}}; }

  static consteval Opt<T> csome(T val) noexcept { return Opt<T>{.has_none = false, .somev = val}; }
  static consteval Opt<T> cnone() noexcept { return Opt<T>{.has_none = true, .nonev = None{}}; }

  constexpr operator bool() const noexcept { return this->is_some(); }

  constexpr bool is_some() const noexcept { return !this->has_none; }
  consteval bool cis_some() const noexcept { return !this->has_none; }

  constexpr bool is_none() const noexcept { return this->has_none; }
  consteval bool cis_none() const noexcept { return !this->has_none; }

  /// @brief returns a copy of inner value, if any
  /// @details this method does no checking to ensure this Opt acutally contains a value,
  /// and as such, it is UB to call this method on an Opt<T> containing None
  /// for a version that returns a reference instead of a copy, @see [Opt<T>::ref_unchecked]
  constexpr T expect() const& noexcept { return this->somev; }

  /// @brief version of [Opt<T>::unwrap] that returns a const reference instead of a copy
  /// @details this method aborts execution if it does not contain a value
  constexpr const T& as_ref() const& noexcept {
    if (this->is_some()) {
      return this->somev;
    }
    LOG_FATAL("Cannot unwrap Opt containing None value!");
  }

  constexpr const T& ref_unchecked() const& noexcept { return this->somev; }

  /// @breif copies inner value to caller, if any
  constexpr T unwrap() const& noexcept {
    if (this->is_some()) [[likely]] {
      return this->somev;
    }

    LOG_FATAL("Cannot unwrap Opt containing None value!");
  }

  /// @brief compile-time version of [Opt<T>::unwrap]
  /// @details there is no compile-time version of [Opt<T>::expect],
  /// as the check in this method is done at compile-time
  consteval T cunwrap() const& noexcept {
    if constexpr (this->cis_some()) [[likely]] {
      return this->somev;
    } else {
      // NOTE: we just want to trigger compile error/failure, so this throw
      // never actually happens or ever gets compiled into resulting binary
      throw "cannot unwrap Opt with None value!";
    }
  }

  /// @brief returns pointer to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr T* operator->() noexcept { return &this->somev; }

  /// @brief returns reference to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr T& operator*() noexcept { return &this->somev; }

  /// @brief equality comparison between 2 Opt<T>.
  /// @details for equality, both must be is_none() or is_some(), if both are is_some(),
  /// each inner value is compared with its operator==
  constexpr bool operator==(const Opt<T>& other) const noexcept {
    if (this == &other) {
      return true;
    }
    if (this->is_none() && other.is_none()) {
      return true;
    }
    if (this->is_some() && other.is_some()) {
      return this->ref_unchecked() == other.ref_unchecked();
    }
    return false;
  }

  /// @brief @see [Opt<T>::operator==]
  constexpr bool operator!=(const Opt<T>& other) const noexcept { return !(*this == other); }

  /// @brief uses given function callback to map value of T to value of U, if any
  /// @details if inner value is None, then a new Opt<U>::none is returned
  template <typename U, typename F>
  constexpr Opt<U> map(F func) const noexcept {
    if (this->is_some()) [[likely]] {
      U val = func(this->ref_unchecked());
      return Opt<U>::some(val);
    }
    return Opt<U>::none();
  }
};

/// @brief alias for an optional pointer.
/// @details if pointer is null, Opt should be none,
/// otherwise Opt should contain a valid, non-null pointer
template <typename T>
using OptPtr = Opt<T*>;

/// @brief alias for const reference to T.
template <typename T>
using OptRef = Opt<const T&>;

/// @brief constructor(factory) function for Opt<T> with T val
/// @details for a version that creates an Opt<T>::none, @see [None]
template <typename T>
constexpr Opt<T> Some(T val) noexcept {
  return Opt<T>::some(val);
}

/// @brief constructor(factory) function for Opt<T> with no value
/// @details for a version that creates an Opt<T>::none, @see [None]
template <typename T>
constexpr Opt<T> None() noexcept {
  return Opt<T>::none();
}

}  // namespace nv::nv

#endif  // ifdef __cplusplus

#ifndef __cplusplus

#error "libnv cxx module opt.hpp can only be included/used from C++!";

#endif
