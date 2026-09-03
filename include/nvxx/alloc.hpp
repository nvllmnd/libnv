#pragma once

#include <array>
#include <concepts>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>

#include "nv/memory/alloc.h"
#include "nvxx/common.hpp"
#include "opt.hpp"
#include "slice.hpp"

#ifdef __cplusplus

namespace nv {

template <class T>
  requires(std::is_unbounded_array_v<typename T::FlexMemberType>)
constexpr Layout layout_of_flex_array(isize count) noexcept {
  return {.size = sizeof(T) + (sizeof(typename T::FlexValueType) * count), .align = alignof(T)};
}

/// @brief used to notify caller about the next state after requested allocation in a bump/arena-style allocator
struct BumpState {
  /// @brief true when there is enough space for requested allocation size + alignment paddding (if any), otherwise
  /// false
  /// @details not having enough space is not considered an error condition
  bool is_enough_space;
  /// @brief pointer to where the next top pointer should be.
  byte* next_top;
  /// @brief pointer to new allocation
  /// @details may be null if allocation does not fit in current memory space, which is not considered an error
  void* allocation;
  /// @brief full size of the allocation in bytes, including padding taken for alignment
  isize alloc_size;
  // padding used for alignment
  isize padding;

  /// @brief size of the allocation, not including alignment padding
  /// @remarks the alloc_size field already includes the requested allocation size + alignemnt padding
  constexpr isize size_bytes() const noexcept { return this->alloc_size - this->padding; }
  constexpr bool is_empty() const noexcept { return is_null(this->next_top); }
  constexpr bool is_error() const noexcept {
    return is_not_null(this->next_top) && (is_null(this->allocation) || this->alloc_size <= 0);
  }

  template <class T>
  constexpr T* cast() const noexcept {
    return static_cast<T*>(this->allocation);
  }

  template <class T>
  [[gnu::returns_nonnull]]
  constexpr T* must_cast() const noexcept {
    if (this->allocation && (this->alloc_size - this->padding) == sizeof(T)) {
      return this->template cast<T>();
    }
    LOG_FATAL(
        "(padding - alloc_size): %d bytes for requested allocation of size: %li bytes does not match, or allocation "
        "pointer is null!");
  }

  template <class T>
  constexpr T* byte_cast() const noexcept {
    return nv::byte_cast<T>(static_cast<byte*>(this->allocation));
  }
};

using BumpResult = opt::NvResult<BumpState>;

[[gnu::nonnull, gnu::pure]]
inline BumpResult bump_alloc(byte* current_top, const byte* end, Layout layout) noexcept {
  if (end < current_top) [[unlikely]] {
    return opt::error_new(opt::Error::InvalidEndIterComesBeforeBegin);
  }
  const usize size_bytes = end - current_top;
  usize space = size_bytes;

  void* top = static_cast<void*>(current_top);
  if (std::align(layout.align, layout.size, top, space)) [[likely]] {
    const isize padding = size_bytes - space;
    return BumpState{.is_enough_space = true,
                     .next_top = static_cast<byte*>(top) + layout.size,
                     .allocation = top,
                     .alloc_size = padding + layout.size,
                     .padding = padding};
  }
  return BumpState{};
}

[[gnu::pure]]
inline BumpResult bump_alloc(slice::Slice<byte> mem_slice, Layout layout) noexcept {
  if (mem_slice.is_empty()) [[unlikely]] {
    return opt::error_new(opt::Error::InvalidEmptySlice);
  }
  return bump_alloc(mem_slice.begin(), mem_slice.cend(), layout);
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
  return {.size = n * static_cast<isize>(sizeof(T)), .align = static_cast<isize>(alignof(T))};
}

template <typename T>
constexpr nv::opt::Opt<Layout> layout_try_extend(Layout self, isize count) noexcept {
  const u64 new_size = (static_cast<u64>(count) * sizeof(T)) + self.size;
  const u64 align = std::max(alignof(T), static_cast<usize>(self.align));
  if (new_size >= INT64_MAX) {
    return nv::opt::None;
  }

  return Layout{.size = static_cast<isize>(new_size), .align = static_cast<isize>(align)};
}

template <typename T>
constexpr Layout layout_extend(Layout self, isize count) noexcept {
  return layout_try_extend<T>(self, count).unwrap();
}

constexpr Layout layout_expand(Layout self, isize count) noexcept {
  return {.size = self.size * count, .align = self.align};
}

constexpr Layout layout_extend_bytes(Layout self, isize count) noexcept {
  return layout_try_extend<byte>(self, count).unwrap();
}

template <class T>
concept MemoryResource = requires(T v, const void* ptr) {
  { v.begin() } noexcept -> std::convertible_to<typename T::ResourceIter>;
  { v.end() } noexcept -> std::convertible_to<typename T::ResourceIter>;
  { v.size_bytes() } noexcept -> std::convertible_to<usize>;
  { v.contains(ptr) } noexcept -> std::same_as<bool>;
  { std::addressof(v[0]) } noexcept -> std::convertible_to<::nv::ptr<ValType<typename T::ResourceType>>>;
} && Pod<ValType<typename T::ResourceType>>;

/// @brief A Typed pointer into a block of memory returned by an implementation of [Allocator]/[AllocatorTraits]
/// @details clients recieving this type from an allocation function are expected to pass this block back to its
/// allocator's free function, if necessary
/// This type is a Pod type and as such does not have move-semantics, and copying does not copy memory but just the
/// pointer to memory
///
/// @remarks this is a pointer-like type with the semantics of a raw pointer
template <class T>
  requires(Pod<T> || std::is_void_v<T>)
struct MemBlock {
  CONTAINER_TEMPLATE_TYPES(T);

  Pointer ptr;
  isize count;

  constexpr operator Str() const noexcept
    requires(std::same_as<Unwrap<T>, char> || std::same_as<ValType<T>, const char>)
  {
    return Str{this->begin(), this->len()};
  }

  constexpr bool is_scalar() const noexcept { return this->count == 1; }
  constexpr Pointer operator->() const noexcept
    requires(std::is_object_v<Unwrap<T>>)
  {
    return this->ptr;
  }
  constexpr Reference operator*() const noexcept { return *this->ptr; }
  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t index) noexcept { *(self.ptr + index); }

  constexpr Pointer begin() const noexcept { return this->ptr; }
  constexpr Pointer end() const noexcept { return this->ptr + this->len(); }

  constexpr ConstPointer cbegin() const noexcept { return this->ptr; }
  constexpr ConstPointer cend() const noexcept { return this->ptr + this->len(); }

  constexpr isize size_bytes() const noexcept { return this->count * sizeof(T); }
  constexpr isize len() const noexcept { return this->count; }
  constexpr isize align() const noexcept { return alignof(T); }

  template <class... Args>
  constexpr Pointer construct(Args&&... args) const noexcept {
    return std::construct_at(this->ptr, std::forward<Args>(args)...);
  }

  constexpr void destruct() const noexcept { std::destroy_at(this->ptr); }
};

#undef eprintln
#undef eprint

#undef println
#undef print

/// @brief A pointer into a block of memory returned by an implementation of [Allocator]/[AllocatorTraits]
/// @details clients recieving this type from an allocation function are expected to pass this block back to its
/// allocator's free function, if necessary
/// you can cast this block of memory to a typed version if you know the type ahead of time, or cast to bytes.
///
/// @remarks this is a pointer-like type with the semantics of a raw pointer
template <>
struct MemBlock<void> {
  void* ptr;
  Layout layout;

  constexpr isize size_bytes() const noexcept { return this->layout.size; }
  constexpr isize align() const noexcept { return this->layout.align; }

  constexpr byte* begin() const noexcept { return static_cast<byte*>(this->ptr); }
  constexpr byte* end() const noexcept { return this->begin() + this->layout.size; }

  constexpr const byte* cbegin() const noexcept { return static_cast<const byte*>(this->ptr); }
  constexpr const byte* cend() const noexcept { return this->cbegin() + this->layout.size; }

  template <class T>
  constexpr nv::opt::Opt<MemBlock<T>> try_cast_block() const noexcept {
    if (sizeof(T) != this->layout.size || alignof(T) != this->layout.align) [[unlikely]] {
      nv::eprintln("Memory block with: Layout(size: {}, align: {}) does not match sizeof(T): {} and alignof(T): {}",
                   this->size_bytes(), this->align(), sizeof(T), alignof(T));

      return opt::None;
    }

    return opt::Some(this->template cast_block<T>());
  }

  constexpr MemBlock<byte> cast_bytes() const noexcept {
    return {
        .ptr = this->begin(),
        .count = this->layout.size,
    };
  }

  template <class T>
  constexpr MemBlock<RemoveRef<T>> cast_block() const noexcept {
    assert_debug(sizeof(T) == this->size_bytes() && alignof(T) == this->align(),
                 "Memory block with: Layout(size: {}, align: {}) does not match sizeof(T): {} and alignof(T): {}",
                 this->size_bytes(), this->align(), sizeof(T), alignof(T));
    return {.ptr = static_cast<RemoveRef<T>*>(this->ptr), .layout = {.size = sizeof(T), .align = alignof(T)}};
  }

  template <class T>
  constexpr RemoveRef<T>* cast() const noexcept {
    return static_cast<RemoveRef<T>*>(this->ptr);
  }

  template <class T>
  constexpr RemoveRef<T>* get() const noexcept {
    return this->template cast<RemoveRef<T>>();
  }
};

/// @brief Opaque block of memory (type erased) returned from an allocation function/method
using Memory = MemBlock<void>;
/// @brief Block of uninitialized bytes returned from an allocation function/method
using ByteMemory = MemBlock<byte>;

template <class T>
constexpr Memory into(RemoveRef<T>* ptr) noexcept {
  return {.ptr = static_cast<void*>(ptr), .layout = layout_of<RemoveRef<T>>};
}

using AllocResult = nv::opt::Result<Memory, nv::opt::Error>;

template <class T>
concept Allocate = requires(T a, Layout l, isize count) {
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
concept StatelessAlloc = std::is_empty_v<T> && (BasicAllocator<T> || Reallocate<T> || Resize<T>);

template <class T>
concept AllocatorTraits = BasicAllocator<T> || Reallocate<T> || Resize<T> || StatelessAlloc<T>;

/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). Required
using AllocFunc = AllocResult (*const)(void* ctx, Layout layout) noexcept;
/// @brief Resize callback/function pointer, used in [AllocVtable] (and thus [Allocator]). optional.
using ResizeFunc = bool (*const)(void* ctx, void* ptr, Layout old, Layout newl) noexcept;
/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). optional
using ReallocFunc = AllocResult (*const)(void* ctx, void* ptr, Layout old, Layout newl) noexcept;
/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). Required
using FreeFunc = void (*const)(void* ctx, void* ptr, Layout layout) noexcept;

/// @brief comptime table of allocation function pointers used by [Allocator] (and its implementations)
struct AllocVtable final {
  /// @see [AllocFunc]
  const AllocFunc alloc;
  /// @see [ReallocFunc]
  const ReallocFunc realloc;
  /// @see [ResizeFunc]
  const ResizeFunc resize;
  /// @see [FreeFunc]
  const FreeFunc free;
};

/// @brief a type-erased Allocator 'interface struct' inspirec by Zig
/// @details This is a C++ version of my C Allcoator 'interface struct', chose to go this route so
/// i dont have to use templates for all my basic container types
///
/// You can use [vtable_adapter] and/or [VtableAdapter] to create a compile time [AllocVtable] which will call T's
/// alloc,realloc,resize,and free if they are implemented on that type as methods
///
struct Allocator final {
  template <class T>
  using Result = opt::Result<MemBlock<RemoveRef<T>>, opt::Error>;

  void* const ctx;
  const AllocVtable* const vtable;

  template <class T>
  constexpr RemoveRef<T>* context() const noexcept
    requires(AllocatorTraits<T>)
  {
    return static_cast<RemoveRef<T>*>(this->ctx);
  }

  constexpr AllocResult allocate(Layout layout) const noexcept { return this->vtable->alloc(this->ctx, layout); }

  template <class T>
  constexpr Result<T> allocate() const noexcept
    requires(Pod<T>)
  {
    return static_cast<T*>(this->allocate(layout_of<T>));
  }

  template <class T>
  constexpr Result<T> alloc_array(isize count) const noexcept
    requires(Pod<T>)
  {
    return this->allocate(layout_array_of<T>(count))
        .expect("Allocation of size {} bytes failed!", sizeof(T) * count)
        .template cast_block<T>();
  }

  template <class T, class... Args>
  constexpr Result<T> construct(Args&&... args) const noexcept
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
concept IntoAllocator = requires(T a) {
  { a.allocator() } noexcept -> std::convertible_to<Allocator>;
  { static_cast<Allocator>(a) } noexcept -> std::same_as<Allocator>;
};

template <class T>
concept AllocatorVtable = requires {
  { T::vtable() } noexcept -> std::convertible_to<const AllocVtable*>;
};

template <class T>
concept IsAllocatorStruct = std::same_as<T, Allocator>;

template <class T>
concept AllocatorImpl = AllocatorVtable<T> && IntoAllocator<T> && AllocatorTraits<T>;

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
  requires(AllocatorTraits<Unref<T>>)
{
  static constexpr AllocVtable VT = VtableAdapter<Unref<T>>::VT;
  return &VT;
}

/// @brief more explicitly named version of [nv::allocator] overload that takes a single optional param
template <class T>
  requires(AllocatorTraits<Unref<T>>)
constexpr Allocator stateless_allocator(const AllocVtable* vt = vtable_adapter<T>()) noexcept {
  assert_debug(is_not_null(vt), "Pointer for Allocator interface struct vtable must not be null!");
  return {.ctx = nullptr, .vtable = vt};
}

template <class T>
[[gnu::nonnull(2)]]
constexpr Allocator allocator(T* ctx, const AllocVtable* vt = vtable_adapter<std::type_identity_t<T>>()) noexcept
  requires(AllocatorImpl<Unref<T>>)
{
  assert_debug(is_not_null(vt), "Pointer for Allocator interface struct vtable must not be null!");
  return {.ctx = static_cast<void*>(ctx), .vtable = vt};
}

template <class T>
[[gnu::nonnull]]
constexpr Allocator allocator(const AllocVtable* vt = vtable_adapter<T>()) noexcept
  requires(AllocatorTraits<Unref<T>>)
{
  return nv::allocator<T>(nullptr, vt);
}

template <isize N>
using StaticMemory = std::array<byte, N>;

/// @brief a fixed sized, bump-style arena allocator
/// @details This is a lite cpp wrapper around the [Arena] C impl
struct BufferAlloc final {
  using CArena = ::NvArena;
  using Self = BufferAlloc;

  CArena inner;

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  [[gnu::pure, gnu::nonnull]]
  static constexpr BufferAlloc cons(byte* begin, byte* end) noexcept {
    const isize len = end - begin;
    return {.inner = arena_new(begin, len)};
  }

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  [[gnu::pure]]
  static constexpr BufferAlloc cons(byte* begin, isize size_bytes) noexcept {
    return {.inner = arena_new(begin, size_bytes)};
  }

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  template <usize N>
  constexpr BufferAlloc cons(StaticMemory<N>* mem) noexcept {
    return Self::cons(mem->begin(), mem->end());
  }

  [[gnu::always_inline]]
  constexpr AllocResult alloc(Layout layout) noexcept {
    if (auto* ptr = arena_alloc(&this->inner, layout)) [[likely]] {
      return Memory{.ptr = ptr, .layout = layout};
    }
    return opt::error_new(opt::Error::AllocITraitImplError);
  }

  [[gnu::always_inline]]
  constexpr AllocResult realloc(void* ptr, Layout old, Layout new_layout) noexcept {
    if (auto* res = arena_realloc(&this->inner, ptr, old, new_layout)) {
      return Memory{.ptr = res, .layout = new_layout};
    }
    return opt::error_new(opt::Error::AllocITraitImplError);
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
    return VtableAdapter<BufferAlloc>::vtable();
  }

  constexpr Allocator allocator() noexcept { return Allocator{.ctx = static_cast<void*>(this), .vtable = vtable()}; }

  constexpr usize available() const noexcept { return arena_avail(&this->inner); }
  constexpr usize used_bytes() const noexcept { return arena_used_bytes(&this->inner); }
};

template <class T>
concept MemResourceAlloc = AllocatorTraits<T> && MemoryResource<typename T::Resource>;

}  // namespace nv
#endif

#ifndef __cplusplus

#error "libnv cxx module alloc.hpp can only be included/used by C++!";

#endif  // #ifndef __cplusplus
