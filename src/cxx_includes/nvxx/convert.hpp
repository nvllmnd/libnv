#pragma once

#include "core/opt.hpp"
#include "core/result.hpp"

#ifdef __cplusplus

namespace nv::conv {

template <typename T, typename E>
constexpr nv::opt::Opt<T> into(nv::result::Result<T, E> res) noexcept {
  if (res.has_value()) {
    return res.value();
  }
  return nv::opt::None;
}

template <typename T, typename E>
constexpr nv::result::Result<T, E> into(nv::opt::Opt<T> opt, E err) noexcept {
  if (opt.has_value()) {
    return opt.value();
  }
  return nv::result::make_error(err);
}

}  // namespace nv::conv

#endif

#ifndef __cplusplus
#error "libnv cxx module result.hpp can only be included/used from C++!";
#endif
