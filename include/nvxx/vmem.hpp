#pragma once

#include "nvxx/alloc.hpp"

namespace nv {

struct PageAlloc {
  static AllocResult alloc(Layout layout) noexcept;

  static inline AllocResult alloc(isize size, isize align) noexcept {
    return PageAlloc::alloc(Layout{.size = size, .align = align});
  }

  [[gnu::nonnull]]
  static bool resize(void* ptr, Layout old_layout, Layout new_layout) noexcept;

  [[gnu::nonnull]]
  static inline bool resize(void* ptr, isize old_size, isize new_size) noexcept {
    return PageAlloc::resize(ptr, Layout{old_size, 1}, Layout{new_size, 1});
  }

  [[gnu::nonnull]]
  static AllocResult realloc(void* ptr, Layout old_layout, Layout new_layout) noexcept;

  [[gnu::nonnull]]
  static inline AllocResult realloc(void* ptr, isize old_size, isize new_size) noexcept {
    return PageAlloc::realloc(ptr, Layout{old_size, 1}, Layout{new_size, 1});
  }

  [[gnu::nonnull]]
  static void free(void* ptr, Layout layout) noexcept;

  [[gnu::nonnull]]
  static inline void free(void* ptr, isize size) noexcept {
    return PageAlloc::free(ptr, Layout{size, 1});
  }

  constexpr operator Allocator() const noexcept { return nv::allocator<PageAlloc>(); }

  [[gnu::returns_nonnull]]
  static constexpr const AllocVtable* vtable() noexcept {
    return vtable_adapter<PageAlloc>();
  }
  static constexpr Allocator allocator() noexcept { return nv::allocator<PageAlloc>(); }
};

template struct VtableAdapter<PageAlloc>;

inline constexpr PageAlloc page_allocator{};

}  // namespace nv
