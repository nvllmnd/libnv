#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

#include "nv/core/debug.h"
#include "nv/core/intdefs.h"
#include "defer.hpp"
#include "core/opt.hpp"
#include "core/slice.hpp"

#ifdef __cplusplus

// namespace nv

namespace nv::alloc {

/// @brief type alias for a (potentially owned) contiguous block of T
template <typename T>
using Mem = nv::slice::Slice<T>;

/// @brief type alias for a contiguous block of bytes
using MemBytes = nv::slice::Slice<byte>;

struct Layout {
  u64 size;
  u64 align;
};

template <typename T>
static constexpr Layout layout_of = Layout{.size = sizeof(T), .align = alignof(T)};

template <typename T, isize N>
static constexpr Layout layout_of_array = Layout{.size = static_cast<u64>(sizeof(T) * N), .align = alignof(T)};

template <isize N>
static constexpr Layout layout_of_bytes = Layout{.size = static_cast<u64>(N), .align = 1};

static constexpr Layout layout_bytes(isize n) noexcept { return {.size = static_cast<u64>(n), .align = 1}; }

template <typename T>
static constexpr Layout layout_array_of(isize n) noexcept {
  return {.size = static_cast<u64>(n * sizeof(T)), .align = alignof(T)};
}

template <typename T>
static constexpr nv::opt::Opt<Layout> layout_try_extend(Layout self, isize count) noexcept {
  const u64 new_size = (static_cast<u64>(count) * sizeof(T)) + self.size;
  const u64 align = std::max(alignof(T), self.align);
  if (new_size >= INT64_MAX) {
    return nv::opt::None;
  }

  return Layout{.size = new_size, .align = align};
}

template <typename T>
static constexpr Layout layout_extend(Layout self, isize count) noexcept {
  return layout_try_extend<T>(self, count).unwrap();
}

template <typename T>
concept Allocate = requires(T alloc, Layout layout, MemBytes mem) {
  { alloc.allocate(layout) } -> std::same_as<MemBytes>;
  alloc.free(mem, layout);
};

template <typename T>
concept Realloc = requires(T alloc, Layout old_layout, Layout new_layout, MemBytes mem) {
  { alloc.reallocate(mem, old_layout, new_layout) } -> std::same_as<MemBytes>;
} && Allocate<T>;

template <typename T>
concept Resize = requires(T alloc, Layout old_layout, Layout new_layout, MemBytes mem) {
  { alloc.resize(mem, old_layout, new_layout) } -> std::same_as<bool>;
} && Allocate<T>;

template <typename T>
concept Allocator = Allocate<T> || Realloc<T> || Resize<T>;

}  // namespace nv::alloc

#endif

#ifndef __cplusplus

#error "libnv cxx module alloc.hpp can only be included/used by C++!";

#endif  // #ifndef __cplusplus
