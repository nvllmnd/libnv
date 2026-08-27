#pragma once

#include <concepts>
#include <cstddef>

#include "nv/core/algo.h"
#include "nv/memory/vmem.h"
#include "nvxx/common.hpp"

namespace nv {

[[gnu::malloc]]
constexpr void* map_memory(isize size, bool noreserve = false) noexcept {
  assert_debug(size >= 0, "size passed to map_memory must be non-negative! (>= 0)");
  return vmemory_map(size, noreserve);
}

[[gnu::nonnull]]
constexpr void* remap_memory(void* ptr, isize old_size, isize new_size, bool relocate) noexcept {
  assert_debug(is_not_null(ptr), "cannot pass nullptr to remap_memory!");
  assert_debug(old_size >= 0 && new_size >= 0, "numeric paramter passed to remap_memory must be non-negative!");
  return vmemory_remap(ptr, old_size, new_size, relocate);
}

constexpr void unmap_memory(void* ptr, isize size) noexcept { vmemory_unmap(ptr, size); }

struct SizeHeader {
  isize size_bytes;
};

template <class T>
concept VirtMemHeader = requires(T val) {
  { val.size_bytes } -> std::convertible_to<isize>;
} && Pod<T>;

template <class T, class H>
concept VirtMemData = Pod<T> && VirtMemHeader<H>;

template <class T, class Header = SizeHeader>
  requires(VirtMemData<T, Header>)
struct VirtMemory {
  using ResourceType = ValType<T>;
  using ResourceIter = ptr<ResourceType>;

  Header head;
  T start[];

  static constexpr isize page_size() noexcept {
    static const isize size = os_page_size();
    return size;
  }

  constexpr T* data() noexcept { return &this->start[0]; }
  constexpr const T* data() const noexcept { return &this->start[0]; }

  constexpr T* begin() noexcept { return &this->start[0]; }
  constexpr T* end() noexcept { return this->begin() + this->length(); }
  constexpr const T* begin() const noexcept { return &this->start[0]; }
  constexpr const T* end() const noexcept { return this->begin() + this->length(); }

  constexpr isize length() const noexcept { return this->size_bytes() / sizeof(T); }
  constexpr isize size_bytes() const noexcept { return this->head.size_bytes; }

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t index) noexcept {
    assert_debug(index >= 0 && index < self.size_bytes(),
                 "index: %li is out of range for VirtMemory of size: %li bytes!", index, self.size_bytes());

    return self.start[index];
  }

  constexpr decltype(auto) index(this auto& self, std::ptrdiff_t index) noexcept {
    ASSERT(index >= 0 && index < self.size_bytes(), "index: %li is out of range for VirtMemory of size: %li bytes!",
           index, self.size_bytes());
    return self[index];
  }

  constexpr bool contains(const T* ptr) const noexcept { return ptr >= this->begin() && ptr < this->end(); }

  constexpr bool contains(const void* ptr) const noexcept {
    auto* p = static_cast<T*>(ptr);
    return this->contains(p);
  }

  constexpr isize offset(const T* ptr) const noexcept {
    if (this->contains(ptr)) {
      return ptr - this->begin();
    }
    return -1;
  }

  constexpr usize offset_unchecked(const T* ptr) const noexcept { return static_cast<usize>(ptr - this->begin()); }
};

template <>
struct VirtMemory<byte> {
  using ResourceType = byte;
  using ResourceIter = ptr<ResourceType>;

  SizeHeader head;
  byte start[];

  static constexpr isize page_size() noexcept {
    static const isize size = os_page_size();
    return size;
  }

  constexpr byte* data() noexcept { return &this->start[0]; }
  constexpr const byte* data() const noexcept { return &this->start[0]; }

  constexpr byte* begin() noexcept { return &this->start[0]; }
  constexpr byte* end() noexcept { return this->begin() + this->size_bytes(); }
  constexpr const byte* begin() const noexcept { return &this->start[0]; }
  constexpr const byte* end() const noexcept { return this->begin() + this->size_bytes(); }

  constexpr decltype(auto) operator[](this auto& self, std::ptrdiff_t index) noexcept {
    assert_debug(index >= 0 && index < self.size_bytes(),
                 "index: %li is out of range for VirtMemory of size: %li bytes!", index, self.size_bytes());

    return self.start[index];
  }
  constexpr decltype(auto) index(this auto& self, std::ptrdiff_t index) noexcept {
    ASSERT(index >= 0 && index < self.size_bytes(), "index: %li is out of range for VirtMemory of size: %li bytes!",
           index, self.size_bytes());
    return &self[index];
  }

  constexpr isize length() const noexcept { return this->size_bytes(); }
  constexpr isize size_bytes() const noexcept { return this->head.size_bytes; }

  constexpr bool contains(const void* ptr) const noexcept {
    const auto* p = static_cast<const byte*>(ptr);
    return p >= this->begin() && p < this->end();
  }

  constexpr isize offset(const byte* ptr) const noexcept {
    if (this->contains(ptr)) {
      return ptr - this->begin();
    }
    return -1;
  }

  constexpr usize offset_unchecked(const byte* ptr) const noexcept {
    assert_debug(
        this->contains(ptr),
        "attempted unchecked offset computation would result in UB as this VirtMemory does not own this pointer!");
    return static_cast<usize>(ptr - this->begin());
  }
};

using Vmem = VirtMemory<byte>;

template <class T = byte, class H = SizeHeader>
  requires(VirtMemData<T, H>)
constexpr VirtMemory<T, H>* vmemory_new(isize size, bool noreserve = false) noexcept {
  if (auto* vm = static_cast<VirtMemory<T, H>*>(map_memory(size, noreserve))) [[likely]] {
    vm->head.size_bytes = size;
    return vm;
  } else {
    return nullptr;
  }
}

template <class T = byte, class H = SizeHeader>
  requires(VirtMemData<T, H>)
[[gnu::nonnull]]
constexpr VirtMemory<T, H>* vmemory_remap(VirtMemory<T, H>* ptr, isize new_size, bool relocate = false) noexcept {
  if (auto* vm = static_cast<VirtMemory<T, H>*>(
          remap_memory(static_cast<void*>(ptr), ptr->size_bytes(), new_size, relocate))) {
    vm->head.size_bytes = new_size;
    return vm;
  } else {
    return nullptr;
  }
}

/// @details frees given VirtMemory. takes a pointer to a pointer, so that source pointer is set to nullptr after this
/// function returns.
/// Its valid to pass a pointer to nullptr to this function, but UB if nullptr is passed to this function
template <class T = byte, class H = SizeHeader>
  requires(VirtMemData<T, H>)
[[gnu::nonnull]]
constexpr void vmemory_destroy(VirtMemory<T, H>** ptr) noexcept {
  auto* p = *ptr;
  if (is_not_null(p)) {
    unmap_memory(static_cast<void*>(p), p->size_bytes());
    *ptr = nullptr;
  }
}

constexpr auto vmem_new = vmemory_new<>;
constexpr auto vmem_remap = vmemory_remap<>;
constexpr auto vmem_destroy = vmemory_destroy<>;

}  // namespace nv
