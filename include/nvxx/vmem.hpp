#pragma once

#include "nvxx/alloc.hpp"

namespace nv {

struct PageAlloc {
  static AllocResult alloc(Layout layout) noexcept;

  static Memory alloc_expect(Layout layout) noexcept { return PageAlloc::alloc(layout).unwrap(); }

  [[gnu::nonnull]]
  static bool resize(void* ptr, Layout old_layout, Layout new_layout) noexcept;
  [[gnu::nonnull]]
  static AllocResult realloc(void* ptr, Layout old_layout, Layout new_layout) noexcept;
  [[gnu::nonnull]]
  static void free(void* ptr, Layout layout) noexcept;

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
