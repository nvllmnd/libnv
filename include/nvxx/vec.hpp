#pragma once

#include <type_traits>
#include "nvxx/alloc.hpp"
#include "nvxx/common.hpp"

namespace nv {

template <class T>
  requires(Pod<T>)
struct VecMem {
  using FlexMemberType = RemoveCvptr<T>[];
  using FlexValueType = std::remove_all_extents_t<FlexMemberType>;

  isize count;
  isize capacity;
  FlexMemberType start;

  template <class A>
    requires(AllocatorTraits<A> && !IsAllocatorStruct<A>)

  constexpr usize len() const noexcept {
    return this->count;
  }
  constexpr isize size_bytes_full() const noexcept { return this->size_bytes() + sizeof(VecMem<T>); }
  constexpr isize size_bytes() const noexcept { return this->count * sizeof(T); }

  constexpr isize size_capacity_full() const noexcept { return this->size_capacity() + sizeof(VecMem<T>); }
  constexpr isize size_capacity() const noexcept { return this->capacity * sizeof(T); }
};

template <class T>
using VecMemFlexType = typename VecMem<T>::FlexMemberType;

template <class T>
using VecMemFlexValueType = typename VecMem<T>::FlexValueType;

template <class T>
concept VecAllocatorTraits = (std::is_void_v<T> || IsAllocatorStruct<T> || AllocatorTraits<T>);

template <class T, class A>
concept VecTraits = VecAllocatorTraits<A> && Pod<T>;

template <class T, class Alloc>
  requires(VecTraits<T, Alloc>)
struct Vec {
  CONTAINER_TEMPLATE_TYPES(T);
  CONTAINER_TEMPLATE_TYPES_AS(Allocator, Alloc);

  using Memory = VecMem<ValueType>;

  Memory* mem;
  Alloc alloc;
};

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

}  // namespace nv
