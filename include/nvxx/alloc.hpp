#pragma once

#include <array>
#include <concepts>

#include <cstddef>
#include <memory>
#include <type_traits>

#include "nv/iter/iterators.h"
#include "nvxx/common.hpp"

#include "nv/memory/alloc.h"
#include "opt.hpp"
#include "slice.hpp"

#undef eprintln
#undef println
#undef eprint
#undef print

#ifdef __cplusplus

namespace nv {

inline constexpr auto DEFAULT_ALIGN = alignof(std::max_align_t);

template <class T>
  requires(std::is_unbounded_array_v<typename T::FlexMemberType>)
constexpr Layout layout_of_flex_array(isize count) noexcept {
  return {.size = sizeof(T) + (sizeof(typename T::FlexValueType) * count), .align = alignof(T)};
}

/// @brief used to notify caller about the next state after requested allocation in a bump/arena-style allocator
// struct BumpState {
//   /// @brief true when there is enough space for requested allocation size + alignment paddding (if any), otherwise
//   /// false
//   /// @details not having enough space is not considered an error condition
//   bool is_enough_space;
//   /// @brief pointer to where the next top pointer should be.
//   byte* next_top;
//   /// @brief pointer to new allocation
//   /// @details may be null if allocation does not fit in current memory space, which is not considered an error
//   void* allocation;
//   /// @brief full size of the allocation in bytes, including padding taken for alignment
//   isize alloc_size;
//
//   constexpr operator bool() const noexcept { return this->is_enough_space; }
//
//   /// @brief size of the allocation in bytes
//   constexpr isize size_bytes() const noexcept { return this->alloc_size; }
//   constexpr bool is_empty() const noexcept { return is_null(this->next_top); }
//   constexpr bool is_error() const noexcept {
//     return is_not_null(this->next_top) && (is_null(this->allocation) || this->alloc_size <= 0);
//   }
//
//   template <class T>
//   constexpr T* cast() const noexcept {
//     return static_cast<T*>(this->allocation);
//   }
//
//   /// @brief helpful for casting scalar types, but will fail at runtime for arrays
//   /// @details this->alloc_size must be equal to sizeof(T) or else runtime execution is aborted
//   template <class T>
//   [[gnu::returns_nonnull]]
//   constexpr T* must_cast() const noexcept {
//     if (this->allocation && (this->alloc_size == sizeof(T))) {
//       return this->template cast<T>();
//     }
//     LOG_FATAL(
//         "Type of size %li bytes does not match BumpState Allocation of size: %li bytes"
//         " or allocation pointer is null!",
//         this->alloc_size, sizeof(T));
//   }
//
//   template <class T>
//   constexpr T* byte_cast() const noexcept {
//     return nv::ptr_cast<T>(static_cast<byte*>(this->allocation));
//   }
// };
//
/// @brief cursor for a linear-bump-style allocator
/// @details C++ version of the Capi allocate_raw/realloc_raw,resize_raw/free_raw (which use IterByte)
struct BumpCursor {
  using IterValType = byte;
  using IterType = IterValType*;
  using ConstIterType = const IterValType*;

  IterByte self;

  [[gnu::nonnull, gnu::pure]]
  static constexpr BumpCursor cons(byte* begin, byte* end, byte* cursor) noexcept {
    return {.self = {.begin = begin, .end = end, .cursor = cursor}};
  }

  constexpr byte* top() const noexcept { return self.cursor; }
  constexpr const byte* ctop() const noexcept { return self.cursor; }

  constexpr byte* begin() const noexcept { return self.begin; }
  constexpr byte* end() const noexcept { return self.end; }

  constexpr const byte* cbegin() const noexcept { return self.begin; }
  constexpr const byte* cend() const noexcept { return self.end; }

  constexpr isize capacity() const noexcept { return this->end() - this->begin(); }

  constexpr isize used_bytes() const noexcept { return self.cursor - self.begin; }
  constexpr isize avail_bytes() const noexcept { return self.end - self.cursor; }

  explicit constexpr operator IterByte() const noexcept { return self; }

  [[gnu::alloc_size(2), gnu::alloc_align(3)]]
  constexpr void* allocate_raw(isize size, isize align) noexcept {
    void* ptr = ::allocate_raw(&self, Layout{size, align});
    return ptr;
  }

  constexpr bool resize_raw(void* ptr, isize oldsize, isize newsize) noexcept {
    const bool res = ::resize_raw(&self, ptr, Layout{oldsize, alignof(std::max_align_t)},
                                  Layout{newsize, alignof(std::max_align_t)});
    return res;
  }

  [[gnu::alloc_size(3), gnu::nonnull]]
  constexpr void* reallocate_raw(void* ptr, isize oldsize, isize newsize) noexcept {
    void* res = ::reallocate_raw(&self, ptr, Layout{oldsize, alignof(std::max_align_t)},
                                 Layout{newsize, alignof(std::max_align_t)});

    return res;
  }

  constexpr void free_raw(void*, isize) noexcept {}

  constexpr void free_raw_zeroed(void* ptr, isize size) noexcept {
    ::free_raw(&self, ptr, Layout{size, alignof(std::max_align_t)}, 0);
  }
};

template<class T>
concept ToBumpCursor = (requires(T alloc) {
  { alloc.bump_cursor() } noexcept -> std::convertible_to<BumpCursor>;
} || requires(T alloc) {
  { static_cast<BumpCursor>(alloc) } noexcept -> std::same_as<BumpCursor>;
});

template <class T>
concept LinearAlloc = requires(T alloc) {
  { alloc.used_bytes() } noexcept -> std::convertible_to<isize>;
  { alloc.avail_bytes() } noexcept -> std::convertible_to<isize>;
} && ToBumpCursor<T> && Cursorlike<T>;

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

using AllocResult = void*;

template <class T>
concept Allocate = requires(T a, isize old, isize align) {
  { a.alloc(old, align) } noexcept -> std::convertible_to<AllocResult>;
};

template <class T>
concept Free = requires(T a, void* p, isize size) {
  { a.free(p, size) } noexcept -> std::same_as<void>;
};

template <class T>
concept BasicAlloc = Allocate<T> && Free<T>;

template <class T>
concept Reallocate = requires(T alloc, void* p, isize old_size, isize new_size) {
  { alloc.realloc(p, old_size, new_size) } noexcept -> std::convertible_to<AllocResult>;
} && BasicAlloc<T>;

template <class T>
concept Resize = requires(T alloc, void* p, isize old_size, isize new_size) {
  { alloc.resize(p, old_size, new_size) } noexcept -> std::same_as<bool>;
} && BasicAlloc<T>;

namespace priv {

template <class T>
concept BaseAllocator = (BasicAlloc<T> || Reallocate<T> || Resize<T>);

}

template <class T>
concept EmptyAllocator = std::is_empty_v<T> && priv::BaseAllocator<T>;

template <class T>
concept StatefulAllocator = !std::is_empty_v<T> && priv::BaseAllocator<T>;

template <class T>
concept Allocator = StatefulAllocator<T> || EmptyAllocator<T>;

/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). Required
using AllocFunc = AllocResult (*const)(void* ctx, isize size, isize align) noexcept;
/// @brief Resize callback/function pointer, used in [AllocVtable] (and thus [Allocator]). optional.
using ResizeFunc = bool (*const)(void* ctx, void* ptr, isize old_size, isize new_size) noexcept;
/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). optional
using ReallocFunc = AllocResult (*const)(void* ctx, void* ptr, isize old_size, isize new_size) noexcept;
/// @brief Allocation callback/function pointer, used in [AllocVtable] (and thus [Allocator]). Required
using FreeFunc = void (*const)(void* ctx, void* ptr, isize size) noexcept;

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

/// @brief a type-erased Allocator 'interface struct' inspired by Zig
/// @details Contains allocator context/state (if any) and a pointer to
/// a (hopefully) static constexpr [AllocVtable] instance
///
/// This is a C++ version of my C Allocator 'interface struct', chose to go this route so
/// i dont have to use templates for all my basic container types
///
/// There is also a non-type erased version that is more type-save, inspired by [std::allocator_traits], @see
/// [nv::AllocTraits]
///
/// You can use [vtable_adapter] and/or [VtableAdapter] to create a compile time [AllocVtable] which will call T's
/// alloc,realloc,resize,and free if they are implemented on that type as methods
///
///
struct AllocContext final {
  template <class T>
  using Result = opt::Result<T*, opt::Error>;

  void* const ctx;
  const AllocVtable* const vtable;

  template <class T>
  constexpr RemoveRef<T>* context() const noexcept
    requires(Allocator<T>)
  {
    return static_cast<RemoveRef<T>*>(this->ctx);
  }

  AllocResult allocate(Layout layout) const noexcept {
    return this->vtable->alloc(this->ctx, layout.size, layout.align);
  }

  AllocResult allocate(isize size, isize align) const noexcept { return this->allocate(Layout{size, align}); }

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
    requires(Pod<T> && std::is_constructible_v<T, Args...>)
  {
    if (auto res = this->allocate<T>(); res.has_value()) {
      T* ptr = *res;
      [[assume(ptr != nullptr)]];
      *ptr = T{std::forward<Args>(args)...};
      return ptr;
    } else {
      return res;
    }
  }

  template <class T, class... Args>
  constexpr Result<T> construct(Args&&... args) const noexcept
    requires(!Pod<T> && std::is_constructible_v<T, Args...>)
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
    requires(!Pod<T> && std::is_destructible_v<T>)
  {
    std::destroy_at(val);
  }

  constexpr AllocResult reallocate(void* ptr, Layout old, Layout new_layout) const noexcept {
    return this->vtable->realloc(this->ctx, ptr, old.size, new_layout.size);
  }

  constexpr AllocResult reallocate(void* ptr, isize old_size, isize new_size) const noexcept {
    return this->reallocate(ptr, Layout{old_size, 1}, Layout{new_size, 1});
  }

  constexpr bool resize(void* ptr, Layout old, Layout new_layout) const noexcept {
    return this->vtable->resize(this->ctx, ptr, old.size, new_layout.size);
  }

  constexpr bool resize(void* ptr, isize old, isize new_size) const noexcept {
    return this->resize(ptr, Layout{old, 1}, Layout{new_size, 1});
  }

  constexpr void free(void* ptr, Layout layout) const noexcept { this->vtable->free(this->ctx, ptr, layout.size); }

  constexpr void free(void* ptr, isize size) const noexcept { this->free(ptr, Layout{size, 1}); }

  constexpr bool has_state() const noexcept { return nv::is_not_null(this->ctx); }

  constexpr bool is_stateless() const noexcept { return !this->has_state(); }
};

template <class T>
concept IntoAllocator = requires(T a) {
  { a.context() } noexcept -> std::convertible_to<AllocContext>;
  { static_cast<AllocContext>(a) } noexcept -> std::same_as<AllocContext>;
};

template <class T>
concept IsAllocatorStruct = std::same_as<T, AllocContext>;

template <class T>
concept AllocatorImpl = IntoAllocator<T> && Allocator<T>;

/// @breif Allocator Vtable Adapter generator
/// @details]
template <class T>
  requires(Allocator<T>)
struct VtableAdapter {
  static constexpr bool is_stateless = std::is_empty_v<T>;
  static constexpr bool has_state = !VtableAdapter<T>::is_stateless;

  [[gnu::returns_nonnull]]
  static constexpr T* context_cast(void* ctx) noexcept {
    if constexpr (is_stateless) {
      static T inst{};
      return &inst;
    } else {
      return static_cast<T*>(ctx);
    }
  }

 private:
  [[gnu::nonnull, gnu::alloc_size(2), gnu::alloc_align(3)]]
  static constexpr void* alloc_impl(void* ctx, isize size_bytes, isize align) noexcept
    requires(has_state)
  {
    assert_debug(
        is_not_null(ctx),
        "method alloc -> Context pointer is null for allocator vtable that contains state! Something went wrong with "
        "allocation of size: %li bytes and alignment %li",
        size_bytes, align);

    auto* self = static_cast<T*>(ctx);
    return self->alloc(size_bytes, align);
  }

  [[gnu::alloc_size(2), gnu::alloc_align(3)]]
  static constexpr void* alloc_impl(void*, isize size_bytes, isize align) noexcept
    requires(std::is_empty_v<T>)
  {
    return T::alloc(size_bytes, align);
  }

  [[gnu::nonnull(2), gnu::alloc_size(4)]]
  static constexpr void* realloc_impl(void* ctx, void* ptr, isize old_size, isize new_size) noexcept
    requires(has_state)
  {
    assert_debug(is_not_null(ctx),
                 "method realloc -> Context pointer is null for allocator vtable that contains state! Something went "
                 "wrong with "
                 "reallocation of old size: %li bytes to new size: %li bytes",
                 old_size, new_size);

    auto* self = static_cast<T*>(ctx);
    if constexpr (Resize<T>) {
      if (self->resize(ptr, old_size, new_size)) {
        return ptr;
      }
    }

    if constexpr (Reallocate<T>) {
      return self->realloc(ptr, old_size, new_size);
    } else {
      if (void* next = self->alloc(new_size)) [[likely]] {
        memcpy(next, ptr, old_size);
        self->free(ptr);
        return next;
      }
    }
    return nullptr;
  }

  [[gnu::nonnull(2), gnu::alloc_size(4)]]
  static constexpr void* realloc_impl(void*, void* ptr, isize old_size, isize new_size) noexcept
    requires(is_stateless)
  {
    if constexpr (Resize<T>) {
      if (T::resize(ptr, old_size, new_size)) {
        return ptr;
      }
    }

    if constexpr (Reallocate<T>) {
      return T::realloc(ptr, old_size, new_size);
    } else {
      if (void* next = T::alloc(new_size)) [[likely]] {
        memcpy(next, ptr, old_size);
        T::free(ptr);
        return next;
      }
    }
    return nullptr;
  }

  [[gnu::nonnull(2)]]
  static constexpr bool resize_impl(void*, void* ptr, isize old_size, isize new_size) noexcept
    requires(is_stateless)
  {
    if constexpr (Resize<T>) {
      return T::resize(ptr, old_size, new_size);
    } else {
      return false;
    }
  }

  static constexpr bool resize_impl(void* ctx, void* ptr, isize old_size, isize new_size) noexcept
    requires(has_state)
  {
    assert_debug(is_not_null(ctx),
                 "method resize -> Context pointer is null for allocator vtable that contains state! Something went "
                 "wrong with "
                 "resize of old size: %li bytes to new size: %li bytes",
                 old_size, new_size);

    if constexpr (Resize<T>) {
      auto* self = static_cast<T*>(ctx);
      return self->resize(ptr, old_size, new_size);
    } else {
      return false;
    }
  }

  static constexpr void free_impl(void* ctx, void* ptr, isize size_bytes) noexcept
    requires(has_state)
  {
    assert_debug(
        is_not_null(ctx),
        "method free -> Context pointer is null for allocator vtable that contains state! Something went wrong with "
        "free of size size: %li bytes!",
        size_bytes);

    auto* self = static_cast<T*>(ctx);
    self->free(ptr, size_bytes);
  }

  static constexpr void free_impl(void*, void* ptr, isize size_bytes) noexcept
    requires(is_stateless)
  {
    T::free(ptr, size_bytes);
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
  requires(Allocator<Unref<T>>)
{
  static constexpr AllocVtable VT = VtableAdapter<Unref<T>>::VT;
  return &VT;
}

/// @brief more explicitly named version of [nv::alloc_context] overload that takes a single optional param
template <class T>
  requires(Allocator<Unref<T>>)
constexpr AllocContext empty_alloc_context(const AllocVtable* vt = vtable_adapter<T>()) noexcept {
  assert_debug(is_not_null(vt), "Pointer for Allocator interface struct vtable must not be null!");
  return {.ctx = nullptr, .vtable = vt};
}

/// @brief returns a type erased allocator context
/// @details @see [AllocContext]
template <class T>
[[gnu::nonnull(2)]]
constexpr AllocContext alloc_context(T* ctx, const AllocVtable* vt = vtable_adapter<std::type_identity_t<T>>()) noexcept
  requires(AllocatorImpl<Unref<T>>)
{
  assert_debug(is_not_null(vt), "Pointer for Allocator interface struct vtable must not be null!");
  return {.ctx = static_cast<void*>(ctx), .vtable = vt};
}

template <class T>
[[gnu::nonnull]]
constexpr AllocContext alloc_context(const AllocVtable* vt = vtable_adapter<T>()) noexcept
  requires(Allocator<Unref<T>>)
{
  return nv::alloc_context<T>(nullptr, vt);
}

template <isize N>
using StaticMemory = std::array<byte, N>;

/// @brief Generalized way to access Allocator implementations
/// @details inspired by [std::allocator_traits], so this is libnv's version of that
/// you can specialize your own partial specialization if youd like custom behavior
template <class A>
struct AllocTraits;

template <class Al>
  requires(EmptyAllocator<Al>)
struct AllocTraits<Al> {
  /// @brief alias for inner template type Al
  using Impl = Al;
  /// @brief alias for decltype(*this)
  using Self = AllocTraits<Al>;

  static_assert(!std::same_as<Al, Self>,
                "Too many levels of AllocTraits nesting! try to just use 1 wrapper around a single allocator!");
  static_assert(std::is_object_v<Al>,
                "Template parameter must be an object type and should not be a pointer or reference type");

  static constexpr void* alloc(isize size, isize align) noexcept { return Impl::alloc(size, align); }
  static constexpr bool resize(void* ptr, isize old_size, isize new_size) noexcept {
    if constexpr (Resize<Impl>) {
      return Impl::resize(ptr, old_size, new_size);
    } else {
      return false;
    }
  }
  static constexpr void* realloc(void* ptr, isize old_size, isize new_size) noexcept {
    if constexpr (Reallocate<Impl>) {
      return Impl::realloc(ptr, old_size, new_size);
    } else {
      if constexpr (Resize<Impl>) {
        if (Impl::resize(ptr, old_size, new_size)) {
          return ptr;
        }
      }

      if (void* next = Impl::alloc(new_size, alignof(std::max_align_t))) [[likely]] {
        memcpy(next, ptr, old_size);
        Impl::free(ptr);
        return next;
      }
      return nullptr;
    }
  }
  static constexpr void free(void* ptr, isize size) noexcept { Impl::free(ptr, size); }

  template <class T>
  static constexpr void* realloc(T* ptr, isize old_count, isize new_count) noexcept {
    if (auto* p = static_cast<T*>(Self::realloc(ptr, old_count * sizeof(T), new_count * sizeof(T)))) {
      assert_debug(is_aligned(p), "Pointer returned by realloc is not properly aligned!");
      return p;
    }
    return nullptr;
  }

  template <class T>
  static constexpr bool resize(T* ptr, isize old_count, isize new_count) noexcept {
    return Self::resize(ptr, old_count * sizeof(T), new_count * sizeof(T));
  }

  template <class T>
  static constexpr void free(T* ptr) noexcept {
    Self::free(ptr, sizeof(T));
  }

  template <class T>
  static constexpr void free_array(T* ptr, isize count) noexcept {
    Self::free(ptr, sizeof(T) * count);
  }

  template <class T>
  static constexpr T* alloc() noexcept {
    return static_cast<T*>(Self::alloc(sizeof(T), alignof(T)));
  }

  template <class T>
  static constexpr T* alloc_array(isize count) noexcept {
    return static_cast<T*>(Self::alloc(sizeof(T) * count, alignof(T)));
  }

  template <class T, isize N>
  static constexpr ArrayPtr<T, N> alloc_array() noexcept {
    return static_cast<ArrayPtr<T, N>>(Self::alloc(sizeof(T) * N, alignof(T)));
  }

  static constexpr char* alloc_string(isize count) noexcept { return static_cast<char*>(Self::alloc(1, count)); }
  static constexpr Str dupstr(Str string) noexcept {
    const isize len = string.length();
    if (char* s = Self::alloc_string(len)) [[likely]] {
      memcpy(s, string.begin(), len);
      return {s, static_cast<usize>(len)};
    }
    return {};
  }

  /// @brief returns a type-erased [Allocator] struct
  /// @details [Allocator] returned points to template type A, not to this wrapper type
  static constexpr AllocContext context() noexcept { return nv::alloc_context<Al>(); }
};

template <class A>
  requires(StatefulAllocator<A>)
struct AllocTraits<A> {
  using Impl = A;
  using Self = AllocTraits<A>;

  static_assert(std::is_object_v<A>,
                "Template parameter must be an object type and should not be a pointer or reference type");

  static_assert(!std::same_as<A, Self>,
                "Too many levels of AllocTraits nesting! try to just use 1 wrapper around a single allocator!");

  Impl* self;

  constexpr void* alloc(isize size, isize align) const noexcept { return self->alloc(size, align); }
  constexpr bool resize(void* ptr, isize old_size, isize new_size) const noexcept {
    if constexpr (Resize<A>) {
      return self->resize(ptr, old_size, new_size);
    } else {
      return false;
    }
  }
  constexpr void* realloc(void* ptr, isize old_size, isize new_size) const noexcept {
    if constexpr (Reallocate<A>) {
      return self->realloc(ptr, old_size, new_size);
    } else {
      if constexpr (Resize<A>) {
        if (self->resize(ptr, old_size, new_size)) {
          return ptr;
        }
      }

      if (void* next = self->alloc(new_size, alignof(std::max_align_t))) [[likely]] {
        memcpy(next, ptr, old_size);
        self->free(ptr);
        return next;
      }
      return nullptr;
    }
  }
  constexpr void free(void* ptr, isize size) const noexcept { self->free(ptr, size); }

  template <class T>
  constexpr void* realloc(T* ptr, isize old_count, isize new_count) const noexcept {
    if (auto* p = static_cast<T*>(self->realloc(ptr, old_count * sizeof(T), new_count * sizeof(T)))) {
      assert_debug(is_aligned(p), "Pointer returned by realloc is not properly aligned!");
      return p;
    }
    return nullptr;
  }

  template <class T>
  constexpr bool resize(T* ptr, isize old_count, isize new_count) const noexcept {
    return this->resize(ptr, old_count * sizeof(T), new_count * sizeof(T));
  }

  template <class T>
  constexpr void free(T* ptr) const noexcept {
    this->free(ptr, sizeof(T));
  }

  template <class T>
  constexpr void free_array(T* ptr, isize count) const noexcept {
    this->free(ptr, sizeof(T) * count);
  }

  template <class T>
  constexpr T* alloc() const noexcept {
    return static_cast<T*>(this->alloc(sizeof(T), alignof(T)));
  }

  template <class T>
  constexpr T* alloc_array(isize count) const noexcept {
    return static_cast<T*>(this->alloc(sizeof(T) * count, alignof(T)));
  }

  template <class T, isize N>
  constexpr ArrayPtr<T, N> alloc_array() const noexcept {
    return static_cast<ArrayPtr<T, N>>(this->alloc(sizeof(T) * N, alignof(T)));
  }

  constexpr char* alloc_string(isize count) const noexcept { return static_cast<char*>(Self::alloc(1, count)); }

  constexpr Str dupstr(Str string) const noexcept {
    const isize len = string.length();
    if (char* s = this->alloc_string(len)) [[likely]] {
      memcpy(s, string.begin(), len);
      return {s, static_cast<usize>(len)};
    }
    return {};
  }

  /// @brief returns a type-erased [Allocator] struct
  /// @details [Allocator] returned points to self pointer of template type A, not to this wrapper type
  constexpr AllocContext context() const noexcept { return nv::alloc_context(self); }

  constexpr operator AllocContext() const noexcept { return this->context(); }
};

/// @brief quick way to create an instance of AllocTraits for an EmptyAllocator impl
template <class T>
  requires(std::is_object_v<T> && EmptyAllocator<T>)
inline constexpr AllocTraits<T> empty_allocator{};

/// @brief wraps pointer StatefulAllocator impl in AllocTraits struct interface
template <class T>
  requires(std::is_object_v<T> && StatefulAllocator<T>)
[[gnu::nonnull]]
constexpr AllocTraits<T> to_traits(T* state) {
  assert_debug(is_not_null(state), "got null pointer when it must not be null.");
  return AllocTraits{.self = state};
}

/// @brief version overload of above [to_traits] that wraps EmptyAllocator impls
template <class T>
  requires(std::is_object_v<T> && EmptyAllocator<T>)
consteval AllocTraits<T> to_traits() {
  return empty_allocator<T>;
}

template <class T>
AllocTraits(T*) -> AllocTraits<T>;

/// @brief a (non-owning) fixed sized, bump-style arena allocator
/// @details This is a lite cpp wrapper around the [NvArena] C impl
/// @warning Memory is not owned by this type and should be freed at callers discretion
/// @remarks This type has static (cons)tructor methods to initalize a new instance on the stack. i.e.
/// [BumpArena::cons]
struct BumpArena final {
  using CArena = ::NvArena;
  using Self = BumpArena;

  using Traits = AllocTraits<Self>;

  CArena inner;

  static constexpr BumpArena cons(isize size, AllocContext alloc) noexcept {
    if (void* ptr = alloc.allocate(layout_array_of<byte>(size))) [[likely]] {
      byte* begin = static_cast<byte*>(ptr);
      return BumpArena::cons(begin, begin + size);
    }
    return {};
  }

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  [[gnu::pure, gnu::nonnull]]
  static constexpr BumpArena cons(byte* begin, byte* end) noexcept {
    const isize len = end - begin;
    return {.inner = arena_new(begin, len)};
  }

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  [[gnu::pure]]
  static constexpr BumpArena cons(byte* begin, isize size_bytes) noexcept {
    return {.inner = arena_new(begin, size_bytes)};
  }

  /// @brief (cons)tructs a new instance of [BufferAlloc] on the stack
  template <usize N>
  constexpr BumpArena cons(StaticMemory<N>* mem) noexcept {
    return Self::cons(mem->begin(), mem->end());
  }

  [[gnu::always_inline]]
  constexpr AllocResult alloc(Layout layout) noexcept {
    if (auto* ptr = arena_alloc(&this->inner, layout)) [[likely]] {
      return ptr;
    }
    return nullptr;
  }

  [[gnu::always_inline]]
  constexpr AllocResult alloc(isize size, isize align) noexcept {
    return this->alloc(Layout{.size = size, .align = align});
  }
  [[gnu::always_inline]]
  constexpr AllocResult realloc(void* ptr, isize old, isize new_size) noexcept {
    return this->realloc(ptr, Layout{.size = old, .align = 1}, Layout{.size = new_size, .align = 1});
  }

  [[gnu::always_inline]]
  constexpr AllocResult realloc(void* ptr, Layout old, Layout new_layout) noexcept {
    if (auto* res = arena_realloc(&this->inner, ptr, old, new_layout)) {
      return res;
    }
    return nullptr;
  }

  [[gnu::always_inline]]
  constexpr bool resize(void* ptr, Layout old, Layout new_layout) noexcept {
    return arena_resize(&this->inner, ptr, old, new_layout);
  }

  [[gnu::always_inline]]
  constexpr bool resize(void* ptr, isize old, isize new_size) noexcept {
    return arena_resize(&this->inner, ptr, Layout{old, 1}, Layout{new_size, 1});
  }

  constexpr void free(void*, Layout) noexcept {}
  constexpr void free(void*, isize) noexcept {}

  constexpr operator AllocContext() noexcept { return this->context(); }

  constexpr Traits to_traits() noexcept { return nv::to_traits(this); }

  [[gnu::returns_nonnull]]
  static constexpr const AllocVtable* vtable() noexcept {
    // vtable_adapter<ChunkArena>();
    return VtableAdapter<BumpArena>::vtable();
  }

  constexpr AllocContext context() noexcept {
    return AllocContext{.ctx = static_cast<void*>(this), .vtable = vtable()};
  }

  constexpr usize available() const noexcept { return arena_avail(&this->inner); }
  constexpr usize used_bytes() const noexcept { return arena_used_bytes(&this->inner); }
};

template struct VtableAdapter<BumpArena>;
template struct AllocTraits<BumpArena>;

using BumpAlloc = AllocTraits<BumpArena>;

/// @brief wraps A in [AllocTraits] and invokes its (static) alloc method
/// @details first parameter, while required, is not used at all, and is only kept
/// to keep this functions parameters uniform with the variant that operates on non-empty (statefull) allocator impls
///
template <class A>
  requires(EmptyAllocator<A>)
constexpr void* allocate(isize size, isize align, A = empty_allocator<A>) noexcept {
  return AllocTraits<A>::alloc(size, align);
}

template <class A>
  requires(StatefulAllocator<A>)
[[gnu::nonnull]]
constexpr void* allocate(isize size, isize align, A* allocator) noexcept {
  return allocator->alloc(size, align);
}

constexpr void* allocate(isize size, isize align, AllocContext ctx) noexcept { return ctx.allocate(size, align); }
}  // namespace nv
#endif

#ifndef __cplusplus

#error "libnv cxx module alloc.hpp can only be included/used by C++!";

#endif  // #ifndef __cplusplus
