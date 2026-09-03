#pragma once

#include "nvxx/alloc.hpp"
namespace nv {

template <class Alloc>
  requires(IsVoid<Alloc> || IsAllocatorStruct<Alloc> || AllocatorTraits<Alloc>)
struct String {
  char* data;
  isize len;
  Alloc alloc;
};

template <>
struct String<Allocator> {
  char* data;
  isize len;
  Allocator alloc;
};

template <>
struct String<void> {
  using Self = String<void>;
  char* data;
  isize len;

  static constexpr Self cons(isize len, Allocator alloc) noexcept {
    if (const auto _opt = (alloc.template alloc_array<char>(len)))
      for (auto [val, _running] = std::tuple{_opt.unwrap(), true}; _running; _running = false) {
      }

    // if (char* ptr = alloc.alloc_array<char>(len)) {
    //
    // }
  }
};

}  // namespace nv
