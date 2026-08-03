#pragma once

#include <optional>

namespace nv::opt {

using std::make_optional;
using std::nullopt;
using std::optional;

using NoneType = std::nullopt_t;

inline constexpr NoneType None = std::nullopt;

template <class T>
using Opt = std::optional<T>;

template <class T>
constexpr Opt<T> make_opt(T val) noexcept {
  return std::make_optional<T>(val);
}

}  // namespace nv::opt
