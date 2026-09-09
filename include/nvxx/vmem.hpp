#pragma once

#include "nvxx/alloc.hpp"

namespace nv {

/// @brief stateless page allocator implementation
/// @details contrary to the C API, This Vmem does not contain state, but instead encapsulates allocating from mmap
struct Vmem {
  static constexpr auto DEFAULT_PAGE_ALIGN = 4096;

  static constexpr AllocResult alloc(Layout layout) noexcept { return Vmem::alloc(layout.size); }

  /// @brief second parameter is ignored, as mmap returns a page aligned pointer anyway
  static AllocResult alloc(isize size, isize = Vmem::DEFAULT_PAGE_ALIGN) noexcept;

  [[gnu::nonnull]]
  static bool resize(void* ptr, isize old_size, isize new_size) noexcept;

  [[gnu::nonnull]]
  static constexpr bool resize(void* ptr, Layout old_layout, Layout new_layout) noexcept {
    return Vmem::resize(ptr, old_layout.size, new_layout.size);
  }

  static constexpr AllocResult realloc(void* ptr, Layout old_layout, Layout new_layout) noexcept {
    return Vmem::realloc(ptr, old_layout.size, new_layout.size);
  }

  [[gnu::nonnull]]
  static AllocResult realloc(void* ptr, isize old_size, isize new_size) noexcept;

  static void free(void* ptr, isize size) noexcept;

  [[gnu::nonnull]]
  static constexpr void free(void* ptr, Layout layout) noexcept {
    Vmem::free(ptr, layout.size);
  }

  constexpr operator AllocContext() const noexcept { return nv::alloc_context<Vmem>(); }

  [[gnu::returns_nonnull]]
  static constexpr const AllocVtable* vtable() noexcept {
    return vtable_adapter<Vmem>();
  }
  static constexpr AllocContext context() noexcept { return nv::alloc_context<Vmem>(); }
};
using PageAlloc = AllocTraits<Vmem>;

// NOTE: PageAlloc, and therefor Vmem all always compare equal, as memory allocated by Vmem or PageAlloc can be freed by
// any Vmem or PageAlloc, as they just call mmap,mremap,munmap internally

constexpr bool operator==(const PageAlloc&, const PageAlloc&) noexcept { return true; }
constexpr bool operator!=(const PageAlloc&, const PageAlloc&) noexcept { return false; }
constexpr bool operator==(const Vmem&, const Vmem&) noexcept { return true; }
constexpr bool operator!=(const Vmem&, const Vmem&) noexcept { return false; }
constexpr bool operator==(const PageAlloc&, const Vmem&) noexcept { return true; }
constexpr bool operator!=(const PageAlloc&, const Vmem&) noexcept { return false; }
constexpr bool operator==(const Vmem&, const PageAlloc&) noexcept { return true; }
constexpr bool operator!=(const Vmem&, const PageAlloc&) noexcept { return false; }

template struct VtableAdapter<Vmem>;
template struct AllocTraits<Vmem>;

/// @brief alias for templated allocator(traits) api

/// @brief stateless instance for vmem allocator
inline constexpr Vmem vmem_allocator{};
/// @breif stateless instance for page allocator
/// @details most likely you will use [PageAlloc], as this allocator contains no state
inline constexpr PageAlloc page_alloc{};

}  // namespace nv
