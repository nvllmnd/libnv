#pragma once

#include "nv/iter/iterators.h"
#include "nvxx/alloc.hpp"
#include "nvxx/vmem.hpp"
namespace nv {

struct ArenaBlock {
  struct ArenaBlock* prev;
  isize size;

  BumpCursor cursor_;

  byte storage[];

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t index) noexcept {
    assert_debug(0 <= index && index < self.capacity(), "Index: %li out of range! (0-%li)", index, self.capacity());
    return self.storage[index];
  }

  constexpr byte* cursor() noexcept { return this->cursor_.top(); }

  constexpr const byte* cursor() const noexcept { return this->cursor_.top(); }

  constexpr isize used_bytes() const noexcept { return this->cursor() - this->begin(); }
  constexpr isize avail_bytes() const noexcept { return this->end() - this->cursor(); }

  /// @brief storagae size of this block in bytes, not including header size
  constexpr isize len() const noexcept { return this->size; }

  /// @brief same as [len] or [capacity], but accounts for the size of this blocks header as well
  constexpr isize block_size() const noexcept { return this->len() + sizeof(ArenaBlock); }
  /// @breif same as [len]
  constexpr isize capacity() const noexcept { return this->len(); }

  constexpr byte* begin() noexcept { return &this->storage[0]; }
  constexpr const byte* begin() const noexcept { return &this->storage[0]; }

  constexpr byte* end() noexcept { return this->begin() + this->len(); }
  constexpr const byte* end() const noexcept { return this->begin() + this->len(); }

  [[gnu::alloc_size(2), gnu::alloc_align(3)]]
  constexpr void* alloc(isize size, isize align) noexcept {
    if (size < this->len() && this->avail_bytes() < size) {
      return nullptr;
    }
    // we check if this block has enough space above so this will only
    // every return nullptr in case extra padding is used for alignment that would overflow our buffer
    if (void* ptr = bc.allocate_raw(size, align)) [[likely]] {
      this->cursor_ = bc.top();
      return ptr;
    }
    return nullptr;
  }

  [[gnu::nonnull]]
  constexpr bool resize(void* ptr, isize oldsize, isize newsize) noexcept {
    if (this->contains(ptr)) [[likely]] {
      BumpCursor bc = this->bump_cursor();
      if (bc.resize_raw(ptr, oldsize, newsize)) {
        this->cursor_ = bc.top();
        return true;
      }
    }
    return false;
  }

  [[gnu::nonnull]]
  constexpr void* realloc(void* ptr, isize oldsize, isize newsize) noexcept {
    if (this->resize(ptr, oldsize, newsize)) {
      return ptr;
    }

    BumpCursor bc = this->bump_cursor();
    if (void* res = bc.reallocate_raw(ptr, oldsize, newsize)) {
      this->cursor_ = bc.top();
      return res;
    }
    return nullptr;
  }

  constexpr void free(void*, isize) noexcept {}

  constexpr void free_zeroed(void* ptr, isize size) noexcept {
    if (this->contains(ptr)) {
      BumpCursor bc = this->bump_cursor();
      bc.free_raw(ptr, size);
    }
  }

  [[gnu::pure]]
  constexpr bool contains(const void* ptr) const noexcept {
    return this->begin() <= ptr && ptr < this->end();
  }
};

namespace priv {

constexpr ArenaBlock* alloc_block(isize size, AllocContext alloc) noexcept {
  if (auto* ptr = static_cast<ArenaBlock*>(allocate(size + sizeof(ArenaBlock), alignof(ArenaBlock), alloc))) {
    ptr->prev = nullptr;
    ptr->size = size;
    ptr->cursor_ = BumpCursor::cons(ptr->begin(), ptr->end(), ptr->begin());
    return ptr;
  }
  return nullptr;
}

template <class T>
  requires(EmptyAllocator<T> && !std::same_as<T, Vmem>)
constexpr ArenaBlock* alloc_block(isize size, T = empty_allocator<T>) noexcept {
  if (auto* ptr =
          static_cast<ArenaBlock*>(allocate(size + sizeof(ArenaBlock), alignof(ArenaBlock), empty_allocator<T>))) {
    ptr->prev = nullptr;
    ptr->size = size;
    ptr->cursor_ = BumpCursor::cons(ptr->begin(), ptr->end(), ptr->begin());
    return ptr;
  }
  return nullptr;
}

template <class T>
  requires(StatefulAllocator<T>)
constexpr ArenaBlock* alloc_block(isize size, T* alloc) noexcept {
  if (auto* ptr = static_cast<ArenaBlock*>(allocate(size + sizeof(ArenaBlock), alignof(std::max_align_t), alloc))) {
    ptr->prev = nullptr;
    ptr->size = size;
    ptr->cursor_ = BumpCursor::cons(ptr->begin(), ptr->end(), ptr->cursor());
    return ptr;
  }
  return nullptr;
}

constexpr ArenaBlock* alloc_block(isize size, Vmem = {}) noexcept {
  if (auto* ptr = static_cast<ArenaBlock*>(Vmem::alloc(size))) {
    ptr->prev = nullptr;
    ptr->size = size;
    ptr->cursor_ = BumpCursor::cons(ptr->begin(), ptr->end(), ptr->cursor());
    return ptr;
  }
  return nullptr;
}

}  // namespace priv

struct ArenaState {
  ArenaBlock* head;
  ArenaBlock* tail;
};

template <class A>
  requires(Allocator<A> || std::same_as<A, AllocContext>)
struct Arena {
  static constexpr bool is_alloc = std::same_as<A, AllocContext>;

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
