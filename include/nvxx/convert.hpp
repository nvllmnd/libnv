#pragma once

#include "opt.hpp"
#include "result.hpp"

#ifdef __cplusplus

namespace nv::conv {

template <typename T, class E>
constexpr nv::opt::Opt<T> into(nv::result::Result<T, E> res) noexcept {
  if (res.has_value()) {
    return res.value();
  }
  return nv::opt::None;
}

}  // namespace nv::conv

#endif

#ifndef __cplusplus
#error "libnv cxx module result.hpp can only be included/used from C++!";
#endif
