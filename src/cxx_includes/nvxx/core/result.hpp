#pragma once

#include "nonstd/expected.hpp"

namespace nv::result {
using nonstd::expected;
using nonstd::make_error;
using nonstd::unexpect_t;

template <class T, class E>
using Result = nonstd::expected<T, E>;

using Error = nonstd::unexpect_t;

}  // namespace nv::result
