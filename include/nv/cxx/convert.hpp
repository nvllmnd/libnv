#pragma once

#include "opt.hpp"
#include "result.hpp"
namespace nv::conv {

template <typename T, typename E>
constexpr nv::opt::Opt<T> into(result::Result<T, E> res) noexcept {
  using namespace nv::opt;
  if (res.is_ok()) {
    return Opt<T>::some(res.unwrap());
  }
  return Opt<T>::none();
}

template <typename T, typename E>
constexpr result::Result<T, E> into(nv::opt::Opt<T> opt, E err) noexcept {
  if (opt.is_some()) {
    return result::Result<T, E>::ok(opt.runwrap());
  }
  return result::Result<T, E>::error(err);
}

}  // namespace nv::conv
