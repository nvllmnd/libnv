#pragma once
#include <concepts>
#include <expected>
#include <type_traits>
#include <utility>

#include "nv/core/log.h"
#include "opt.hpp"

#ifdef __cplusplus

namespace nv::result {

/// @brief used to disambiguate between constructing a Result<T,E> for Error type E and the
/// constructor for the inner template type E constructor
template <class E>
struct Error {
  using Type = E;

  E err;
};

template <typename E>
Error(const E&) -> Error<E>;

template <typename E>
constexpr Error<E> make_error(const E& e) noexcept {
  return Error<E>{e};
}

/// @brief a discrimnated union over T, or a specified error value E
/// @details T and E must satisfy std::is_standard_layout and std::is_trivial, as this type
/// does not know or care about complex RAII semantics, ZII > RAII anyway >:)
///
/// Keep in mind that although this type can be created trivially, it doesnt really make sense to do so, unless you want
/// to create a Result<T, E> with a zeroed Ok value
template <typename T, typename E = u64>
struct [[nodiscard]] Result {
  using OkType = T;
  using ErrorType = Error<E>;
  using InnerErrorType = Error<E>::Type;

  /// @brief flags wether this Result is an error result or has an ok value
  /// @remarks this is called is_error (as opposed to is_ok) to make zeroed instantiaions of this type valid,
  /// as such zeroed Results are the same as doing Result<T,E>::ok(T{}):
  constexpr explicit Result(const T& val) noexcept : is_error(false), okv(val) {}
  // constexpr explicit Result(const E& val) noexcept : is_error(true), errv(Error<E>{val}) {}

  constexpr Result(const Error<E>& err) noexcept : is_error(true), errv(err) {}
  //
  // constexpr Result(const Result<T, E>& other) noexcept = default;

  constexpr operator bool() const noexcept { return this->is_ok(); }

  constexpr bool operator==(const Result<T, E>& other) const noexcept {
    if (this == &other) {
      return true;
    }

    if (this->is_err() && other.is_err()) {
      return this->errv == other.errv;
    }
    if (this->is_ok() && other.is_ok()) {
      return this->okv == other.okv;
    }

    return false;
  }

  constexpr bool operator!=(const Result<T, E>& other) const noexcept { return !(*this == other); }

  constexpr T* operator->() & noexcept {
    assert_debug(this->is_ok(),
                 "dereferencing a Result that contains an error value is UB! This check is only done in debug builds "
                 "BE CAREFUL and have fun! :D");
    return &this->okv;
  }
  constexpr const T* operator->() const& noexcept {
    assert_debug(this->is_ok(),
                 "dereferencing a Result that contains an error value is UB! This check is only done in debug builds "
                 "BE CAREFUL and have fun! :D");
    return &this->okv;
  }

  constexpr T& operator*() & noexcept {
    assert_debug(this->is_ok(),
                 "dereferencing a Result that contains an error value is UB! This check is only done in debug builds "
                 "BE CAREFUL and have fun! :D");
    return this->okv;
  }

  constexpr const T& operator*() const& noexcept {
    assert_debug(this->is_ok(),
                 "dereferencing a Result that contains an error value is UB! This check is only done in debug builds "
                 "BE CAREFUL and have fun! :D");
    return this->okv;
  }

  /// @brief creates Result<T> with given value
  /// @details Result is considered to not have an error, for a version that creates a Result<T, E> with an error value,
  /// @see [Result<T,E>::error]
  // static constexpr Result<T, E> Ok(const T& val) noexcept { return Result<T, E>{val}; }

  /// @brief creates Result<T> with given error
  /// @details Result is considered to have an error, for a version that creates a Result<T, E> with an ok value,
  /// @see [Result<T,E>::ok]
  // static constexpr Result<T, E> Error(const E& err) noexcept { return Result<T, E>{err}; }

  constexpr bool is_ok() const noexcept { return !this->is_error; }

  constexpr bool is_err() const noexcept { return this->is_error; }

  /// @brief returns copy of inner Ok value, if any
  /// @details this method will abort execution if it conatins an error value
  /// for a version of this method that unwraps inner error value @see [Result<T,E>::unwrap_err]
  constexpr T unwrap() const& noexcept {
    if (this->is_ok()) [[likely]] {
      return this->okv;
    }
    LOG_FATAL("Cannot unwrap Result containing Error value!");
  }

  /// @brief same as calling [Result<T, E>::unwrap], but does no checking
  /// @brief calling this method on a Result<T,E> that contains an error is UB
  constexpr T unwrap_unchecked() const& noexcept { return this->okv; }

  /// @brief same as calling [Result<T, E>::unwrap_err], but does no checking
  /// @brief calling this method on a Result<T,E> that contains a value is UB
  constexpr E unwrap_err_unchecked() const& noexcept { return this->errv; }

  /// @brief returns copy of inner Err value, if any
  /// @details this method will abort execution if it conatins an ok value
  /// for a version of this method that unwraps inner ok value @see [Result<T,E>::unwrap]
  constexpr E unwrap_err() const& noexcept {
    if (this->is_err()) [[likely]] {
      return this->errv;
    }

    LOG_FATAL("Cannot unwrap_err Result containing Ok value!");
  }

  /// @brief converts a Result<T,E> to a Opt<T>, discarding the Error value, if any
  constexpr nv::opt::Opt<T> into_opt() const& noexcept {
    using namespace nv::opt;
    if (this->is_ok()) [[likely]] {
      return Some(this->okv);
    }
    return None;
  }

  /// @brief same as callilng [Result<T, U>::value]
  constexpr const T& ref_unchecked() const noexcept { return this->value(); }

  /// @brief returns inner ok value if there is one
  /// @details this method will abort execution if it contains an error value
  constexpr const T& as_ref() const noexcept {
    if (this->is_ok()) [[likely]] {
      return this->value();
    }

    LOG_FATAL("Cannot get inner reference to Result containing Error value!");
  }

  /// @brief returns inner value without checking if it contains one
  /// @details if this Result contains an error value, calling this method is UB
  constexpr const T& value() const noexcept { return this->okv; }

  /// @brief returns inner error without checking if it contains one
  /// @details if this Result contains an ok value, calling this method is UB
  constexpr const E& err_value() const noexcept { return this->errv; }

  /// @brief uses given function to map a Result<T, E> to a Result<U,E>,
  /// @details leaves error value alone, if any
  template <typename U, typename F>
  constexpr Result<U, E> map(F func) const noexcept {
    if (this->is_ok()) [[likely]] {
      U val = func(this->ref_unchecked());
      return Result<U, E>::Ok(val);
    }
    return Result<U, E>::err(this->unwrap_err());
  }

  /// @brief uses given function to map a Result<T, E> to a Result<T,V>,
  /// @details leaves ok value alone, if any
  template <typename V, typename F>
  constexpr Result<T, V> map_err(F func) const noexcept {
    if (this->is_err()) [[likely]] {
      E val = func(this->unwrap_err_unchecked());
      return Result<T, V>::err(val);
    }
    return Result<T, V>::Ok(this->unwrap_unchecked());
  }

 private:
  union {
    T okv;
    Error<E> errv;
  };
  bool is_error : 1;

  constexpr Result() = delete;
  constexpr Result(const Result& other) noexcept = default;
};

/// @brief ctor function for helping deduce template arguments for an Ok Result<T,E>
/// @detail for a version that constructs [Opt]s of T, @see [Some]
template <typename T, typename E>
constexpr Result<T, E> Ok(const T& val) noexcept {
  return Result{val};
}

/// @brief ctor function for helping deduce template arguments for an Ok Result<T,E>
/// @detail for a version that constructs [Opt]s of T, @see [None]
template <typename T, typename E>
constexpr Result<T, E> Err(const E& err) noexcept {
  return Result{make_error(err)};
}

}  // namespace nv::result

#endif

#ifndef __cplusplus
#error "libnv cxx module result.hpp can only be included/used from C++!";
#endif
