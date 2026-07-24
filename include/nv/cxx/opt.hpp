#pragma once

#include <type_traits>

#include "nv/core/log.h"

#ifdef __cplusplus

namespace nv::opt {

struct NoneType {
  enum struct Id { Token };

  constexpr NoneType(Id) noexcept {}
};

inline constexpr NoneType None = NoneType{NoneType::Id::Token};

/// @brief an Optional value over T, or None (Opt<T>::None)
template <typename T>
struct Opt {
  using Some = T;

  consteval Opt() noexcept : nonev(None), has_some(false) {}
  constexpr Opt(NoneType) noexcept : nonev(None), has_some(false) {}
  constexpr explicit Opt(const T& val) noexcept : somev(val), has_some(true) {}

  // constexpr ~Opt() noexcept
  //   requires(!std::is_trivially_destructible_v<T>)
  // {}

  constexpr Opt<T>& operator=(const T& val) noexcept {
    if (this->is_some() && this->somev != val) {
      this->somev = val;
    }
    return *this;
  }

  constexpr Opt<T>& operator=(const NoneType& nv) noexcept
    requires std::is_trivially_destructible_v<T>
  {
    this->nonev = nv;
    return *this;
  }

  constexpr operator bool() const noexcept { return this->is_some(); }

  constexpr bool is_some() const noexcept { return this->has_some; }

  constexpr bool is_none() const noexcept { return !this->has_some; }

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

  /// @brief returns pointer to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr const T* operator->() const& noexcept { return &this->somev; }

  /// @brief returns reference to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr const T& operator*() const& noexcept { return &this->somev; }

  /// @brief returns pointer to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr T* operator->() & noexcept { return &this->somev; }

  /// @brief returns reference to inner some value, if any
  /// @detail if this is called when there is no value in this option, behavior is undefined
  constexpr T& operator*() & noexcept { return &this->somev; }

  constexpr bool operator==(const NoneType&) const noexcept { return this->is_none(); }
  constexpr bool operator!=(const NoneType&) const noexcept { return !this->is_none(); }

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
  constexpr Opt<U> map(F func) const noexcept
    requires(std::is_invocable_v<F, const U&>)
  {
    if (this->is_some()) [[likely]] {
      U val = func(this->ref_unchecked());
      return Opt<U>::some(val);
    }
    return opt::None;
  }

  template <typename U>
  friend struct Opt;

 private:
  union {
    NoneType nonev;
    T somev;
  };

  bool has_some : 1;
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
constexpr Opt<T> Some(const T& val) noexcept {
  return Opt<T>{val};
}

/// @brief constructor(factory) function for Opt<T> with no value
/// @details for a version that creates an Opt<T>::none, @see [None]

// template <typename T>
// constexpr Opt<T> None;

}  // namespace nv::opt

#endif  // ifdef __cplusplus

#ifndef __cplusplus

#error "libnv cxx module opt.hpp can only be included/used from C++!";

#endif
