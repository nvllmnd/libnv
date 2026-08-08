#pragma once

#include "nonstd/expected.hpp"

namespace nv::result {

using nonstd::make_error;
template <class T, class E>
using Result = nonstd::expected<T, E>;

}  // namespace nv::result
