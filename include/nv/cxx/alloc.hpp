#pragma once

#include <concepts>
#include <type_traits>

#include "nv/core/debug.h"
#include "nv/core/intdefs.h"
#include "opt.hpp"

#ifdef __cplusplus

namespace nv {

/// @brief type alias for a contiguous block of T
template <typename T>
using Mem = Slice<T>;

/// @brief type alias for a contiguous block of bytes
using MemBytes = Slice<byte>;

struct Layout {
  u64 size;
  u64 align;

  template <typename T>
  static consteval Layout make() noexcept {
    return {
        .size = sizeof(T),
        .align = alignof(T),
    };
  }

  template <typename T>
  static constexpr Layout array(isize size) noexcept {
    return {
        .size = static_cast<u64>(size <= 0 ? 1 : size),
        .align = alignof(T),
    };
  }

  template <typename T, isize COUNT>
  static consteval Layout array() noexcept {
    static_assert(COUNT >= 1,
                  "Layout::array<T> was given a negative literal for COUNT when a positive number was expected");
    static_assert(
        COUNT <= INT32_MAX,
        "Layout::array<T> was given a COUNT larger than INT32_MAX! provide a value smaller than that to compile!");
    static_assert(COUNT * sizeof(T) <= UINT64_MAX, "sizeof(T) * COUNT would overflow UINT64_MAX ");

    return {
        .size = sizeof(T) * COUNT,
        .align = alignof(T),
    };
  }

  static constexpr Layout bytes(isize size) noexcept { return Layout::array<u8>(size); }

  template <isize SIZE>
  static consteval Layout bytes() noexcept {
    return {
        .size = SIZE,
        .align = 1,
    };
  }

  template <typename T>
  static constexpr opt::Opt<Layout> try_extend(Layout self, isize count) noexcept {
    using namespace nv::opt;

    const u64 new_size = (static_cast<u64>(count) * sizeof(T)) + self.size;
    const u64 align = std::max(alignof(T), self.align);
    if (new_size >= INT64_MAX) {
      return Opt<Layout>::none();
    }

    return Opt<Layout>::some({.size = new_size, .align = align});
  }

  template <typename T>
  static constexpr Layout extend(Layout self, isize count) noexcept {
    return Layout::try_extend<T>(self, count).unwrap();
  }

  Layout extend_bytes(isize size) const noexcept { return Layout::extend<byte>(*this, size); }
};

template <typename T>
concept AllocateRaw = requires(T alloc, isize size, void* ptr) {
  { alloc.allocate_raw(size) } -> std::same_as<void*>;
  alloc.free_raw(ptr, size);
};

static_assert(std::is_standard_layout_v<Layout> && std::is_trivial_v<Layout>, "HUH");

template <typename T>
concept ReallocRaw = (requires(T alloc, isize old_size, isize new_size, void* ptr) {
                       { alloc.reallocate_raw(ptr, old_size, new_size) } -> std::same_as<void*>;
                     }) && AllocateRaw<T>;

template <typename T>
concept ResizeRaw = (requires(T alloc, isize old_size, isize new_size, void* ptr) {
                      { alloc.resize_raw(ptr, old_size, new_size) } -> std::same_as<bool>;
                    }) && AllocateRaw<T>;

template <typename T>
concept AllocatorRaw = AllocateRaw<T> || ReallocRaw<T> || ResizeRaw<T>;

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

}  // namespace nv

#endif

#ifndef __cplusplus

#error "libnv cxx module alloc.hpp can only be included/used by C++!";

#endif  // #ifndef __cplusplus
