#pragma once

#include <optional>
#include <type_traits>

namespace nv::opt {
using std::make_optional;
using std::nullopt;
using std::optional;

using NoneType = std::nullopt_t;

inline constexpr NoneType None = std::nullopt;

template <class T>
  requires(!std::is_reference_v<T>)
using Opt = std::optional<T>;

template <class T>
constexpr auto make_opt = std::make_optional<T>;
}  // namespace nv::opt
