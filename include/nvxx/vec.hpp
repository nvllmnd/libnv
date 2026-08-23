#pragma once

#include <type_traits>
#include "nvxx/alloc.hpp"
#include "nvxx/common.hpp"
#include "nvxx/vmem.hpp"

namespace nv {

template <class T>
  requires(Pod<T>)
struct VecMem {
  using FlexMemberType = std::remove_cvref_t<T>[];
  using FlexValueType = std::remove_all_extents_t<FlexMemberType>;

  isize count;
  isize capacity;
  FlexMemberType start;

  template <class A>
    requires(AllocatorTraits<A> && !IsAllocator<A>)
  // static constexpr VecMem* allocate(isize capacity, A* alloc) noexcept {}
  //
  // static constexpr VecMem* allocate(isize capacity, Allocator alloc) noexcept {}

  constexpr usize len() const noexcept {
    return this->count;
  }
  constexpr isize size_bytes_full() const noexcept { return this->size_bytes() + sizeof(VecMem<T>); }
  constexpr isize size_bytes() const noexcept { return this->count * sizeof(T); }
  constexpr isize size_capacity_full() const noexcept { return this->size_capacity() + sizeof(VecMem<T>); }
  constexpr isize size_capacity() const noexcept { return this->capacity * sizeof(T); }
};

template <class T>
concept VecAllocatorTraits = (std::is_void_v<T> || IsAllocator<T> || AllocatorTraits<T>);

template <class T, class A>
concept VecTraits = VecAllocatorTraits<A> && Pod<T>;

// namespace priv {
// template <class T, class A>
//   requires((!std::is_void_v<A> && (IsAllocator<A> || AllocatorTraits<A>)) && Pod<T>)
// [[gnu::nonnull]]
// constexpr VecMem<T>* vec_mem_new(isize capacity, A* alloc) noexcept {
//   T* ptr = nullptr;
//   if constexpr (IsAllocator<A>) {
//     ptr = alloc->allocate
//   } else {
//   }
// }

// }  // namespace priv

template <class T, class Alloc = Allocator>
  requires(VecTraits<T, Alloc>)
struct Vec {
  CONTAINER_TEMPLATE_TYPES(T);
  CONTAINER_TEMPLATE_TYPES_AS(Allocator, Alloc);

  using Memory = VecMem<ValueType>;

  Memory* mem;
  Alloc alloc;
};

// template <class T, class A>
//   requires(VecTraits<T, A>)
// constexpr Vec<T, A> vec_new(isize capacity = 0) noexcept {
//   if (auto* ptr = static_cast<VecMem<T>*>()) }
//
template <class T>
  requires(Pod<T>)
struct Vec<T, void> {
  using Memory = VecMem<T>;
  using AllocatorType = Allocator;

  Memory* mem;
};

template <class T>
  requires(Pod<T>)
using RawVec = Vec<T, void>;

// template <class T, class U>
// concept VecLike = requires(T v) {
//   { v.mem } -> std::convertible_to<typename T::Memory>;
//   std::same_as<typename T::Memory, VecMem<U>>;
// };

}  // namespace nv
