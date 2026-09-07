#pragma once

#include "nv/iter/iterators.h"
#include "nvxx/alloc.hpp"
namespace nv {

struct ArenaBlock {
  struct ArenaBlock* prev;
  byte* cursor;
  isize size;

  byte storage[];

  constexpr isize len() const noexcept { return this->size; }

  constexpr byte* begin() noexcept { return &this->storage[0]; }
  constexpr const byte* cbegin() const noexcept { return &this->storage[0]; }

  constexpr byte* end() noexcept { return this->begin() + this->len(); }
  constexpr const byte* cend() const noexcept { return this->cbegin() + this->len(); }
};

struct ArenaState {
  ArenaBlock* head;
  ArenaBlock* tail;
};

template <class A>
  requires(AllocatorTraits<A> || std::same_as<A, Allocator>)
struct Arena {
  static constexpr bool is_alloc = std::same_as<A, Allocator>;

  A alloc;

  constexpr void dothing() noexcept
    requires(is_alloc)
  {
    this->alloc.allocate();
  }
};

// template <>
// struct Arena<Allocator> {};
//
// template <class A>
//   requires(AllocatorTraits<A> && std::is_empty_v<A>)
// struct Arena<A> {};
//
// template <class A>
//   requires(AllocatorTraits<A>)
// struct Arena<A> {};
//
}  // namespace nv
