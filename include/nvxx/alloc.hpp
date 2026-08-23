#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <type_traits>
#include "opt.hpp"
#include "slice.hpp"
#include "nv/memory/alloc.h"
#include "result.hpp"

#ifdef __cplusplus

namespace nv {

// namespace nv namespace nv::inline alloc {
/// @brief type alias for a (potentially owned) contiguous block of T
template <typename T>
using Mem = nv::slice::Slice<T>;

/// @brief type alias for a contiguous block of bytes
using MemBytes = nv::slice::Slice<byte>;

using AllocResult = nv::result::Result<MemBytes, nv::result::Error>;

template <class T>
concept Allocate = requires(T a, Layout l) {
  { a.alloc(l) } noexcept -> std::convertible_to<AllocResult>;
};

template <class T>
concept Free = requires(T a, void* p, Layout layout) {
  { a.free(p, layout) } noexcept -> std::same_as<void>;
};

template <class T>
concept BasicAllocator = Allocate<T> && Free<T>;

template <class T>
concept Reallocate = requires(T alloc, void* p, Layout l) {
  { alloc.realloc(p, l, l) } noexcept -> std::convertible_to<AllocResult>;
} && BasicAllocator<T>;

template <class T>
concept Resize = requires(T alloc, void* p, Layout l) {
  { alloc.resize(p, l, l) } noexcept -> std::same_as<bool>;
} && BasicAllocator<T>;

template <class T>
concept AllocatorTraits = BasicAllocator<T> || Reallocate<T> || Resize<T>;

template <class T>
  requires(std::is_unbounded_array_v<typename T::FlexMemberType>)
constexpr Layout layout_of_flex_array(isize count) noexcept {
  return {.size = sizeof(T) + (sizeof(typename T::FlexValueType) * count), .align = alignof(T)};
}

template <typename T>
constexpr Layout layout_of = Layout{.size = sizeof(T), .align = alignof(T)};

template <typename T, isize N>
constexpr Layout layout_of_array = Layout{.size = static_cast<u64>(sizeof(T) * N), .align = alignof(T)};

template <isize N>
constexpr Layout layout_of_bytes = Layout{.size = static_cast<u64>(N), .align = 1};

constexpr Layout layout_bytes(isize n) noexcept { return {.size = n, .align = 1}; }

template <typename T>
constexpr Layout layout_array_of(isize n) noexcept {
  return {.size = n * sizeof(T), .align = alignof(T)};
}

template <typename T>
constexpr nv::opt::Opt<Layout> layout_try_extend(Layout self, isize count) noexcept {
  const u64 new_size = (static_cast<u64>(count) * sizeof(T)) + self.size;
  const u64 align = std::max(alignof(T), static_cast<usize>(self.align));
  if (new_size >= INT64_MAX) {
    return nv::opt::None;
  }

  return Layout{.size = new_size, .align = align};
}

template <typename T>
constexpr Layout layout_extend(Layout self, isize count) noexcept {
  return layout_try_extend<T>(self, count).unwrap();
}

using AllocFunc = AllocResult (*const)(void* ctx, Layout layout) noexcept;
using ResizeFunc = bool (*const)(void* ctx, void* ptr, Layout old, Layout newl) noexcept;
using ReallocFunc = AllocResult (*const)(void* ctx, void* ptr, Layout old, Layout newl) noexcept;
using FreeFunc = void (*const)(void* ctx, void* ptr, Layout layout) noexcept;

struct AllocVtable {
  const AllocFunc alloc;
  const ReallocFunc realloc;
  const ResizeFunc resize;
  const FreeFunc free;
};

/// @brief a type-erased Allocator 'interface struct' inspirec by Zig
/// @details This is a C++ version of my C Allcoator 'interface struct', chose to go this route so
/// i dont have to use templates for all my basic container types
///
/// You can use [vtable_adapter] and/or [VtableAdapter] to create a compile time [AllocVtable] which will call T's
/// alloc,realloc,resize,and free if they are implemented on that type as methods
///
struct Allocator {
  void* const ctx;
  const AllocVtable* const vtable;

  template <class T>
  constexpr T* context() const noexcept
    requires(AllocatorTraits<T>)
  {
    return static_cast<T*>(this->ctx);
  }

  constexpr AllocResult allocate(Layout layout) const noexcept { return this->vtable->alloc(this->ctx, layout); }

  template <class T>
  constexpr result::Result<T*, nv::result::Error> allocate() const noexcept
    requires(std::is_standard_layout_v<T> && std::is_trivially_constructible_v<T>)
  {
    return static_cast<T*>(this->allocate(layout_of<T>));
  }

  template <class T>
  constexpr result::Result<T*, nv::result::Error> alloc_array(isize count) const noexcept
    requires(std::is_standard_layout_v<T> && std::is_trivially_constructible_v<T>)
  {
    return static_cast<T*>(this->allocate(layout_of_array_of<T>(count)));
  }

  template <class T, class... Args>
  constexpr result::Result<T*, nv::result::Error> construct(Args&&... args) const noexcept
    requires(std::is_constructible_v<T, Args...>)
  {
    if (auto res = this->allocate<T>(); res.has_value()) {
      T* ptr = *res;

      [[assume(ptr != nullptr)]];
      return std::construct_at(ptr, std::forward<Args>(args)...);
    } else {
      return res;
    }
  }

  template <class T>
  constexpr void destruct(T* val) const noexcept
    requires(std::is_destructible_v<T>)
  {
    std::destroy_at(val);
  }

  constexpr AllocResult reallocate(void* ptr, Layout old, Layout layout) const noexcept {
    return this->vtable->realloc(this->ctx, ptr, old, layout);
  }

  constexpr bool resize(void* ptr, Layout old, Layout layout) const noexcept {
    return this->vtable->resize(this->ctx, ptr, old, layout);
  }

  constexpr void free(void* ptr, Layout layout) const noexcept { this->vtable->free(this->ctx, ptr, layout); }

  constexpr bool has_state() const noexcept { return nv::is_not_null(this->ctx); }

  constexpr bool is_stateless() const noexcept { return !this->has_state(); }
};

template <class T>
concept IsAllocator = std::same_as<T, Allocator>;

template <class T>
concept AllocatorImpl = requires(T a) {
  { a.allocator() } noexcept -> std::convertible_to<Allocator>;
  { a.vtable() } noexcept -> std::convertible_to<const AllocVtable*>;
  { static_cast<Allocator>(a) } -> std::convertible_to<Allocator>;
} && AllocatorTraits<T>;

/// @breif Allocator Vtable Adapter generator
/// @details]
template <class T>
  requires(AllocatorTraits<T>)
struct VtableAdapter {
 private:
  static constexpr AllocResult alloc_impl(void* ctx, Layout layout) noexcept {
    auto* self = static_cast<T*>(ctx);
    return self->alloc(layout);
  }

  [[gnu::nonnull(2)]]
  static constexpr AllocResult realloc_impl(void* ctx, void* ptr, Layout old, Layout new_layout) noexcept {
    if constexpr (Reallocate<T>) {
      auto* self = static_cast<T*>(ctx);
      return self->realloc(ptr, old, new_layout);
    } else {
      return AllocResult{nv::result::Error::AllocITraitImplError};
    }
  }

  [[gnu::nonnull(2)]]
  static constexpr bool resize_impl(void* ctx, void* ptr, Layout old, Layout new_layout) noexcept {
    if constexpr (Resize<T>) {
      auto* self = static_cast<T*>(ctx);
      return self->resize(ptr, old, new_layout);
    } else {
      return false;
    }
  }

  static constexpr void free_impl(void* ctx, void* ptr, Layout layout) noexcept {
    auto* self = static_cast<T*>(ctx);
    self->free(ptr, layout);
  }

 public:
  static constexpr AllocVtable VT = {
      .alloc = alloc_impl,
      .realloc = realloc_impl,
      .resize = resize_impl,
      .free = free_impl,
  };

  [[gnu::returns_nonnull]]
  static consteval const AllocVtable* vtable() noexcept {
    return &VT;
  }
};

//
template <class T>
[[gnu::returns_nonnull]]
consteval const AllocVtable* vtable_adapter() noexcept
  requires(AllocatorTraits<std::remove_cvref_t<T>>)
{
  static constexpr AllocVtable VT = VtableAdapter<std::remove_cvref_t<T>>::VT;
  return &VT;
}

template <class T>
constexpr Allocator as_allocator() noexcept
  requires(AllocatorImpl<std::remove_cvref_t<T>>)
{
  return {.ctx = nullptr, .vtable = vtable_adapter<std::type_identity_t<T>>()};
}

template <class T>
constexpr Allocator as_allocator(T* ctx, const AllocVtable* vt = vtable_adapter<std::type_identity_t<T>>()) noexcept
  requires(AllocatorImpl<std::remove_cvref_t<T>>)
{
  return {.ctx = static_cast<void*>(ctx), .vtable = vt};
}

/// @brief a fixed sized, bump-style arena allocator
/// @details This is a lite cpp wrapper around the [Arena] C impl
struct Arena final {
  using CArena = ::Arena;

  CArena inner;

  [[gnu::always_inline]]
  constexpr AllocResult alloc(Layout layout) noexcept {
    if (auto* ptr = static_cast<byte*>(arena_alloc(&this->inner, layout))) [[likely]] {
      return nv::slice::slice_new(ptr, layout.size);
    }
    return nv::result::error_new(nv::result::Error::AllocITraitImplError);
  }

  [[gnu::always_inline]]
  constexpr AllocResult realloc(void* ptr, Layout old, Layout new_layout) noexcept {
    if (auto* res = static_cast<byte*>(arena_realloc(&this->inner, ptr, old, new_layout))) {
      return nv::slice::slice_new(res, new_layout.size);
    }
    return nv::result::error_new(nv::result::Error::AllocITraitImplError);
  }

  [[gnu::always_inline]]
  constexpr bool resize(void* ptr, Layout old, Layout new_layout) noexcept {
    return arena_resize(&this->inner, ptr, old, new_layout);
  }

  constexpr void free(void*, Layout) noexcept {}

  constexpr operator Allocator() noexcept { return this->allocator(); }

  [[gnu::returns_nonnull]]
  static constexpr const AllocVtable* vtable() noexcept {
    // vtable_adapter<ChunkArena>();
    return VtableAdapter<Arena>::vtable();
  }

  constexpr Allocator allocator() noexcept { return Allocator{.ctx = static_cast<void*>(this), .vtable = vtable()}; }

  constexpr usize available() const noexcept { return arena_avail(&this->inner); }
  constexpr usize used_bytes() const noexcept { return arena_used_bytes(&this->inner); }
};

[[gnu::pure, gnu::nonnull]]
constexpr Arena chunk_arena_new(byte* begin, byte* end) noexcept {
  const isize len = end - begin;
  return {.inner = arena_new(begin, len)};
}
[[gnu::pure]]
constexpr Arena chunk_arena_new(byte* begin, isize size_bytes) noexcept {
  return {.inner = arena_new(begin, size_bytes)};
}

template <usize N>
inline constexpr std::array<byte, N> StaticMemory = {};

template <usize N>
constexpr Arena chunk_arena_new(std::array<byte, N>* mem) noexcept {
  return chunk_arena_new(mem->begin(), mem->end());
}

}  // namespace nv
#endif

#ifndef __cplusplus

#error "libnv cxx module alloc.hpp can only be included/used by C++!";

#endif  // #ifndef __cplusplus
